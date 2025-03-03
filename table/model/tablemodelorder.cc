#include "tablemodelorder.h"

#include "global/resourcepool.h"
#include "tablemodelutils.h"

TableModelOrder::TableModelOrder(
    Sqlite* sql, bool rule, int node_id, CInfo& info, const Node* node, CTreeModel* product_tree, Sqlite* sqlite_stakeholder, QObject* parent)
    : TableModel { sql, rule, node_id, info, parent }
    , product_tree_ { static_cast<const TreeModelProduct*>(product_tree) }
    , sqlite_stakeholder_ { static_cast<SqliteStakeholder*>(sqlite_stakeholder) }
    , node_ { node }
    , party_id_ { node->party }
{
    if (node_id >= 1)
        sql_->ReadTrans(trans_shadow_list_, node_id);

    if (party_id_ >= 1)
        sqlite_stakeholder_->ReadTrans(party_id_);
}

TableModelOrder::~TableModelOrder()
{
    if (node_id_ != 0) {
        RSyncBool(node_id_, 0, true);
    }
}

void TableModelOrder::UpdateLhsNode(int node_id)
{
    if (node_id_ != 0 || node_id <= 0)
        return;

    node_id_ = node_id;
    if (trans_shadow_list_.isEmpty())
        return;

    PurifyTransShadow(node_id);

    if (!trans_shadow_list_.isEmpty())
        sql_->WriteTransRange(trans_shadow_list_);
}

void TableModelOrder::UpdateParty(int node_id, int party_id)
{
    if (node_id_ != node_id || party_id_ == party_id)
        return;

    party_id_ = party_id;
    sqlite_stakeholder_->ReadTrans(party_id);
}

void TableModelOrder::UpdatePrice()
{
    // 遍历trans_shadow_list，对比exclusive_price_，检测是否存在inside_product_id, 不存在添加，存在更新
    for (auto it = sync_price_.cbegin(); it != sync_price_.cend(); ++it) {
        sqlite_stakeholder_->UpdatePrice(party_id_, it.key(), node_->date_time, it.value());
    }

    sync_price_.clear();
}

void TableModelOrder::RSyncBool(int node_id, int /*column*/, bool value)
{
    if (node_id != node_id_ || !value || sync_price_.isEmpty())
        return;

    PurifyTransShadow();
    UpdatePrice();
}

void TableModelOrder::RSyncInt(int node_id, int column, int value)
{
    const TreeEnumO kColumn { column };

    switch (kColumn) {
    case TreeEnumO::kID:
        UpdateLhsNode(node_id);
        break;
    case TreeEnumO::kParty:
        UpdateParty(node_id, value);
        break;
    default:
        break;
    }
}

QVariant TableModelOrder::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* trans_shadow { trans_shadow_list_.at(index.row()) };
    const TableEnumO kColumn { index.column() };

    switch (kColumn) {
    case TableEnumO::kID:
        return *trans_shadow->id;
    case TableEnumO::kCode:
        return *trans_shadow->code;
    case TableEnumO::kInsideProduct:
        return *trans_shadow->rhs_node == 0 ? QVariant() : *trans_shadow->rhs_node;
    case TableEnumO::kUnitPrice:
        return *trans_shadow->lhs_ratio == 0 ? QVariant() : *trans_shadow->lhs_ratio;
    case TableEnumO::kSecond:
        return *trans_shadow->lhs_credit == 0 ? QVariant() : *trans_shadow->lhs_credit;
    case TableEnumO::kDescription:
        return *trans_shadow->description;
    case TableEnumO::kColor:
        return *trans_shadow->rhs_node == 0 ? QVariant() : product_tree_->Color(*trans_shadow->rhs_node);
    case TableEnumO::kFirst:
        return *trans_shadow->lhs_debit == 0 ? QVariant() : *trans_shadow->lhs_debit;
    case TableEnumO::kNetAmount:
        return *trans_shadow->rhs_credit == 0 ? QVariant() : *trans_shadow->rhs_credit;
    case TableEnumO::kDiscount:
        return *trans_shadow->discount == 0 ? QVariant() : *trans_shadow->discount;
    case TableEnumO::kGrossAmount:
        return *trans_shadow->rhs_debit == 0 ? QVariant() : *trans_shadow->rhs_debit;
    case TableEnumO::kDiscountPrice:
        return *trans_shadow->rhs_ratio == 0 ? QVariant() : *trans_shadow->rhs_ratio;
    case TableEnumO::kOutsideProduct:
        return *trans_shadow->support_id == 0 ? QVariant() : *trans_shadow->support_id;
    default:
        return QVariant();
    }
}

