#include "tablemodelproduct.h"

#include "component/constvalue.h"
#include "global/resourcepool.h"
#include "tablemodelutils.h"

TableModelProduct::TableModelProduct(Sqlite* sql, bool rule, int node_id, CInfo& info, QObject* parent)
    : TableModel { sql, rule, node_id, info, parent }
{
    if (node_id >= 1)
        sql_->ReadNodeTrans(trans_shadow_list_, node_id);
}

QVariant TableModelProduct::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* trans_shadow { trans_shadow_list_.at(index.row()) };
    const TableEnumProduct kColumn { index.column() };

    switch (kColumn) {
    case TableEnumProduct::kID:
        return *trans_shadow->id;
    case TableEnumProduct::kDateTime:
        return *trans_shadow->date_time;
    case TableEnumProduct::kCode:
        return *trans_shadow->code;
    case TableEnumProduct::kUnitCost:
        return *trans_shadow->unit_price == 0 ? QVariant() : *trans_shadow->unit_price;
    case TableEnumProduct::kDescription:
        return *trans_shadow->description;
    case TableEnumProduct::kSupportID:
        return *trans_shadow->support_id == 0 ? QVariant() : *trans_shadow->support_id;
    case TableEnumProduct::kRhsNode:
        return *trans_shadow->rhs_node == 0 ? QVariant() : *trans_shadow->rhs_node;
    case TableEnumProduct::kState:
        return *trans_shadow->state ? *trans_shadow->state : QVariant();
    case TableEnumProduct::kDocument:
        return trans_shadow->document->isEmpty() ? QVariant() : trans_shadow->document->size();
    case TableEnumProduct::kDebit:
        return *trans_shadow->lhs_debit == 0 ? QVariant() : *trans_shadow->lhs_debit;
    case TableEnumProduct::kCredit:
        return *trans_shadow->lhs_credit == 0 ? QVariant() : *trans_shadow->lhs_credit;
    case TableEnumProduct::kSubtotal:
        return trans_shadow->subtotal;
    default:
        return QVariant();
    }
}

bool TableModelProduct::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    const TableEnumProduct kColumn { index.column() };
    const int kRow { index.row() };

    auto* trans_shadow { trans_shadow_list_.at(kRow) };
    int old_rhs_node { *trans_shadow->rhs_node };
    int old_hel_node { *trans_shadow->support_id };

    bool rhs_changed { false };
    bool deb_changed { false };
    bool cre_changed { false };
    bool rat_changed { false };
    bool sup_changed { false };

    switch (kColumn) {
    case TableEnumProduct::kDateTime:
        TableModelUtils::UpdateField(sql_, trans_shadow, info_.transaction, value.toString(), kDateTime, &TransShadow::date_time);
        break;
    case TableEnumProduct::kCode:
        TableModelUtils::UpdateField(sql_, trans_shadow, info_.transaction, value.toString(), kCode, &TransShadow::code);
        break;
    case TableEnumProduct::kState:
        TableModelUtils::UpdateField(sql_, trans_shadow, info_.transaction, value.toBool(), kState, &TransShadow::state);
        break;
    case TableEnumProduct::kDescription:
        TableModelUtils::UpdateField(
            sql_, trans_shadow, info_.transaction, value.toString(), kDescription, &TransShadow::description, [this]() { emit SSearch(); });
        break;
    case TableEnumProduct::kSupportID:
        sup_changed = TableModelUtils::UpdateField(sql_, trans_shadow, info_.transaction, value.toInt(), kSupportID, &TransShadow::support_id);
        break;
    case TableEnumProduct::kUnitCost:
        rat_changed = UpdateRatio(trans_shadow, value.toDouble());
        break;
    case TableEnumProduct::kRhsNode:
        rhs_changed = TableModelUtils::UpdateRhsNode(trans_shadow, value.toInt());
        break;
    case TableEnumProduct::kDebit:
        deb_changed = UpdateDebit(trans_shadow, value.toDouble());
        break;
    case TableEnumProduct::kCredit:
        cre_changed = UpdateCredit(trans_shadow, value.toDouble());
        break;
    default:
        return false;
    }

    if (old_rhs_node == 0) {
        if (rhs_changed) {
            sql_->WriteTrans(trans_shadow);
            TableModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, kRow, rule_);

            emit SResizeColumnToContents(std::to_underlying(TableEnumProduct::kSubtotal));
            emit SAppendOneTrans(info_.section, trans_shadow);

            emit SUpdateLeafValueOne(*trans_shadow->rhs_node, *trans_shadow->unit_price, kUnitCost);
            emit SUpdateLeafValueOne(node_id_, *trans_shadow->unit_price, kUnitCost);

            double ratio { *trans_shadow->lhs_ratio };
            double debit { *trans_shadow->lhs_debit };
            double credit { *trans_shadow->lhs_credit };
            emit SUpdateLeafValue(node_id_, debit, credit, ratio * debit, ratio * credit);

            ratio = *trans_shadow->rhs_ratio;
            debit = *trans_shadow->rhs_debit;
            credit = *trans_shadow->rhs_credit;
            emit SUpdateLeafValue(*trans_shadow->rhs_node, debit, credit, ratio * debit, ratio * credit);

            if (*trans_shadow->support_id != 0) {
                emit SAppendSupportTrans(info_.section, trans_shadow);
            }
        }

        emit SResizeColumnToContents(index.column());
        return true;
    }

    if (deb_changed || cre_changed || rat_changed) {
        sql_->UpdateTransValue(trans_shadow);
        emit SSearch();
        emit SUpdateBalance(info_.section, old_rhs_node, *trans_shadow->id);
    }

    if (sup_changed) {
        if (old_hel_node != 0)
            emit SRemoveSupportTrans(info_.section, old_hel_node, *trans_shadow->id);

        if (*trans_shadow->support_id != 0) {
            emit SAppendSupportTrans(info_.section, trans_shadow);
        }
    }

    if (deb_changed || cre_changed) {
        TableModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, kRow, rule_);
        emit SResizeColumnToContents(std::to_underlying(TableEnumProduct::kSubtotal));
    }

    if (rhs_changed) {
        sql_->UpdateTransValue(trans_shadow);
        emit SRemoveOneTrans(info_.section, old_rhs_node, *trans_shadow->id);
        emit SAppendOneTrans(info_.section, trans_shadow);

        double ratio { *trans_shadow->rhs_ratio };
        double debit { *trans_shadow->rhs_debit };
        double credit { *trans_shadow->rhs_credit };
        emit SUpdateLeafValue(*trans_shadow->rhs_node, debit, credit, ratio * debit, ratio * credit);
        emit SUpdateLeafValue(old_rhs_node, -debit, -credit, -ratio * debit, -ratio * credit);
    }

    emit SResizeColumnToContents(index.column());
    return true;
}

