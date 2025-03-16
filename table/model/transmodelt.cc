#include "transmodelt.h"

#include <QTimer>

#include "component/constvalue.h"
#include "global/resourcepool.h"
#include "transmodelutils.h"

TransModelT::TransModelT(CTransModelArg& arg, QObject* parent)
    : TransModel { arg, parent }
{
    assert(node_id_ >= 1 && "Assertion failed: node_id_ must be positive (>= 1)");
    sql_->ReadTrans(trans_shadow_list_, node_id_);
}

QVariant TransModelT::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* trans_shadow { trans_shadow_list_.at(index.row()) };
    const TransEnumT kColumn { index.column() };

    switch (kColumn) {
    case TransEnumT::kID:
        return *trans_shadow->id;
    case TransEnumT::kDateTime:
        return *trans_shadow->date_time;
    case TransEnumT::kCode:
        return *trans_shadow->code;
    case TransEnumT::kUnitCost:
        return *trans_shadow->lhs_ratio == 0 ? QVariant() : *trans_shadow->lhs_ratio;
    case TransEnumT::kDescription:
        return *trans_shadow->description;
    case TransEnumT::kSupportID:
        return *trans_shadow->support_id == 0 ? QVariant() : *trans_shadow->support_id;
    case TransEnumT::kRhsNode:
        return *trans_shadow->rhs_node == 0 ? QVariant() : *trans_shadow->rhs_node;
    case TransEnumT::kState:
        return *trans_shadow->state ? *trans_shadow->state : QVariant();
    case TransEnumT::kDocument:
        return trans_shadow->document->isEmpty() ? QVariant() : trans_shadow->document->size();
    case TransEnumT::kDebit:
        return *trans_shadow->lhs_debit == 0 ? QVariant() : *trans_shadow->lhs_debit;
    case TransEnumT::kCredit:
        return *trans_shadow->lhs_credit == 0 ? QVariant() : *trans_shadow->lhs_credit;
    case TransEnumT::kSubtotal:
        return trans_shadow->subtotal;
    default:
        return QVariant();
    }
}

bool TransModelT::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    const TransEnumT kColumn { index.column() };
    const int kRow { index.row() };

    auto* trans_shadow { trans_shadow_list_.at(kRow) };
    int old_rhs_node { *trans_shadow->rhs_node };
    int old_sup_node { *trans_shadow->support_id };

    bool rhs_changed { false };
    bool deb_changed { false };
    bool cre_changed { false };
    bool sup_changed { false };

    switch (kColumn) {
    case TransEnumT::kDateTime:
        TransModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kDateTime, value.toString(), &TransShadow::date_time);
        break;
    case TransEnumT::kCode:
        TransModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kCode, value.toString(), &TransShadow::code);
        break;
    case TransEnumT::kState:
        TransModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kState, value.toBool(), &TransShadow::state);
        break;
    case TransEnumT::kDescription:
        TransModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kDescription, value.toString(), &TransShadow::description, [this]() { emit SSearch(); });
        break;
    case TransEnumT::kSupportID:
        sup_changed = TransModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kSupportID, value.toInt(), &TransShadow::support_id);
        break;
    case TransEnumT::kUnitCost:
        UpdateRatio(trans_shadow, value.toDouble());
        break;
    case TransEnumT::kRhsNode:
        rhs_changed = TransModelUtils::UpdateRhsNode(trans_shadow, value.toInt());
        break;
    case TransEnumT::kDebit:
        deb_changed = UpdateDebit(trans_shadow, value.toDouble());
        break;
    case TransEnumT::kCredit:
        cre_changed = UpdateCredit(trans_shadow, value.toDouble());
        break;
    default:
        return false;
    }

    if (old_rhs_node == 0 && rhs_changed) {
        sql_->WriteTrans(trans_shadow);
        TransModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, kRow, node_rule_);

        emit SResizeColumnToContents(std::to_underlying(TransEnumT::kSubtotal));
        emit SAppendOneTransL(section_, trans_shadow);

        emit SSyncDouble(*trans_shadow->rhs_node, std::to_underlying(TransEnumT::kUnitCost), *trans_shadow->lhs_ratio);
        emit SSyncDouble(node_id_, std::to_underlying(TransEnumT::kUnitCost), *trans_shadow->lhs_ratio);

        double ratio { *trans_shadow->lhs_ratio };
        double debit { *trans_shadow->lhs_debit };
        double credit { *trans_shadow->lhs_credit };
        emit SUpdateLeafValue(node_id_, debit, credit, ratio * debit, ratio * credit);

        ratio = *trans_shadow->rhs_ratio;
        debit = *trans_shadow->rhs_debit;
        credit = *trans_shadow->rhs_credit;
        emit SUpdateLeafValue(*trans_shadow->rhs_node, debit, credit, ratio * debit, ratio * credit);

        if (*trans_shadow->support_id != 0) {
            emit SAppendOneTransS(section_, *trans_shadow->support_id, *trans_shadow->id);
        }
    }

    if (deb_changed || cre_changed) {
        sql_->SyncTransValue(trans_shadow);
        TransModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, kRow, node_rule_);

        emit SSearch();
        emit SUpdateBalance(section_, old_rhs_node, *trans_shadow->id);
        emit SResizeColumnToContents(std::to_underlying(TransEnumT::kSubtotal));
    }

    if (sup_changed) {
        if (old_sup_node != 0)
            emit SRemoveOneTransS(section_, old_sup_node, *trans_shadow->id);

        if (*trans_shadow->support_id != 0 && *trans_shadow->id != 0) {
            emit SAppendOneTransS(section_, *trans_shadow->support_id, *trans_shadow->id);
        }
    }

    if (old_rhs_node != 0 && rhs_changed) {
        sql_->SyncTransValue(trans_shadow);
        emit SRemoveOneTransL(section_, old_rhs_node, *trans_shadow->id);
        emit SAppendOneTransL(section_, trans_shadow);

        double ratio { *trans_shadow->rhs_ratio };
        double debit { *trans_shadow->rhs_debit };
        double credit { *trans_shadow->rhs_credit };
        emit SUpdateLeafValue(*trans_shadow->rhs_node, debit, credit, ratio * debit, ratio * credit);
        emit SUpdateLeafValue(old_rhs_node, -debit, -credit, -ratio * debit, -ratio * credit);
    }

    emit SResizeColumnToContents(index.column());
    return true;
}