bool TableModelOrder::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    const TableEnumO kColumn { index.column() };
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

    switch (kColumn) {
    case TableEnumO::kCode:
        TableModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kCode, value.toString(), &TransShadow::code);
        break;
    case TableEnumO::kDescription:
        TableModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kDescription, value.toString(), &TransShadow::description);
        break;
    case TableEnumO::kInsideProduct:
        ins_changed = UpdateInsideProduct(trans_shadow, value.toInt());
        break;
    case TableEnumO::kUnitPrice:
        uni_changed = UpdateUnitPrice(trans_shadow, value.toDouble());
        break;
    case TableEnumO::kSecond:
        sec_changed = UpdateSecond(trans_shadow, value.toDouble());
        break;
    case TableEnumO::kFirst:
        fir_changed = TableModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kFirst, value.toDouble(), &TransShadow::lhs_debit);
        break;
    case TableEnumO::kDiscountPrice:
        dis_changed = UpdateDiscountPrice(trans_shadow, value.toDouble());
        break;
    case TableEnumO::kOutsideProduct:
        ins_changed = UpdateOutsideProduct(trans_shadow, value.toInt());
        break;
    default:
        return false;
    }

    if (fir_changed)
        emit SUpdateLeafValue(*trans_shadow->lhs_node, 0.0, 0.0, value.toDouble() - old_first);

    if (sec_changed) {
        double second_delta { value.toDouble() - old_second };
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

    emit SResizeColumnToContents(index.column());
    return true;
}

void TableModelOrder::sort(int column, Qt::SortOrder order)
{
    // ignore subtotal column
    if (column <= -1 || column >= info_.trans_header.size() - 1)
        return;

    auto Compare = [column, order](TransShadow* lhs, TransShadow* rhs) -> bool {
        const TableEnumO kColumn { column };

        switch (kColumn) {
        case TableEnumO::kCode:
            return (order == Qt::AscendingOrder) ? (*lhs->code < *rhs->code) : (*lhs->code > *rhs->code);
        case TableEnumO::kInsideProduct:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_node < *rhs->rhs_node) : (*lhs->rhs_node > *rhs->rhs_node);
        case TableEnumO::kUnitPrice:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_ratio < *rhs->lhs_ratio) : (*lhs->lhs_ratio > *rhs->lhs_ratio);
        case TableEnumO::kFirst:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_debit < *rhs->lhs_debit) : (*lhs->lhs_debit > *rhs->lhs_debit);
        case TableEnumO::kSecond:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_credit < *rhs->lhs_credit) : (*lhs->lhs_credit > *rhs->lhs_credit);
        case TableEnumO::kNetAmount:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_credit < *rhs->rhs_credit) : (*lhs->rhs_credit > *rhs->rhs_credit);
        case TableEnumO::kGrossAmount:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_debit < *rhs->rhs_debit) : (*lhs->rhs_debit > *rhs->rhs_debit);
        case TableEnumO::kDiscountPrice:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_ratio < *rhs->rhs_ratio) : (*lhs->rhs_ratio > *rhs->rhs_ratio);
        case TableEnumO::kOutsideProduct:
            return (order == Qt::AscendingOrder) ? (*lhs->support_id < *rhs->support_id) : (*lhs->support_id > *rhs->support_id);
        case TableEnumO::kDiscount:
            return (order == Qt::AscendingOrder) ? (*lhs->discount < *rhs->discount) : (*lhs->discount > *rhs->discount);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(trans_shadow_list_.begin(), trans_shadow_list_.end(), Compare);
    emit layoutChanged();
}

Qt::ItemFlags TableModelOrder::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const TableEnumO kColumn { index.column() };

    switch (kColumn) {
    case TableEnumO::kID:
    case TableEnumO::kGrossAmount:
    case TableEnumO::kDiscount:
    case TableEnumO::kNetAmount:
    case TableEnumO::kColor:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}

bool TableModelOrder::removeRows(int row, int /*count*/, const QModelIndex& parent)
{
    if (row <= -1)
        return false;

    auto* trans_shadow { trans_shadow_list_.at(row) };
    int lhs_node { *trans_shadow->lhs_node };

    beginRemoveRows(parent, row, row);
    trans_shadow_list_.removeAt(row);
    endRemoveRows();

    if (lhs_node != 0)
        sql_->RemoveTrans(*trans_shadow->id);

    ResourcePool<TransShadow>::Instance().Recycle(trans_shadow);
    return true;
}

bool TableModelOrder::UpdateInsideProduct(TransShadow* trans_shadow, int value)
{
    if (*trans_shadow->rhs_node == value)
        return false;

    *trans_shadow->rhs_node = value;

    CrossSearch(trans_shadow, value, true);
    emit SResizeColumnToContents(std::to_underlying(TableEnumO::kUnitPrice));
    emit SResizeColumnToContents(std::to_underlying(TableEnumO::kOutsideProduct));

    return true;
}

