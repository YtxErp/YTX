#include "tablemodelorder.h"

#include "global/resourcepool.h"

TableModelOrder::TableModelOrder(
    Sqlite* sql, bool rule, int node_id, CInfo& info, const NodeShadow* node_shadow, CTreeModel* product_tree, Sqlite* sqlite_stakeholder, QObject* parent)
    : TableModel { sql, rule, node_id, info, parent }
    , product_tree_ { static_cast<const TreeModelProduct*>(product_tree) }
    , sqlite_stakeholder_ { static_cast<SqliteStakeholder*>(sqlite_stakeholder) }
    , node_shadow_ { node_shadow }
    , party_id_ { *node_shadow->party }
{
    if (node_id >= 1)
        sql_->ReadNodeTrans(trans_shadow_list_, node_id);

    if (party_id_ >= 1)
        sqlite_stakeholder_->ReadTrans(party_id_);
}

void TableModelOrder::RUpdateNodeID(int node_id)
{
    if (node_id_ != 0 || node_id <= 0)
        return;

    node_id_ = node_id;
    if (trans_shadow_list_.isEmpty())
        return;

    TransShadow* trans_shadow {};
    double first_diff {};
    double second_diff {};
    double amount_diff {};
    double discount_diff {};
    double settled_diff {};

    for (auto i { trans_shadow_list_.size() - 1 }; i >= 0; --i) {
        trans_shadow = trans_shadow_list_.at(i);
        if (*trans_shadow->rhs_node == 0) {
            beginRemoveRows(QModelIndex(), i, i);
            ResourcePool<TransShadow>::Instance().Recycle(trans_shadow_list_.takeAt(i));
            endRemoveRows();
        } else {
            *trans_shadow->lhs_node = node_id;

            first_diff += *trans_shadow->lhs_debit;
            second_diff += *trans_shadow->lhs_credit;
            amount_diff += *trans_shadow->rhs_credit;
            discount_diff += *trans_shadow->rhs_debit;
            settled_diff += *trans_shadow->settled;
        }
    }

    // 一次向数据库添加多条交易
    if (!trans_shadow_list_.isEmpty())
        sql_->WriteTransRangeO(trans_shadow_list_);

    emit SUpdateLeafValue(node_id, first_diff, second_diff, amount_diff, discount_diff, settled_diff);
}

void TableModelOrder::RUpdateFinished(int node_id, bool checked)
{
    if (node_id != node_id_ || !checked)
        return;

    TransShadow* trans_shadow {};
    for (auto i { trans_shadow_list_.size() - 1 }; i >= 0; --i) {
        trans_shadow = trans_shadow_list_.at(i);
        if (*trans_shadow->rhs_node == 0) {
            beginRemoveRows(QModelIndex(), i, i);
            ResourcePool<TransShadow>::Instance().Recycle(trans_shadow_list_.takeAt(i));
            endRemoveRows();
        }
    }

    // 遍历trans_shadow_list，对比exclusive_price_，检测是否存在inside_product_id, 不存在添加，存在更新
    for (auto it = update_price_.cbegin(); it != update_price_.cend(); ++it) {
        sqlite_stakeholder_->UpdatePrice(*node_shadow_->party, it.key(), *node_shadow_->date_time, it.value());
    }

    update_price_.clear();
}

void TableModelOrder::RUpdateParty(int node_id, int party_id)
{
    if (node_id_ != node_id)
        return;

    party_id_ = party_id;
    sqlite_stakeholder_->ReadTrans(party_id);
}

