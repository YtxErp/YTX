#include "transmodelo.h"

#include "global/resourcepool.h"
#include "transmodelutils.h"

TransModelO::TransModelO(CTransModelArg& arg, const Node* node, CNodeModel* product_tree, Sqlite* sqlite_stakeholder, QObject* parent)
    : TransModel { arg, parent }
    , product_tree_ { static_cast<const NodeModelP*>(product_tree) }
    , sqlite_stakeholder_ { static_cast<SqliteS*>(sqlite_stakeholder) }
    , node_ { node }
    , party_id_ { node->party }
{
    if (node_id_ >= 1)
        sql_->ReadTrans(trans_shadow_list_, node_id_);

    if (party_id_ >= 1)
        sqlite_stakeholder_->ReadTrans(party_id_);
}

void TransModelO::UpdateLhsNode(int node_id)
{
    assert(node_id >= 1 && "Assertion failed: node_id must be positive (>= 1)");
    assert(node_id_ == 0 && "Assertion failed: node_id_ must be 0");

    node_id_ = node_id;
    if (trans_shadow_list_.isEmpty())
        return;

    PurifyTransShadow(node_id);

    if (!trans_shadow_list_.isEmpty())
        sql_->WriteTransRange(trans_shadow_list_);
}

void TransModelO::UpdateParty(int node_id, int party_id)
{
    assert(node_id_ == node_id && "Assertion failed: node_id_ must be equal to node_id");
    if (party_id_ == party_id)
        return;

    party_id_ = party_id;
    sqlite_stakeholder_->ReadTrans(party_id);
}

void TransModelO::SyncRule(int node_id, bool value)
{
    assert(node_id_ == node_id && "Assertion failed: node_id_ must be equal to node_id");
    if (node_rule_ == value)
        return;

    node_rule_ = value;

    if (trans_shadow_list_.isEmpty())
        return;

    beginResetModel();
    for (auto* trans_shadow : std::as_const(trans_shadow_list_)) {
        *trans_shadow->lhs_debit *= -1;
        *trans_shadow->lhs_credit *= -1;
        *trans_shadow->discount *= -1;
        *trans_shadow->rhs_credit *= -1;
        *trans_shadow->rhs_debit *= -1;
    }
    endResetModel();
}

void TransModelO::RSyncBoolWD(int node_id, int column, bool value)
{
    assert(node_id_ == node_id && "Assertion failed: node_id_ must be equal to node_id");

    if (NodeEnumO(column) == NodeEnumO::kFinished) {
        if (!value)
            return;

        PurifyTransShadow();
    }

    if (NodeEnumO(column) == NodeEnumO::kRule) {
        SyncRule(node_id, value);
    }
}

void TransModelO::RSyncInt(int node_id, int column, int value)
{
    const NodeEnumO kColumn { column };

    switch (kColumn) {
    case NodeEnumO::kID:
        UpdateLhsNode(node_id);
        break;
    case NodeEnumO::kParty:
        UpdateParty(node_id, value);
        break;
    default:
        break;
    }
}

QVariant TransModelO::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* trans_shadow { trans_shadow_list_.at(index.row()) };
    const TransEnumO kColumn { index.column() };

    switch (kColumn) {
    case TransEnumO::kID:
        return *trans_shadow->id;
    case TransEnumO::kCode:
        return *trans_shadow->code;
    case TransEnumO::kInsideProduct:
        return *trans_shadow->rhs_node == 0 ? QVariant() : *trans_shadow->rhs_node;
    case TransEnumO::kUnitPrice:
        return *trans_shadow->lhs_ratio == 0 ? QVariant() : *trans_shadow->lhs_ratio;
    case TransEnumO::kSecond:
        return *trans_shadow->lhs_credit == 0 ? QVariant() : *trans_shadow->lhs_credit;
    case TransEnumO::kDescription:
        return *trans_shadow->description;
    case TransEnumO::kColor:
        return *trans_shadow->rhs_node == 0 ? QVariant() : product_tree_->Color(*trans_shadow->rhs_node);
    case TransEnumO::kFirst:
        return *trans_shadow->lhs_debit == 0 ? QVariant() : *trans_shadow->lhs_debit;
    case TransEnumO::kNetAmount:
        return *trans_shadow->rhs_credit == 0 ? QVariant() : *trans_shadow->rhs_credit;
    case TransEnumO::kDiscount:
        return *trans_shadow->discount == 0 ? QVariant() : *trans_shadow->discount;
    case TransEnumO::kGrossAmount:
        return *trans_shadow->rhs_debit == 0 ? QVariant() : *trans_shadow->rhs_debit;
    case TransEnumO::kDiscountPrice:
        return *trans_shadow->rhs_ratio == 0 ? QVariant() : *trans_shadow->rhs_ratio;
    case TransEnumO::kOutsideProduct:
        return *trans_shadow->support_id == 0 ? QVariant() : *trans_shadow->support_id;
    default:
        return QVariant();
    }
}