void TableModelProduct::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.table_header.size() - 1)
        return;

    auto Compare = [column, order](TransShadow* lhs, TransShadow* rhs) -> bool {
        const TableEnumProduct kColumn { column };

        switch (kColumn) {
        case TableEnumProduct::kDateTime:
            return (order == Qt::AscendingOrder) ? (*lhs->date_time < *rhs->date_time) : (*lhs->date_time > *rhs->date_time);
        case TableEnumProduct::kCode:
            return (order == Qt::AscendingOrder) ? (*lhs->code < *rhs->code) : (*lhs->code > *rhs->code);
        case TableEnumProduct::kUnitCost:
            return (order == Qt::AscendingOrder) ? (*lhs->unit_price < *rhs->unit_price) : (*lhs->unit_price > *rhs->unit_price);
        case TableEnumProduct::kDescription:
            return (order == Qt::AscendingOrder) ? (*lhs->description < *rhs->description) : (*lhs->description > *rhs->description);
        case TableEnumProduct::kSupportID:
            return (order == Qt::AscendingOrder) ? (*lhs->support_id < *rhs->support_id) : (*lhs->support_id > *rhs->support_id);
        case TableEnumProduct::kRhsNode:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_node < *rhs->rhs_node) : (*lhs->rhs_node > *rhs->rhs_node);
        case TableEnumProduct::kState:
            return (order == Qt::AscendingOrder) ? (*lhs->state < *rhs->state) : (*lhs->state > *rhs->state);
        case TableEnumProduct::kDocument:
            return (order == Qt::AscendingOrder) ? (lhs->document->size() < rhs->document->size()) : (lhs->document->size() > rhs->document->size());
        case TableEnumProduct::kDebit:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_debit < *rhs->lhs_debit) : (*lhs->lhs_debit > *rhs->lhs_debit);
        case TableEnumProduct::kCredit:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_credit < *rhs->lhs_credit) : (*lhs->lhs_credit > *rhs->lhs_credit);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(trans_shadow_list_.begin(), trans_shadow_list_.end(), Compare);
    emit layoutChanged();

    TableModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, 0, rule_);
}

Qt::ItemFlags TableModelProduct::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const TableEnumProduct kColumn { index.column() };

    switch (kColumn) {
    case TableEnumProduct::kID:
    case TableEnumProduct::kSubtotal:
    case TableEnumProduct::kDocument:
    case TableEnumProduct::kState:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}