QVariant TableModelOrder::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* trans_shadow { trans_shadow_list_.at(index.row()) };
    const TableEnumOrder kColumn { index.column() };

    switch (kColumn) {
    case TableEnumOrder::kID:
        return *trans_shadow->id;
    case TableEnumOrder::kCode:
        return *trans_shadow->code;
    case TableEnumOrder::kInsideProduct:
        return *trans_shadow->rhs_node == 0 ? QVariant() : *trans_shadow->rhs_node;
    case TableEnumOrder::kUnitPrice:
        return *trans_shadow->unit_price == 0 ? QVariant() : *trans_shadow->unit_price;
    case TableEnumOrder::kSecond:
        return *trans_shadow->lhs_credit == 0 ? QVariant() : *trans_shadow->lhs_credit;
    case TableEnumOrder::kDescription:
        return *trans_shadow->description;
    case TableEnumOrder::kColor:
        return *trans_shadow->rhs_node == 0 ? QVariant() : product_tree_->Color(*trans_shadow->rhs_node);
    case TableEnumOrder::kFirst:
        return *trans_shadow->lhs_debit == 0 ? QVariant() : *trans_shadow->lhs_debit;
    case TableEnumOrder::kAmount:
        return *trans_shadow->rhs_credit == 0 ? QVariant() : *trans_shadow->rhs_credit;
    case TableEnumOrder::kSettled:
        return *trans_shadow->settled == 0 ? QVariant() : *trans_shadow->settled;
    case TableEnumOrder::kDiscount:
        return *trans_shadow->rhs_debit == 0 ? QVariant() : *trans_shadow->rhs_debit;
    case TableEnumOrder::kDiscountPrice:
        return *trans_shadow->discount_price == 0 ? QVariant() : *trans_shadow->discount_price;
    case TableEnumOrder::kOutsideProduct:
        return *trans_shadow->support_id == 0 ? QVariant() : *trans_shadow->support_id;
    default:
        return QVariant();
    }
}

bool TableModelOrder::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    const TableEnumOrder kColumn { index.column() };
    const int kRow { index.row() };

    auto* trans_shadow { trans_shadow_list_.at(kRow) };
    const int old_rhs_node { *trans_shadow->rhs_node };
    const double old_first { *trans_shadow->lhs_debit };
    const double old_second { *trans_shadow->lhs_credit };
    const double old_discount { *trans_shadow->rhs_debit };
    const double old_amount { *trans_shadow->rhs_credit };
    const double old_settled { *trans_shadow->settled };

    bool ins_changed { false };
    bool fir_changed { false };
    bool sec_changed { false };
    bool uni_changed { false };
    bool dis_changed { false };

    switch (kColumn) {
    case TableEnumOrder::kCode:
        UpdateField(trans_shadow, value.toString(), kCode, &TransShadow::code);
        break;
    case TableEnumOrder::kDescription:
        UpdateField(trans_shadow, value.toString(), kDescription, &TransShadow::description);
        break;
    case TableEnumOrder::kInsideProduct:
        ins_changed = UpdateInsideProduct(trans_shadow, value.toInt());
        break;
    case TableEnumOrder::kUnitPrice:
        uni_changed = UpdateUnitPrice(trans_shadow, value.toDouble());
        break;
    case TableEnumOrder::kSecond:
        sec_changed = UpdateSecond(trans_shadow, value.toDouble());
        break;
    case TableEnumOrder::kFirst:
        fir_changed = UpdateField(trans_shadow, value.toDouble(), kFirst, &TransShadow::lhs_debit);
        break;
    case TableEnumOrder::kDiscountPrice:
        dis_changed = UpdateDiscountPrice(trans_shadow, value.toDouble());
        break;
    case TableEnumOrder::kOutsideProduct:
        ins_changed = UpdateOutsideProduct(trans_shadow, value.toInt());
        break;
    default:
        return false;
    }

    emit SResizeColumnToContents(index.column());

    if (node_id_ == 0) {
        return false;
    }

    if (ins_changed) {
        if (old_rhs_node == 0) {
            sql_->WriteTrans(trans_shadow);
            emit SUpdateLeafValue(*trans_shadow->lhs_node, *trans_shadow->lhs_debit, *trans_shadow->lhs_credit, *trans_shadow->rhs_credit,
                *trans_shadow->rhs_debit, *trans_shadow->settled);
        } else
            sql_->UpdateField(info_.transaction, value.toInt(), kInsideProduct, *trans_shadow->id);
    }

    if (fir_changed)
        emit SUpdateLeafValueOne(*trans_shadow->lhs_node, value.toDouble() - old_first, kFirst);

    if (sec_changed) {
        double second_diff { value.toDouble() - old_second };
        double amount_diff { *trans_shadow->rhs_credit - old_amount };
        double discount_diff { *trans_shadow->rhs_debit - old_discount };
        double settled_diff { *trans_shadow->settled - old_settled };
        emit SUpdateLeafValue(*trans_shadow->lhs_node, 0.0, second_diff, amount_diff, discount_diff, settled_diff);
    }

    if (uni_changed) {
        double amount_diff { *trans_shadow->rhs_credit - old_amount };
        double settled_diff { *trans_shadow->settled - old_settled };
        emit SUpdateLeafValue(*trans_shadow->lhs_node, 0.0, 0.0, amount_diff, 0.0, settled_diff);
    }

    if (dis_changed) {
        double discount_diff { *trans_shadow->rhs_debit - old_discount };
        double settled_diff { *trans_shadow->settled - old_settled };
        emit SUpdateLeafValue(*trans_shadow->lhs_node, 0.0, 0.0, 0.0, discount_diff, settled_diff);
    }

    return true;
}

