#include "tablemodelfinance.h"

#include "component/constvalue.h"
#include "tablemodelutils.h"

TableModelFinance::TableModelFinance(Sqlite* sql, bool rule, int node_id, CInfo& info, QObject* parent)
    : TableModel { sql, rule, node_id, info, parent }
{
    if (node_id >= 1)
        sql_->ReadNodeTrans(trans_shadow_list_, node_id);
}

QVariant TableModelFinance::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* trans_shadow { trans_shadow_list_.at(index.row()) };
    const TableEnumFinance kColumn { index.column() };

    switch (kColumn) {
    case TableEnumFinance::kID:
        return *trans_shadow->id;
    case TableEnumFinance::kDateTime:
        return *trans_shadow->date_time;
    case TableEnumFinance::kCode:
        return *trans_shadow->code;
    case TableEnumFinance::kLhsRatio:
        return *trans_shadow->lhs_ratio;
    case TableEnumFinance::kDescription:
        return *trans_shadow->description;
    case TableEnumFinance::kSupportID:
        return *trans_shadow->support_id == 0 ? QVariant() : *trans_shadow->support_id;
    case TableEnumFinance::kRhsNode:
        return *trans_shadow->rhs_node == 0 ? QVariant() : *trans_shadow->rhs_node;
    case TableEnumFinance::kState:
        return *trans_shadow->state ? *trans_shadow->state : QVariant();
    case TableEnumFinance::kDocument:
        return trans_shadow->document->isEmpty() ? QVariant() : trans_shadow->document->size();
    case TableEnumFinance::kDebit:
        return *trans_shadow->lhs_debit == 0 ? QVariant() : *trans_shadow->lhs_debit;
    case TableEnumFinance::kCredit:
        return *trans_shadow->lhs_credit == 0 ? QVariant() : *trans_shadow->lhs_credit;
    case TableEnumFinance::kSubtotal:
        return trans_shadow->subtotal;
    default:
        return QVariant();
    }
}

bool TableModelFinance::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    const TableEnumFinance kColumn { index.column() };
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
    case TableEnumFinance::kDateTime:
        TableModelUtils::UpdateField(sql_, trans_shadow, info_.transaction, value.toString(), kDateTime, &TransShadow::date_time);
        break;
    case TableEnumFinance::kCode:
        TableModelUtils::UpdateField(sql_, trans_shadow, info_.transaction, value.toString(), kCode, &TransShadow::code);
        break;
    case TableEnumFinance::kState:
        TableModelUtils::UpdateField(sql_, trans_shadow, info_.transaction, value.toBool(), kState, &TransShadow::state);
        break;
    case TableEnumFinance::kDescription:
        TableModelUtils::UpdateField(
            sql_, trans_shadow, info_.transaction, value.toString(), kDescription, &TransShadow::description, [this]() { emit SSearch(); });
        break;
    case TableEnumFinance::kSupportID:
        sup_changed = TableModelUtils::UpdateField(sql_, trans_shadow, info_.transaction, value.toInt(), kSupportID, &TransShadow::support_id);
        break;
    case TableEnumFinance::kLhsRatio:
        rat_changed = UpdateRatio(trans_shadow, value.toDouble());
        break;
    case TableEnumFinance::kRhsNode:
        rhs_changed = TableModelUtils::UpdateRhsNode(trans_shadow, value.toInt());
        break;
    case TableEnumFinance::kDebit:
        deb_changed = UpdateDebit(trans_shadow, value.toDouble());
        break;
    case TableEnumFinance::kCredit:
        cre_changed = UpdateCredit(trans_shadow, value.toDouble());
        break;
    default:
        return false;
    }

    if (old_rhs_node == 0) {
        if (rhs_changed) {
            sql_->WriteTrans(trans_shadow);
            TableModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, kRow, rule_);

            emit SResizeColumnToContents(std::to_underlying(TableEnumFinance::kSubtotal));
            emit SAppendOneTrans(info_.section, trans_shadow);

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
        emit SResizeColumnToContents(std::to_underlying(TableEnumFinance::kSubtotal));
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

void TableModelFinance::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.table_header.size() - 1)
        return;

    auto Compare = [column, order](TransShadow* lhs, TransShadow* rhs) -> bool {
        const TableEnumFinance kColumn { column };

        switch (kColumn) {
        case TableEnumFinance::kDateTime:
            return (order == Qt::AscendingOrder) ? (*lhs->date_time < *rhs->date_time) : (*lhs->date_time > *rhs->date_time);
        case TableEnumFinance::kCode:
            return (order == Qt::AscendingOrder) ? (*lhs->code < *rhs->code) : (*lhs->code > *rhs->code);
        case TableEnumFinance::kLhsRatio:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_ratio < *rhs->lhs_ratio) : (*lhs->lhs_ratio > *rhs->lhs_ratio);
        case TableEnumFinance::kDescription:
            return (order == Qt::AscendingOrder) ? (*lhs->description < *rhs->description) : (*lhs->description > *rhs->description);
        case TableEnumFinance::kSupportID:
            return (order == Qt::AscendingOrder) ? (*lhs->support_id < *rhs->support_id) : (*lhs->support_id > *rhs->support_id);
        case TableEnumFinance::kRhsNode:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_node < *rhs->rhs_node) : (*lhs->rhs_node > *rhs->rhs_node);
        case TableEnumFinance::kState:
            return (order == Qt::AscendingOrder) ? (*lhs->state < *rhs->state) : (*lhs->state > *rhs->state);
        case TableEnumFinance::kDocument:
            return (order == Qt::AscendingOrder) ? (lhs->document->size() < rhs->document->size()) : (lhs->document->size() > rhs->document->size());
        case TableEnumFinance::kDebit:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_debit < *rhs->lhs_debit) : (*lhs->lhs_debit > *rhs->lhs_debit);
        case TableEnumFinance::kCredit:
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

Qt::ItemFlags TableModelFinance::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const TableEnumFinance kColumn { index.column() };

    switch (kColumn) {
    case TableEnumFinance::kID:
    case TableEnumFinance::kSubtotal:
    case TableEnumFinance::kDocument:
    case TableEnumFinance::kState:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}
