#include "transmodelp.h"

#include "component/constvalue.h"
#include "global/resourcepool.h"
#include "transmodelutils.h"

TransModelP::TransModelP(CTransModelArg& arg, QObject* parent)
    : TransModel { arg, parent }
{
    if (node_id_ >= 1)
        sql_->ReadTrans(trans_shadow_list_, node_id_);
}

QVariant TransModelP::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* trans_shadow { trans_shadow_list_.at(index.row()) };
    const TransEnumP kColumn { index.column() };

    switch (kColumn) {
    case TransEnumP::kID:
        return *trans_shadow->id;
    case TransEnumP::kDateTime:
        return *trans_shadow->date_time;
    case TransEnumP::kCode:
        return *trans_shadow->code;
    case TransEnumP::kUnitCost:
        return *trans_shadow->lhs_ratio == 0 ? QVariant() : *trans_shadow->lhs_ratio;
    case TransEnumP::kDescription:
        return *trans_shadow->description;
    case TransEnumP::kSupportID:
        return *trans_shadow->support_id == 0 ? QVariant() : *trans_shadow->support_id;
    case TransEnumP::kRhsNode:
        return *trans_shadow->rhs_node == 0 ? QVariant() : *trans_shadow->rhs_node;
    case TransEnumP::kState:
        return *trans_shadow->state ? *trans_shadow->state : QVariant();
    case TransEnumP::kDocument:
        return trans_shadow->document->isEmpty() ? QVariant() : trans_shadow->document->size();
    case TransEnumP::kDebit:
        return *trans_shadow->lhs_debit == 0 ? QVariant() : *trans_shadow->lhs_debit;
    case TransEnumP::kCredit:
        return *trans_shadow->lhs_credit == 0 ? QVariant() : *trans_shadow->lhs_credit;
    case TransEnumP::kSubtotal:
        return trans_shadow->subtotal;
    default:
        return QVariant();
    }
}

bool TransModelP::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    const TransEnumP kColumn { index.column() };
    const int kRow { index.row() };

    auto* trans_shadow { trans_shadow_list_.at(kRow) };
    int old_rhs_node { *trans_shadow->rhs_node };
    int old_sup_node { *trans_shadow->support_id };

    bool rhs_changed { false };
    bool deb_changed { false };
    bool cre_changed { false };
    bool sup_changed { false };

    switch (kColumn) {
    case TransEnumP::kDateTime:
        TransModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kDateTime, value.toString(), &TransShadow::date_time);
        break;
    case TransEnumP::kCode:
        TransModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kCode, value.toString(), &TransShadow::code);
        break;
    case TransEnumP::kState:
        TransModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kState, value.toBool(), &TransShadow::state);
        break;
    case TransEnumP::kDescription:
        TransModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kDescription, value.toString(), &TransShadow::description, [this]() { emit SSearch(); });
        break;
    case TransEnumP::kSupportID:
        sup_changed = TransModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kSupportID, value.toInt(), &TransShadow::support_id);
        break;
    case TransEnumP::kUnitCost:
        UpdateRatio(trans_shadow, value.toDouble());
        break;
    case TransEnumP::kRhsNode:
        rhs_changed = TransModelUtils::UpdateRhsNode(trans_shadow, value.toInt());
        break;
    case TransEnumP::kDebit:
        deb_changed = UpdateDebit(trans_shadow, value.toDouble());
        break;
    case TransEnumP::kCredit:
        cre_changed = UpdateCredit(trans_shadow, value.toDouble());
        break;
    default:
        return false;
    }

    if (old_rhs_node == 0 && rhs_changed) {
        sql_->WriteTrans(trans_shadow);
        TransModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, kRow, node_rule_);

        emit SResizeColumnToContents(std::to_underlying(TransEnumP::kSubtotal));
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

    if (sup_changed) {
        if (old_sup_node != 0)
            emit SRemoveSupportTrans(info_.section, old_sup_node, *trans_shadow->id);

        if (*trans_shadow->support_id != 0) {
            emit SAppendSupportTrans(info_.section, trans_shadow);
        }
    }

    if (deb_changed || cre_changed) {
        sql_->SyncTransValue(trans_shadow);
        TransModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, kRow, node_rule_);

        emit SSearch();
        emit SUpdateBalance(info_.section, old_rhs_node, *trans_shadow->id);
        emit SResizeColumnToContents(std::to_underlying(TransEnumP::kSubtotal));
    }

    if (old_rhs_node != 0 && rhs_changed) {
        sql_->SyncTransValue(trans_shadow);
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

void TransModelP::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.trans_header.size() - 1)
        return;

    auto Compare = [column, order](TransShadow* lhs, TransShadow* rhs) -> bool {
        const TransEnumP kColumn { column };

        switch (kColumn) {
        case TransEnumP::kDateTime:
            return (order == Qt::AscendingOrder) ? (*lhs->date_time < *rhs->date_time) : (*lhs->date_time > *rhs->date_time);
        case TransEnumP::kCode:
            return (order == Qt::AscendingOrder) ? (*lhs->code < *rhs->code) : (*lhs->code > *rhs->code);
        case TransEnumP::kUnitCost:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_ratio < *rhs->lhs_ratio) : (*lhs->lhs_ratio > *rhs->lhs_ratio);
        case TransEnumP::kDescription:
            return (order == Qt::AscendingOrder) ? (*lhs->description < *rhs->description) : (*lhs->description > *rhs->description);
        case TransEnumP::kSupportID:
            return (order == Qt::AscendingOrder) ? (*lhs->support_id < *rhs->support_id) : (*lhs->support_id > *rhs->support_id);
        case TransEnumP::kRhsNode:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_node < *rhs->rhs_node) : (*lhs->rhs_node > *rhs->rhs_node);
        case TransEnumP::kState:
            return (order == Qt::AscendingOrder) ? (*lhs->state < *rhs->state) : (*lhs->state > *rhs->state);
        case TransEnumP::kDocument:
            return (order == Qt::AscendingOrder) ? (lhs->document->size() < rhs->document->size()) : (lhs->document->size() > rhs->document->size());
        case TransEnumP::kDebit:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_debit < *rhs->lhs_debit) : (*lhs->lhs_debit > *rhs->lhs_debit);
        case TransEnumP::kCredit:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_credit < *rhs->lhs_credit) : (*lhs->lhs_credit > *rhs->lhs_credit);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(trans_shadow_list_.begin(), trans_shadow_list_.end(), Compare);
    emit layoutChanged();

    TransModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, 0, node_rule_);
}