void TableModelOrder::sort(int column, Qt::SortOrder order)
{
    // ignore subtotal column
    if (column <= -1 || column >= info_.table_header.size() - 1)
        return;

    auto Compare = [column, order](TransShadow* lhs, TransShadow* rhs) -> bool {
        const TableEnumOrder kColumn { column };

        switch (kColumn) {
        case TableEnumOrder::kCode:
            return (order == Qt::AscendingOrder) ? (*lhs->code < *rhs->code) : (*lhs->code > *rhs->code);
        case TableEnumOrder::kInsideProduct:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_node < *rhs->rhs_node) : (*lhs->rhs_node > *rhs->rhs_node);
        case TableEnumOrder::kUnitPrice:
            return (order == Qt::AscendingOrder) ? (*lhs->unit_price < *rhs->unit_price) : (*lhs->unit_price > *rhs->unit_price);
        case TableEnumOrder::kFirst:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_debit < *rhs->lhs_debit) : (*lhs->lhs_debit > *rhs->lhs_debit);
        case TableEnumOrder::kSecond:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_credit < *rhs->lhs_credit) : (*lhs->lhs_credit > *rhs->lhs_credit);
        case TableEnumOrder::kAmount:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_credit < *rhs->rhs_credit) : (*lhs->rhs_credit > *rhs->rhs_credit);
        case TableEnumOrder::kDiscount:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_debit < *rhs->rhs_debit) : (*lhs->rhs_debit > *rhs->rhs_debit);
        case TableEnumOrder::kDiscountPrice:
            return (order == Qt::AscendingOrder) ? (*lhs->discount_price < *rhs->discount_price) : (*lhs->discount_price > *rhs->discount_price);
        case TableEnumOrder::kOutsideProduct:
            return (order == Qt::AscendingOrder) ? (*lhs->support_id < *rhs->support_id) : (*lhs->support_id > *rhs->support_id);
        case TableEnumOrder::kSettled:
            return (order == Qt::AscendingOrder) ? (*lhs->settled < *rhs->settled) : (*lhs->settled > *rhs->settled);
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
    const TableEnumOrder kColumn { index.column() };

    switch (kColumn) {
    case TableEnumOrder::kID:
    case TableEnumOrder::kAmount:
    case TableEnumOrder::kDiscount:
    case TableEnumOrder::kSettled:
    case TableEnumOrder::kColor:
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
    emit SResizeColumnToContents(std::to_underlying(TableEnumOrder::kUnitPrice));
    emit SResizeColumnToContents(std::to_underlying(TableEnumOrder::kOutsideProduct));

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
        sql_->UpdateField(info_.transaction, value, kOutsideProduct, *trans_shadow->id);
    }

    emit SResizeColumnToContents(std::to_underlying(TableEnumOrder::kUnitPrice));
    emit SResizeColumnToContents(std::to_underlying(TableEnumOrder::kInsideProduct));

    bool ins_changed { *trans_shadow->rhs_node != old_rhs_node };
    return ins_changed;
}

