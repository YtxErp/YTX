#include "tablemodeltask.h"

#include <QTimer>

#include "component/constvalue.h"
#include "global/resourcepool.h"
#include "tablemodelutils.h"

TableModelTask::TableModelTask(Sqlite* sql, bool rule, int node_id, CInfo& info, QObject* parent)
    : TableModel { sql, rule, node_id, info, parent }
{
    if (node_id >= 1)
        sql_->ReadNodeTrans(trans_shadow_list_, node_id);
}

QVariant TableModelTask::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* trans_shadow { trans_shadow_list_.at(index.row()) };
    const TableEnumTask kColumn { index.column() };

    switch (kColumn) {
    case TableEnumTask::kID:
        return *trans_shadow->id;
    case TableEnumTask::kDateTime:
        return *trans_shadow->date_time;
    case TableEnumTask::kCode:
        return *trans_shadow->code;
    case TableEnumTask::kUnitCost:
        return *trans_shadow->lhs_ratio == 0 ? QVariant() : *trans_shadow->lhs_ratio;
    case TableEnumTask::kDescription:
        return *trans_shadow->description;
    case TableEnumTask::kSupportID:
        return *trans_shadow->support_id == 0 ? QVariant() : *trans_shadow->support_id;
    case TableEnumTask::kRhsNode:
        return *trans_shadow->rhs_node == 0 ? QVariant() : *trans_shadow->rhs_node;
    case TableEnumTask::kState:
        return *trans_shadow->state ? *trans_shadow->state : QVariant();
    case TableEnumTask::kDocument:
        return trans_shadow->document->isEmpty() ? QVariant() : trans_shadow->document->size();
    case TableEnumTask::kDebit:
        return *trans_shadow->lhs_debit == 0 ? QVariant() : *trans_shadow->lhs_debit;
    case TableEnumTask::kCredit:
        return *trans_shadow->lhs_credit == 0 ? QVariant() : *trans_shadow->lhs_credit;
    case TableEnumTask::kSubtotal:
        return trans_shadow->subtotal;
    default:
        return QVariant();
    }
}