bool TransModelO::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    const TransEnumO kColumn { index.column() };
    const int kRow { index.row() };

    auto* trans_shadow { trans_shadow_list_.at(kRow) };
    const int old_rhs_node { *trans_shadow->rhs_node };
    const double old_first { *trans_shadow->lhs_debit };
    const double old_second { *trans_shadow->lhs_credit };
    const double old_discount { *trans_shadow->discount };
    const double old_gross_amount { *trans_shadow->rhs_debit };
    const double old_net_amount { *trans_shadow->rhs_credit };

    bool ins_changed { false };
    bool fir_changed { false };
    bool sec_changed { false };
    bool uni_changed { false };
    bool dis_changed { false };

    const int kCoefficient { node_rule_ ? -1 : 1 };

    switch (kColumn) {
    case TransEnumO::kCode:
        TransModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kCode, value.toString(), &TransShadow::code);
        break;
    case TransEnumO::kDescription:
        TransModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kDescription, value.toString(), &TransShadow::description);
        break;
    case TransEnumO::kInsideProduct:
        ins_changed = UpdateInsideProduct(trans_shadow, value.toInt());
        break;
    case TransEnumO::kUnitPrice:
        uni_changed = UpdateUnitPrice(trans_shadow, value.toDouble());
        break;
    case TransEnumO::kSecond:
        sec_changed = UpdateSecond(trans_shadow, value.toDouble(), kCoefficient);
        break;
    case TransEnumO::kFirst:
        fir_changed = UpdateFirst(trans_shadow, value.toDouble(), kCoefficient);
        break;
    case TransEnumO::kDiscountPrice:
        dis_changed = UpdateDiscountPrice(trans_shadow, value.toDouble());
        break;
    case TransEnumO::kOutsideProduct:
        ins_changed = UpdateOutsideProduct(trans_shadow, value.toInt());
        break;
    default:
        return false;
    }

    emit SResizeColumnToContents(index.column());

    if (fir_changed)
        emit SUpdateLeafValue(*trans_shadow->lhs_node, 0.0, 0.0, *trans_shadow->lhs_debit - old_first);

    if (sec_changed) {
        double second_delta { *trans_shadow->lhs_credit - old_second };
        double gross_amount_delta { *trans_shadow->rhs_debit - old_gross_amount };
        double discount_delta { *trans_shadow->discount - old_discount };
        double net_amount_delta { *trans_shadow->rhs_credit - old_net_amount };
        emit SUpdateLeafValue(*trans_shadow->lhs_node, gross_amount_delta, net_amount_delta, 0.0, second_delta, discount_delta);
    }

    if (uni_changed) {
        double gross_amount_delta { *trans_shadow->rhs_debit - old_gross_amount };
        double net_amount_delta { *trans_shadow->rhs_credit - old_net_amount };
        emit SUpdateLeafValue(*trans_shadow->lhs_node, gross_amount_delta, net_amount_delta);
    }

    if (dis_changed) {
        double discount_delta { *trans_shadow->discount - old_discount };
        double net_amount_delta { *trans_shadow->rhs_credit - old_net_amount };
        emit SUpdateLeafValue(*trans_shadow->lhs_node, 0.0, net_amount_delta, 0.0, 0.0, discount_delta);
    }

    if (node_id_ == 0) {
        return false;
    }

    if (ins_changed) {
        if (old_rhs_node == 0) {
            sql_->WriteTrans(trans_shadow);
            emit SUpdateLeafValue(*trans_shadow->lhs_node, *trans_shadow->rhs_debit, *trans_shadow->rhs_credit, *trans_shadow->lhs_debit,
                *trans_shadow->lhs_credit, *trans_shadow->discount);
        } else
            sql_->WriteField(info_.trans, kInsideProduct, value.toInt(), *trans_shadow->id);
    }

    return true;
}