void TransModelT::sort(int column, Qt::SortOrder order)
{
    assert(column >= 0 && "Column index out of range");
    if (column >= info_.trans_header.size() - 1)
        return;

    auto Compare = [column, order](TransShadow* lhs, TransShadow* rhs) -> bool {
        const TransEnumT kColumn { column };

        switch (kColumn) {
        case TransEnumT::kDateTime:
            return (order == Qt::AscendingOrder) ? (*lhs->date_time < *rhs->date_time) : (*lhs->date_time > *rhs->date_time);
        case TransEnumT::kCode:
            return (order == Qt::AscendingOrder) ? (*lhs->code < *rhs->code) : (*lhs->code > *rhs->code);
        case TransEnumT::kUnitCost:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_ratio < *rhs->lhs_ratio) : (*lhs->lhs_ratio > *rhs->lhs_ratio);
        case TransEnumT::kDescription:
            return (order == Qt::AscendingOrder) ? (*lhs->description < *rhs->description) : (*lhs->description > *rhs->description);
        case TransEnumT::kSupportID:
            return (order == Qt::AscendingOrder) ? (*lhs->support_id < *rhs->support_id) : (*lhs->support_id > *rhs->support_id);
        case TransEnumT::kRhsNode:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_node < *rhs->rhs_node) : (*lhs->rhs_node > *rhs->rhs_node);
        case TransEnumT::kState:
            return (order == Qt::AscendingOrder) ? (*lhs->state < *rhs->state) : (*lhs->state > *rhs->state);
        case TransEnumT::kDocument:
            return (order == Qt::AscendingOrder) ? (lhs->document->size() < rhs->document->size()) : (lhs->document->size() > rhs->document->size());
        case TransEnumT::kDebit:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_debit < *rhs->lhs_debit) : (*lhs->lhs_debit > *rhs->lhs_debit);
        case TransEnumT::kCredit:
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

Qt::ItemFlags TransModelT::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const TransEnumT kColumn { index.column() };

    switch (kColumn) {
    case TransEnumT::kID:
    case TransEnumT::kSubtotal:
    case TransEnumT::kDocument:
    case TransEnumT::kState:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}

bool TransModelT::removeRows(int row, int /*count*/, const QModelIndex& parent)
{
    assert(row >= 0 && row <= rowCount(parent) - 1 && "Row must be in the valid range [0, rowCount(parent) - 1]");

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
        emit SRemoveOneTransL(section_, rhs_node_id, trans_id);

        TransModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, row, node_rule_);

        if (int support_id = *trans_shadow->support_id; support_id != 0)
            emit SRemoveOneTransS(section_, support_id, *trans_shadow->id);

        sql_->RemoveTrans(trans_id);

        QTimer::singleShot(50, this, [this, rhs_node_id, unit_cost]() {
            emit SSyncDouble(rhs_node_id, std::to_underlying(TransEnumT::kUnitCost), -unit_cost);
            emit SSyncDouble(node_id_, std::to_underlying(TransEnumT::kUnitCost), -unit_cost);
        });
    }

    ResourcePool<TransShadow>::Instance().Recycle(trans_shadow);
    return true;
}

bool TransModelT::UpdateDebit(TransShadow* trans_shadow, double value)
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

bool TransModelT::UpdateCredit(TransShadow* trans_shadow, double value)
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

bool TransModelT::UpdateRatio(TransShadow* trans_shadow, double value)
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

    emit SSyncDouble(*trans_shadow->rhs_node, std::to_underlying(NodeEnumT::kUnitCost), delta);
    emit SSyncDouble(node_id_, std::to_underlying(NodeEnumT::kUnitCost), delta);

    return true;
}