bool TableModelOrder::UpdateUnitPrice(TransShadow* trans_shadow, double value)
{
    if (std::abs(*trans_shadow->unit_price - value) < kTolerance)
        return false;

    double diff { *trans_shadow->lhs_credit * (value - *trans_shadow->unit_price) };
    *trans_shadow->rhs_credit += diff;
    *trans_shadow->settled += diff;
    *trans_shadow->unit_price = value;

    emit SResizeColumnToContents(std::to_underlying(TableEnumOrder::kAmount));
    emit SResizeColumnToContents(std::to_underlying(TableEnumOrder::kSettled));
    update_price_.insert(*trans_shadow->rhs_node, value);

    if (*trans_shadow->lhs_node == 0 || *trans_shadow->rhs_node == 0)
        return false;

    sql_->UpdateField(info_.transaction, value, kUnitPrice, *trans_shadow->id);
    sql_->UpdateTransValue(trans_shadow);
    return true;
}

bool TableModelOrder::UpdateDiscountPrice(TransShadow* trans_shadow, double value)
{
    if (std::abs(*trans_shadow->discount_price - value) < kTolerance)
        return false;

    double diff { *trans_shadow->lhs_credit * (value - *trans_shadow->discount_price) };
    *trans_shadow->rhs_debit += diff;
    *trans_shadow->settled -= diff;
    *trans_shadow->discount_price = value;

    emit SResizeColumnToContents(std::to_underlying(TableEnumOrder::kDiscount));
    emit SResizeColumnToContents(std::to_underlying(TableEnumOrder::kSettled));

    if (*trans_shadow->lhs_node == 0 || *trans_shadow->rhs_node == 0)
        return false;

    sql_->UpdateField(info_.transaction, value, kDiscountPrice, *trans_shadow->id);
    sql_->UpdateTransValue(trans_shadow);
    return true;
}

bool TableModelOrder::UpdateSecond(TransShadow* trans_shadow, double value)
{
    if (std::abs(*trans_shadow->lhs_credit - value) < kTolerance)
        return false;

    double diff { value - *trans_shadow->lhs_credit };
    *trans_shadow->rhs_credit += *trans_shadow->unit_price * diff;
    *trans_shadow->rhs_debit += *trans_shadow->discount_price * diff;
    *trans_shadow->settled += (*trans_shadow->unit_price - *trans_shadow->discount_price) * diff;

    *trans_shadow->lhs_credit = value;

    emit SResizeColumnToContents(std::to_underlying(TableEnumOrder::kAmount));
    emit SResizeColumnToContents(std::to_underlying(TableEnumOrder::kDiscount));
    emit SResizeColumnToContents(std::to_underlying(TableEnumOrder::kSettled));

    if (*trans_shadow->lhs_node == 0 || *trans_shadow->rhs_node == 0)
        return false;

    sql_->UpdateTransValue(trans_shadow);
    return true;
}

void TableModelOrder::CrossSearch(TransShadow* trans_shadow, int product_id, bool is_inside) const
{
    if (!trans_shadow || !sqlite_stakeholder_ || product_id <= 0)
        return;

    if (sqlite_stakeholder_->CrossSearch(trans_shadow, *node_shadow_->party, product_id, is_inside))
        return;

    *trans_shadow->unit_price = is_inside ? product_tree_->First(product_id) : 0.0;
    is_inside ? * trans_shadow->support_id = 0 : * trans_shadow->rhs_node = 0;
}