bool TableModelOrder::UpdateOutsideProduct(TransShadow* trans_shadow, int value)
{
    if (*trans_shadow->support_id == value)
        return false;

    int old_rhs_node { *trans_shadow->rhs_node };

    *trans_shadow->support_id = value;
    CrossSearch(trans_shadow, value, false);

    if (old_rhs_node) {
        sql_->WriteField(info_.trans, kOutsideProduct, value, *trans_shadow->id);
    }

    emit SResizeColumnToContents(std::to_underlying(TableEnumO::kUnitPrice));
    emit SResizeColumnToContents(std::to_underlying(TableEnumO::kInsideProduct));

    bool ins_changed { *trans_shadow->rhs_node != old_rhs_node };
    return ins_changed;
}

bool TableModelOrder::UpdateUnitPrice(TransShadow* trans_shadow, double value)
{
    if (std::abs(*trans_shadow->lhs_ratio - value) < kTolerance)
        return false;

    double delta { *trans_shadow->lhs_credit * (value - *trans_shadow->lhs_ratio) };
    *trans_shadow->rhs_credit += delta;
    *trans_shadow->rhs_debit += delta;
    *trans_shadow->lhs_ratio = value;

    emit SResizeColumnToContents(std::to_underlying(TableEnumO::kGrossAmount));
    emit SResizeColumnToContents(std::to_underlying(TableEnumO::kNetAmount));
    sync_price_.insert(*trans_shadow->rhs_node, value);

    if (*trans_shadow->lhs_node == 0 || *trans_shadow->rhs_node == 0)
        return true;

    sql_->WriteField(info_.trans, kUnitPrice, value, *trans_shadow->id);
    sql_->WriteTransValue(trans_shadow);
    return true;
}

bool TableModelOrder::UpdateDiscountPrice(TransShadow* trans_shadow, double value)
{
    if (std::abs(*trans_shadow->rhs_ratio - value) < kTolerance)
        return false;

    double delta { *trans_shadow->lhs_credit * (value - *trans_shadow->rhs_ratio) };
    *trans_shadow->rhs_credit -= delta;
    *trans_shadow->discount += delta;
    *trans_shadow->rhs_ratio = value;

    emit SResizeColumnToContents(std::to_underlying(TableEnumO::kDiscount));
    emit SResizeColumnToContents(std::to_underlying(TableEnumO::kNetAmount));

    if (*trans_shadow->lhs_node == 0 || *trans_shadow->rhs_node == 0)
        return true;

    sql_->WriteField(info_.trans, kDiscountPrice, value, *trans_shadow->id);
    sql_->WriteTransValue(trans_shadow);
    return true;
}

bool TableModelOrder::UpdateSecond(TransShadow* trans_shadow, double value)
{
    if (std::abs(*trans_shadow->lhs_credit - value) < kTolerance)
        return false;

    double delta { value - *trans_shadow->lhs_credit };
    *trans_shadow->rhs_debit += *trans_shadow->lhs_ratio * delta;
    *trans_shadow->discount += *trans_shadow->rhs_ratio * delta;
    *trans_shadow->rhs_credit += (*trans_shadow->lhs_ratio - *trans_shadow->rhs_ratio) * delta;

    *trans_shadow->lhs_credit = value;

    emit SResizeColumnToContents(std::to_underlying(TableEnumO::kGrossAmount));
    emit SResizeColumnToContents(std::to_underlying(TableEnumO::kDiscount));
    emit SResizeColumnToContents(std::to_underlying(TableEnumO::kNetAmount));

    if (*trans_shadow->lhs_node == 0 || *trans_shadow->rhs_node == 0)
        // Return without writing data to SQLite
        return true;

    sql_->WriteTransValue(trans_shadow);
    return true;
}

void TableModelOrder::PurifyTransShadow(int lhs_node_id)
{
    TransShadow* trans_shadow {};

    for (auto i { trans_shadow_list_.size() - 1 }; i >= 0; --i) {
        trans_shadow = trans_shadow_list_.at(i);
        if (*trans_shadow->rhs_node == 0) {
            beginRemoveRows(QModelIndex(), i, i);
            ResourcePool<TransShadow>::Instance().Recycle(trans_shadow_list_.takeAt(i));
            endRemoveRows();
        } else if (lhs_node_id != 0) {
            *trans_shadow->lhs_node = lhs_node_id;
        }
    }
}

void TableModelOrder::CrossSearch(TransShadow* trans_shadow, int product_id, bool is_inside) const
{
    if (!trans_shadow || !sqlite_stakeholder_ || product_id <= 0)
        return;

    if (sqlite_stakeholder_->CrossSearch(trans_shadow, party_id_, product_id, is_inside))
        return;

    *trans_shadow->lhs_ratio = is_inside ? product_tree_->First(product_id) : 0.0;
    is_inside ? * trans_shadow->support_id = 0 : * trans_shadow->rhs_node = 0;
}