Qt::ItemFlags TransModelP::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const TransEnumP kColumn { index.column() };

    switch (kColumn) {
    case TransEnumP::kID:
    case TransEnumP::kSubtotal:
    case TransEnumP::kDocument:
    case TransEnumP::kState:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}

bool TransModelP::removeRows(int row, int /*count*/, const QModelIndex& parent)
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
        emit SUpdateLeafValue(*trans_shadow->rhs_node, -debit, -credit, -unit_cost * debit, -unit_cost * credit);

        int trans_id { *trans_shadow->id };
        emit SRemoveOneTrans(info_.section, rhs_node_id, trans_id);
        TransModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, row, node_rule_);

        if (int support_id = *trans_shadow->support_id; support_id != 0)
            emit SRemoveSupportTrans(info_.section, support_id, *trans_shadow->id);

        sql_->RemoveTrans(trans_id);
    }

    ResourcePool<TransShadow>::Instance().Recycle(trans_shadow);
    return true;
}

bool TransModelP::UpdateDebit(TransShadow* trans_shadow, double value)
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
    double quantity_debit_delta { *trans_shadow->lhs_debit - lhs_debit };
    double quantity_credit_delta { *trans_shadow->lhs_credit - lhs_credit };
    double amount_debit_delta { quantity_debit_delta * unit_cost };
    double amount_credit_delta { quantity_credit_delta * unit_cost };

    emit SUpdateLeafValue(node_id_, quantity_debit_delta, quantity_credit_delta, amount_debit_delta, amount_credit_delta);
    emit SUpdateLeafValue(*trans_shadow->rhs_node, quantity_credit_delta, quantity_debit_delta, amount_credit_delta, amount_debit_delta);

    return true;
}

bool TransModelP::UpdateCredit(TransShadow* trans_shadow, double value)
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
    double quantity_debit_delta { *trans_shadow->lhs_debit - lhs_debit };
    double quantity_credit_delta { *trans_shadow->lhs_credit - lhs_credit };
    double amount_debit_delta { quantity_debit_delta * unit_cost };
    double amount_credit_delta { quantity_credit_delta * unit_cost };

    emit SUpdateLeafValue(node_id_, quantity_debit_delta, quantity_credit_delta, amount_debit_delta, amount_credit_delta);
    emit SUpdateLeafValue(*trans_shadow->rhs_node, quantity_credit_delta, quantity_debit_delta, amount_credit_delta, amount_debit_delta);

    return true;
}

bool TransModelP::UpdateRatio(TransShadow* trans_shadow, double value)
{
    double unit_cost { *trans_shadow->lhs_ratio };
    if (std::abs(unit_cost - value) < kTolerance || value < 0)
        return false;

    double delta { value - unit_cost };
    *trans_shadow->lhs_ratio = value;
    *trans_shadow->rhs_ratio = value;

    if (*trans_shadow->rhs_node == 0)
        return false;

    sql_->WriteField(info_.trans, kUnitCost, value, *trans_shadow->id);

    emit SUpdateLeafValue(node_id_, 0, 0, *trans_shadow->lhs_debit * delta, *trans_shadow->lhs_credit * delta);
    emit SUpdateLeafValue(*trans_shadow->rhs_node, 0, 0, *trans_shadow->rhs_debit * delta, *trans_shadow->rhs_credit * delta);

    return true;
}