bool TableModelProduct::removeRows(int row, int /*count*/, const QModelIndex& parent)
{
    if (row <= -1)
        return false;

    auto* trans_shadow { trans_shadow_list_.at(row) };
    int rhs_node_id { *trans_shadow->rhs_node };

    beginRemoveRows(parent, row, row);
    trans_shadow_list_.removeAt(row);
    endRemoveRows();

    if (rhs_node_id != 0) {
        double unit_cost { *trans_shadow->unit_price };
        double debit { *trans_shadow->lhs_debit };
        double credit { *trans_shadow->lhs_credit };
        emit SUpdateLeafValue(node_id_, -debit, -credit, -unit_cost * debit, -unit_cost * credit);

        debit = *trans_shadow->rhs_debit;
        credit = *trans_shadow->rhs_credit;
        emit SUpdateLeafValue(*trans_shadow->rhs_node, -debit, -credit, -unit_cost * debit, -unit_cost * credit);

        int trans_id { *trans_shadow->id };
        emit SRemoveOneTrans(info_.section, rhs_node_id, trans_id);
        TableModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, row, rule_);

        if (int support_id = *trans_shadow->support_id; support_id != 0)
            emit SRemoveSupportTrans(info_.section, support_id, *trans_shadow->id);

        sql_->RemoveTrans(trans_id);
    }

    ResourcePool<TransShadow>::Instance().Recycle(trans_shadow);
    return true;
}

bool TableModelProduct::UpdateDebit(TransShadow* trans_shadow, double value)
{
    double lhs_debit { *trans_shadow->lhs_debit };
    if (std::abs(lhs_debit - value) < kTolerance)
        return false;

    double lhs_credit { *trans_shadow->lhs_credit };

    double abs { qAbs(value - lhs_credit) };
    *trans_shadow->lhs_debit = (value > lhs_credit) ? abs : 0;
    *trans_shadow->lhs_credit = (value <= lhs_credit) ? abs : 0;

    *trans_shadow->rhs_debit = *trans_shadow->lhs_credit;
    *trans_shadow->rhs_credit = *trans_shadow->lhs_debit;

    if (*trans_shadow->rhs_node == 0)
        return false;

    double unit_cost { *trans_shadow->unit_price };
    double quantity_debit_diff { *trans_shadow->lhs_debit - lhs_debit };
    double quantity_credit_diff { *trans_shadow->lhs_credit - lhs_credit };
    double amount_debit_diff { quantity_debit_diff * unit_cost };
    double amount_credit_diff { quantity_credit_diff * unit_cost };

    emit SUpdateLeafValue(node_id_, quantity_debit_diff, quantity_credit_diff, amount_debit_diff, amount_credit_diff);
    emit SUpdateLeafValue(*trans_shadow->rhs_node, quantity_credit_diff, quantity_debit_diff, amount_credit_diff, amount_debit_diff);

    return true;
}

bool TableModelProduct::UpdateCredit(TransShadow* trans_shadow, double value)
{
    double lhs_credit { *trans_shadow->lhs_credit };
    if (std::abs(lhs_credit - value) < kTolerance)
        return false;

    double lhs_debit { *trans_shadow->lhs_debit };

    double abs { qAbs(value - lhs_debit) };
    *trans_shadow->lhs_debit = (value > lhs_debit) ? 0 : abs;
    *trans_shadow->lhs_credit = (value <= lhs_debit) ? 0 : abs;

    *trans_shadow->rhs_debit = *trans_shadow->lhs_credit;
    *trans_shadow->rhs_credit = *trans_shadow->lhs_debit;

    if (*trans_shadow->rhs_node == 0)
        return false;

    double unit_cost { *trans_shadow->unit_price };
    double quantity_debit_diff { *trans_shadow->lhs_debit - lhs_debit };
    double quantity_credit_diff { *trans_shadow->lhs_credit - lhs_credit };
    double amount_debit_diff { quantity_debit_diff * unit_cost };
    double amount_credit_diff { quantity_credit_diff * unit_cost };

    emit SUpdateLeafValue(node_id_, quantity_debit_diff, quantity_credit_diff, amount_debit_diff, amount_credit_diff);
    emit SUpdateLeafValue(*trans_shadow->rhs_node, quantity_credit_diff, quantity_debit_diff, amount_credit_diff, amount_debit_diff);

    return true;
}

bool TableModelProduct::UpdateRatio(TransShadow* trans_shadow, double value)
{
    double unit_cost { *trans_shadow->unit_price };
    if (std::abs(unit_cost - value) < kTolerance || value < 0)
        return false;

    double diff { value - unit_cost };
    *trans_shadow->unit_price = value;

    if (*trans_shadow->rhs_node == 0)
        return false;

    sql_->UpdateField(info_.transaction, value, kUnitCost, *trans_shadow->id);

    emit SUpdateLeafValue(node_id_, 0, 0, *trans_shadow->lhs_debit * diff, *trans_shadow->lhs_credit * diff);
    emit SUpdateLeafValue(*trans_shadow->rhs_node, 0, 0, *trans_shadow->rhs_debit * diff, *trans_shadow->rhs_credit * diff);

    return true;
}