void TransModelO::sort(int column, Qt::SortOrder order)
{
    assert(column >= 0 && column < info_.node_header.size() && "Column index out of range");

    auto Compare = [column, order](TransShadow* lhs, TransShadow* rhs) -> bool {
        const TransEnumO kColumn { column };

        switch (kColumn) {
        case TransEnumO::kCode:
            return (order == Qt::AscendingOrder) ? (*lhs->code < *rhs->code) : (*lhs->code > *rhs->code);
        case TransEnumO::kInsideProduct:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_node < *rhs->rhs_node) : (*lhs->rhs_node > *rhs->rhs_node);
        case TransEnumO::kUnitPrice:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_ratio < *rhs->lhs_ratio) : (*lhs->lhs_ratio > *rhs->lhs_ratio);
        case TransEnumO::kFirst:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_debit < *rhs->lhs_debit) : (*lhs->lhs_debit > *rhs->lhs_debit);
        case TransEnumO::kSecond:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_credit < *rhs->lhs_credit) : (*lhs->lhs_credit > *rhs->lhs_credit);
        case TransEnumO::kNetAmount:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_credit < *rhs->rhs_credit) : (*lhs->rhs_credit > *rhs->rhs_credit);
        case TransEnumO::kGrossAmount:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_debit < *rhs->rhs_debit) : (*lhs->rhs_debit > *rhs->rhs_debit);
        case TransEnumO::kDiscountPrice:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_ratio < *rhs->rhs_ratio) : (*lhs->rhs_ratio > *rhs->rhs_ratio);
        case TransEnumO::kOutsideProduct:
            return (order == Qt::AscendingOrder) ? (*lhs->support_id < *rhs->support_id) : (*lhs->support_id > *rhs->support_id);
        case TransEnumO::kDiscount:
            return (order == Qt::AscendingOrder) ? (*lhs->discount < *rhs->discount) : (*lhs->discount > *rhs->discount);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(trans_shadow_list_.begin(), trans_shadow_list_.end(), Compare);
    emit layoutChanged();
}

Qt::ItemFlags TransModelO::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const TransEnumO kColumn { index.column() };

    switch (kColumn) {
    case TransEnumO::kID:
    case TransEnumO::kGrossAmount:
    case TransEnumO::kDiscount:
    case TransEnumO::kNetAmount:
    case TransEnumO::kColor:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}

bool TransModelO::removeRows(int row, int /*count*/, const QModelIndex& parent)
{
    assert(row >= 0 && row <= rowCount(parent) - 1 && "Row must be in the valid range [0, rowCount(parent) - 1]");

    auto* trans_shadow { trans_shadow_list_.at(row) };
    const int lhs_node { *trans_shadow->lhs_node };
    const int rhs_node { *trans_shadow->rhs_node };

    beginRemoveRows(parent, row, row);
    trans_shadow_list_.removeAt(row);
    endRemoveRows();

    emit SUpdateLeafValue(
        lhs_node, -*trans_shadow->rhs_debit, -*trans_shadow->rhs_credit, -*trans_shadow->lhs_debit, -*trans_shadow->lhs_credit, -*trans_shadow->discount);

    if (lhs_node != 0 && rhs_node != 0) {
        sql_->RemoveTrans(*trans_shadow->id);
    }

    ResourcePool<TransShadow>::Instance().Recycle(trans_shadow);
    return true;
}

bool TransModelO::UpdateInsideProduct(TransShadow* trans_shadow, int value)
{
    if (*trans_shadow->rhs_node == value)
        return false;

    *trans_shadow->rhs_node = value;

    CrossSearch(trans_shadow, value, true);
    emit SResizeColumnToContents(std::to_underlying(TransEnumO::kUnitPrice));
    emit SResizeColumnToContents(std::to_underlying(TransEnumO::kOutsideProduct));

    return true;
}

bool TransModelO::UpdateOutsideProduct(TransShadow* trans_shadow, int value)
{
    if (*trans_shadow->support_id == value)
        return false;

    int old_rhs_node { *trans_shadow->rhs_node };

    *trans_shadow->support_id = value;
    CrossSearch(trans_shadow, value, false);

    if (old_rhs_node) {
        sql_->WriteField(info_.trans, kOutsideProduct, value, *trans_shadow->id);
    }

    emit SResizeColumnToContents(std::to_underlying(TransEnumO::kUnitPrice));
    emit SResizeColumnToContents(std::to_underlying(TransEnumO::kInsideProduct));

    bool ins_changed { *trans_shadow->rhs_node != old_rhs_node };
    return ins_changed;
}