bool TableModelTask::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    const TableEnumTask kColumn { index.column() };
    const int kRow { index.row() };

    auto* trans_shadow { trans_shadow_list_.at(kRow) };
    int old_rhs_node { *trans_shadow->rhs_node };
    int old_hel_node { *trans_shadow->support_id };

    bool rhs_changed { false };
    bool deb_changed { false };
    bool cre_changed { false };
    bool sup_changed { false };

    switch (kColumn) {
    case TableEnumTask::kDateTime:
        TableModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kDateTime, value.toString(), &TransShadow::date_time);
        break;
    case TableEnumTask::kCode:
        TableModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kCode, value.toString(), &TransShadow::code);
        break;
    case TableEnumTask::kState:
        TableModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kState, value.toBool(), &TransShadow::state);
        break;
    case TableEnumTask::kDescription:
        TableModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kDescription, value.toString(), &TransShadow::description, [this]() { emit SSearch(); });
        break;
    case TableEnumTask::kSupportID:
        sup_changed = TableModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kSupportID, value.toInt(), &TransShadow::support_id);
        break;
    case TableEnumTask::kUnitCost:
        UpdateRatio(trans_shadow, value.toDouble());
        break;
    case TableEnumTask::kRhsNode:
        rhs_changed = TableModelUtils::UpdateRhsNode(trans_shadow, value.toInt());
        break;
    case TableEnumTask::kDebit:
        deb_changed = UpdateDebit(trans_shadow, value.toDouble());
        break;
    case TableEnumTask::kCredit:
        cre_changed = UpdateCredit(trans_shadow, value.toDouble());
        break;
    default:
        return false;
    }

    if (old_rhs_node == 0 && rhs_changed) {
        sql_->WriteTrans(trans_shadow);
        TableModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, kRow, rule_);

        emit SResizeColumnToContents(std::to_underlying(TableEnumTask::kSubtotal));
        emit SAppendOneTrans(info_.section, trans_shadow);

        emit SSyncDouble(*trans_shadow->rhs_node, std::to_underlying(TableEnumTask::kUnitCost), *trans_shadow->lhs_ratio);
        emit SSyncDouble(node_id_, std::to_underlying(TableEnumTask::kUnitCost), *trans_shadow->lhs_ratio);

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

    if (deb_changed || cre_changed) {
        sql_->UpdateTransValue(trans_shadow);
        TableModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, kRow, rule_);

        emit SSearch();
        emit SUpdateBalance(info_.section, old_rhs_node, *trans_shadow->id);
        emit SResizeColumnToContents(std::to_underlying(TableEnumTask::kSubtotal));
    }

    if (sup_changed) {
        if (old_hel_node != 0)
            emit SRemoveSupportTrans(info_.section, old_hel_node, *trans_shadow->id);

        if (*trans_shadow->support_id != 0) {
            emit SAppendSupportTrans(info_.section, trans_shadow);
        }
    }

    if (old_rhs_node != 0 && rhs_changed) {
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

void TableModelTask::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.trans_header.size() - 1)
        return;

    auto Compare = [column, order](TransShadow* lhs, TransShadow* rhs) -> bool {
        const TableEnumTask kColumn { column };

        switch (kColumn) {
        case TableEnumTask::kDateTime:
            return (order == Qt::AscendingOrder) ? (*lhs->date_time < *rhs->date_time) : (*lhs->date_time > *rhs->date_time);
        case TableEnumTask::kCode:
            return (order == Qt::AscendingOrder) ? (*lhs->code < *rhs->code) : (*lhs->code > *rhs->code);
        case TableEnumTask::kUnitCost:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_ratio < *rhs->lhs_ratio) : (*lhs->lhs_ratio > *rhs->lhs_ratio);
        case TableEnumTask::kDescription:
            return (order == Qt::AscendingOrder) ? (*lhs->description < *rhs->description) : (*lhs->description > *rhs->description);
        case TableEnumTask::kSupportID:
            return (order == Qt::AscendingOrder) ? (*lhs->support_id < *rhs->support_id) : (*lhs->support_id > *rhs->support_id);
        case TableEnumTask::kRhsNode:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_node < *rhs->rhs_node) : (*lhs->rhs_node > *rhs->rhs_node);
        case TableEnumTask::kState:
            return (order == Qt::AscendingOrder) ? (*lhs->state < *rhs->state) : (*lhs->state > *rhs->state);
        case TableEnumTask::kDocument:
            return (order == Qt::AscendingOrder) ? (lhs->document->size() < rhs->document->size()) : (lhs->document->size() > rhs->document->size());
        case TableEnumTask::kDebit:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_debit < *rhs->lhs_debit) : (*lhs->lhs_debit > *rhs->lhs_debit);
        case TableEnumTask::kCredit:
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

Qt::ItemFlags TableModelTask::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const TableEnumTask kColumn { index.column() };

    switch (kColumn) {
    case TableEnumTask::kID:
    case TableEnumTask::kSubtotal:
    case TableEnumTask::kDocument:
    case TableEnumTask::kState:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}