bool TransModelO::UpdateUnitPrice(TransShadow* trans_shadow, double value)
{
    if (std::abs(*trans_shadow->lhs_ratio - value) < kTolerance)
        return false;

    double delta { *trans_shadow->lhs_credit * (value - *trans_shadow->lhs_ratio) };
    *trans_shadow->rhs_credit += delta;
    *trans_shadow->rhs_debit += delta;
    *trans_shadow->lhs_ratio = value;

    emit SResizeColumnToContents(std::to_underlying(TransEnumO::kGrossAmount));
    emit SResizeColumnToContents(std::to_underlying(TransEnumO::kNetAmount));

    if (*trans_shadow->lhs_node == 0 || *trans_shadow->rhs_node == 0)
        return true;

    sql_->WriteField(info_.trans, kUnitPrice, value, *trans_shadow->id);
    sql_->SyncTransValue(trans_shadow);
    return true;
}

bool TransModelO::UpdateDiscountPrice(TransShadow* trans_shadow, double value)
{
    if (std::abs(*trans_shadow->rhs_ratio - value) < kTolerance)
        return false;

    double delta { *trans_shadow->lhs_credit * (value - *trans_shadow->rhs_ratio) };
    *trans_shadow->rhs_credit -= delta;
    *trans_shadow->discount += delta;
    *trans_shadow->rhs_ratio = value;

    emit SResizeColumnToContents(std::to_underlying(TransEnumO::kDiscount));
    emit SResizeColumnToContents(std::to_underlying(TransEnumO::kNetAmount));

    if (*trans_shadow->lhs_node == 0 || *trans_shadow->rhs_node == 0)
        return true;

    sql_->WriteField(info_.trans, kDiscountPrice, value, *trans_shadow->id);
    sql_->SyncTransValue(trans_shadow);
    return true;
}

bool TransModelO::UpdateSecond(TransShadow* trans_shadow, double value, int kCoefficient)
{
    if (std::abs(*trans_shadow->lhs_credit - value) < kTolerance)
        return false;

    double delta { value * kCoefficient - *trans_shadow->lhs_credit };
    *trans_shadow->rhs_debit += *trans_shadow->lhs_ratio * delta;
    *trans_shadow->discount += *trans_shadow->rhs_ratio * delta;
    *trans_shadow->rhs_credit += (*trans_shadow->lhs_ratio - *trans_shadow->rhs_ratio) * delta;

    *trans_shadow->lhs_credit = value * kCoefficient;

    emit SResizeColumnToContents(std::to_underlying(TransEnumO::kGrossAmount));
    emit SResizeColumnToContents(std::to_underlying(TransEnumO::kDiscount));
    emit SResizeColumnToContents(std::to_underlying(TransEnumO::kNetAmount));

    if (*trans_shadow->lhs_node == 0 || *trans_shadow->rhs_node == 0)
        // Return without writing data to SQLite
        return true;

    sql_->SyncTransValue(trans_shadow);
    return true;
}

bool TransModelO::UpdateFirst(TransShadow* trans_shadow, double value, int kCoefficient)
{
    if (std::abs(*trans_shadow->lhs_debit - value) < kTolerance)
        return false;

    TransModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kFirst, value * kCoefficient, &TransShadow::lhs_debit);
    return true;
}

void TransModelO::PurifyTransShadow(int lhs_node_id)
{
    for (auto i = trans_shadow_list_.size() - 1; i >= 0; --i) {
        auto* trans_shadow { trans_shadow_list_[i] };

        if (*trans_shadow->rhs_node == 0) {
            beginRemoveRows(QModelIndex(), i, i);
            ResourcePool<TransShadow>::Instance().Recycle(trans_shadow_list_.takeAt(i));
            endRemoveRows();
        } else if (lhs_node_id != 0) {
            *trans_shadow->lhs_node = lhs_node_id;
        }
    }
}

void TransModelO::CrossSearch(TransShadow* trans_shadow, int product_id, bool is_inside) const
{
    if (!trans_shadow || !sqlite_stakeholder_ || product_id <= 0)
        return;

    if (sqlite_stakeholder_->CrossSearch(trans_shadow, party_id_, product_id, is_inside))
        return;

    *trans_shadow->lhs_ratio = is_inside ? product_tree_->First(product_id) : 0.0;
    is_inside ? * trans_shadow->support_id = 0 : * trans_shadow->rhs_node = 0;
}