bool TableModelTask::removeRows(int row, int /*count*/, const QModelIndex& parent)
{
    if (row <= -1)
        return false;

    auto* trans_shadow { trans_shadow_list_.at(row) };
    int rhs_node_id { *trans_shadow->rhs_node };

    beginRemoveRows(parent, row, row);
    trans_shadow_list_.removeAt(row);
    endRemoveRows();

    if (rhs_node_id != 0) {
        double unit_cost { *trans_shadow->lhs_ratio };
        double debit { *trans_shadow->lhs_debit };
        double credit { *trans_shadow->lhs_credit };
        emit SUpdateLeafValue(node_id_, -debit, -credit, -unit_cost * debit, -unit_cost * credit);

        debit = *trans_shadow->rhs_debit;
        credit = *trans_shadow->rhs_credit;
        emit SUpdateLeafValue(rhs_node_id, -debit, -credit, -unit_cost * debit, -unit_cost * credit);

        int trans_id { *trans_shadow->id };
        emit SRemoveOneTrans(info_.section, rhs_node_id, trans_id);

        TableModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, row, rule_);

        if (int support_id = *trans_shadow->support_id; support_id != 0)
            emit SRemoveSupportTrans(info_.section, support_id, *trans_shadow->id);

        sql_->RemoveTrans(trans_id);

        QTimer::singleShot(50, this, [this, rhs_node_id, unit_cost]() {
            emit SSyncDouble(rhs_node_id, std::to_underlying(TableEnumTask::kUnitCost), -unit_cost);
            emit SSyncDouble(node_id_, std::to_underlying(TableEnumTask::kUnitCost), -unit_cost);
        });
    }

    ResourcePool<TransShadow>::Instance().Recycle(trans_shadow);
    return true;
}

bool TableModelTask::UpdateDebit(TransShadow* trans_shadow, double value)
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

    double unit_cost { *trans_shadow->lhs_ratio };
    double quantity_debit_diff { *trans_shadow->lhs_debit - lhs_debit };
    double quantity_credit_diff { *trans_shadow->lhs_credit - lhs_credit };
    double amount_debit_diff { quantity_debit_diff * unit_cost };
    double amount_credit_diff { quantity_credit_diff * unit_cost };

    emit SUpdateLeafValue(node_id_, quantity_debit_diff, quantity_credit_diff, amount_debit_diff, amount_credit_diff);
    emit SUpdateLeafValue(*trans_shadow->rhs_node, quantity_credit_diff, quantity_debit_diff, amount_credit_diff, amount_debit_diff);

    return true;
}

bool TableModelTask::UpdateCredit(TransShadow* trans_shadow, double value)
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

    double unit_cost { *trans_shadow->lhs_ratio };
    double quantity_debit_diff { *trans_shadow->lhs_debit - lhs_debit };
    double quantity_credit_diff { *trans_shadow->lhs_credit - lhs_credit };
    double amount_debit_diff { quantity_debit_diff * unit_cost };
    double amount_credit_diff { quantity_credit_diff * unit_cost };

    emit SUpdateLeafValue(node_id_, quantity_debit_diff, quantity_credit_diff, amount_debit_diff, amount_credit_diff);
    emit SUpdateLeafValue(*trans_shadow->rhs_node, quantity_credit_diff, quantity_debit_diff, amount_credit_diff, amount_debit_diff);

    return true;
}

bool TableModelTask::UpdateRatio(TransShadow* trans_shadow, double value)
{
    double unit_cost { *trans_shadow->lhs_ratio };
    if (std::abs(unit_cost - value) < kTolerance || value < 0)
        return false;

    double diff { value - unit_cost };
    *trans_shadow->lhs_ratio = value;
    *trans_shadow->rhs_ratio = value;

    if (*trans_shadow->rhs_node == 0)
        return false;

    sql_->UpdateField(info_.trans, value, kUnitCost, *trans_shadow->id);

    emit SUpdateLeafValue(node_id_, 0, 0, *trans_shadow->lhs_debit * diff, *trans_shadow->lhs_credit * diff);
    emit SUpdateLeafValue(*trans_shadow->rhs_node, 0, 0, *trans_shadow->rhs_debit * diff, *trans_shadow->rhs_credit * diff);

    emit SSyncDouble(*trans_shadow->rhs_node, std::to_underlying(TreeEnumTask::kUnitCost), diff);
    emit SSyncDouble(node_id_, std::to_underlying(TreeEnumTask::kUnitCost), diff);

    return true;
}
