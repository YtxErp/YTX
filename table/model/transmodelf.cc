#include "transmodelf.h"

#include "component/constvalue.h"
#include "transmodelutils.h"

TransModelF::TransModelF(CTransModelArg& arg, QObject* parent)
    : TransModel { arg, parent }
{
    assert(node_id_ >= 1 && "Assertion failed: node_id_ must be positive (>= 1)");
    sql_->ReadTrans(trans_shadow_list_, node_id_);
}

QVariant TransModelF::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* trans_shadow { trans_shadow_list_.at(index.row()) };
    const TransEnumF kColumn { index.column() };

    switch (kColumn) {
    case TransEnumF::kID:
        return *trans_shadow->id;
    case TransEnumF::kDateTime:
        return *trans_shadow->date_time;
    case TransEnumF::kCode:
        return *trans_shadow->code;
    case TransEnumF::kLhsRatio:
        return *trans_shadow->lhs_ratio;
    case TransEnumF::kDescription:
        return *trans_shadow->description;
    case TransEnumF::kSupportID:
        return *trans_shadow->support_id == 0 ? QVariant() : *trans_shadow->support_id;
    case TransEnumF::kRhsNode:
        return *trans_shadow->rhs_node == 0 ? QVariant() : *trans_shadow->rhs_node;
    case TransEnumF::kState:
        return *trans_shadow->state ? *trans_shadow->state : QVariant();
    case TransEnumF::kDocument:
        return trans_shadow->document->isEmpty() ? QVariant() : trans_shadow->document->size();
    case TransEnumF::kDebit:
        return *trans_shadow->lhs_debit == 0 ? QVariant() : *trans_shadow->lhs_debit;
    case TransEnumF::kCredit:
        return *trans_shadow->lhs_credit == 0 ? QVariant() : *trans_shadow->lhs_credit;
    case TransEnumF::kSubtotal:
        return trans_shadow->subtotal;
    default:
        return QVariant();
    }
}

bool TransModelF::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    const TransEnumF kColumn { index.column() };
    const int kRow { index.row() };

    auto* trans_shadow { trans_shadow_list_.at(kRow) };
    int old_rhs_node { *trans_shadow->rhs_node };
    int old_sup_node { *trans_shadow->support_id };

    bool rhs_changed { false };
    bool deb_changed { false };
    bool cre_changed { false };
    bool rat_changed { false };
    bool sup_changed { false };

    switch (kColumn) {
    case TransEnumF::kDateTime:
        TransModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kDateTime, value.toString(), &TransShadow::date_time);
        break;
    case TransEnumF::kCode:
        TransModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kCode, value.toString(), &TransShadow::code);
        break;
    case TransEnumF::kState:
        TransModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kState, value.toBool(), &TransShadow::state);
        break;
    case TransEnumF::kDescription:
        TransModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kDescription, value.toString(), &TransShadow::description, [this]() { emit SSearch(); });
        break;
    case TransEnumF::kSupportID:
        sup_changed = TransModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kSupportID, value.toInt(), &TransShadow::support_id);
        break;
    case TransEnumF::kLhsRatio:
        rat_changed = UpdateRatio(trans_shadow, value.toDouble());
        break;
    case TransEnumF::kRhsNode:
        rhs_changed = TransModelUtils::UpdateRhsNode(trans_shadow, value.toInt());
        break;
    case TransEnumF::kDebit:
        deb_changed = UpdateDebit(trans_shadow, value.toDouble());
        break;
    case TransEnumF::kCredit:
        cre_changed = UpdateCredit(trans_shadow, value.toDouble());
        break;
    default:
        return false;
    }

    if (old_rhs_node == 0 && rhs_changed) {
        sql_->WriteTrans(trans_shadow);
        TransModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, kRow, node_rule_);

        emit SResizeColumnToContents(std::to_underlying(TransEnumF::kSubtotal));
        emit SAppendOneTransL(section_, trans_shadow);

        double ratio { *trans_shadow->lhs_ratio };
        double debit { *trans_shadow->lhs_debit };
        double credit { *trans_shadow->lhs_credit };
        emit SSyncLeafValue(node_id_, debit, credit, ratio * debit, ratio * credit);

        ratio = *trans_shadow->rhs_ratio;
        debit = *trans_shadow->rhs_debit;
        credit = *trans_shadow->rhs_credit;
        emit SSyncLeafValue(*trans_shadow->rhs_node, debit, credit, ratio * debit, ratio * credit);

        if (*trans_shadow->support_id != 0) {
            emit SAppendOneTransS(section_, *trans_shadow->support_id, *trans_shadow->id);
        }
    }

    if (deb_changed || cre_changed || rat_changed) {
        sql_->UpdateTransValue(trans_shadow);
        emit SSearch();
        emit SSyncBalance(section_, old_rhs_node, *trans_shadow->id);
    }

    if (sup_changed) {
        if (old_sup_node != 0)
            emit SRemoveOneTransS(section_, old_sup_node, *trans_shadow->id);

        if (*trans_shadow->support_id != 0 && *trans_shadow->id != 0) {
            emit SAppendOneTransS(section_, *trans_shadow->support_id, *trans_shadow->id);
        }
    }

    if (deb_changed || cre_changed) {
        TransModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, kRow, node_rule_);
        emit SResizeColumnToContents(std::to_underlying(TransEnumF::kSubtotal));
    }

    if (old_rhs_node != 0 && rhs_changed) {
        sql_->UpdateTransValue(trans_shadow);
        emit SRemoveOneTransL(section_, old_rhs_node, *trans_shadow->id);
        emit SAppendOneTransL(section_, trans_shadow);

        double ratio { *trans_shadow->rhs_ratio };
        double debit { *trans_shadow->rhs_debit };
        double credit { *trans_shadow->rhs_credit };
        emit SSyncLeafValue(*trans_shadow->rhs_node, debit, credit, ratio * debit, ratio * credit);
        emit SSyncLeafValue(old_rhs_node, -debit, -credit, -ratio * debit, -ratio * credit);
    }

    emit SResizeColumnToContents(index.column());
    return true;
}

void TransModelF::sort(int column, Qt::SortOrder order)
{
    assert(column >= 0 && "Column index out of range");
    if (column >= info_.trans_header.size() - 1)
        return;

    auto Compare = [column, order](TransShadow* lhs, TransShadow* rhs) -> bool {
        const TransEnumF kColumn { column };

        switch (kColumn) {
        case TransEnumF::kDateTime:
            return (order == Qt::AscendingOrder) ? (*lhs->date_time < *rhs->date_time) : (*lhs->date_time > *rhs->date_time);
        case TransEnumF::kCode:
            return (order == Qt::AscendingOrder) ? (*lhs->code < *rhs->code) : (*lhs->code > *rhs->code);
        case TransEnumF::kLhsRatio:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_ratio < *rhs->lhs_ratio) : (*lhs->lhs_ratio > *rhs->lhs_ratio);
        case TransEnumF::kDescription:
            return (order == Qt::AscendingOrder) ? (*lhs->description < *rhs->description) : (*lhs->description > *rhs->description);
        case TransEnumF::kSupportID:
            return (order == Qt::AscendingOrder) ? (*lhs->support_id < *rhs->support_id) : (*lhs->support_id > *rhs->support_id);
        case TransEnumF::kRhsNode:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_node < *rhs->rhs_node) : (*lhs->rhs_node > *rhs->rhs_node);
        case TransEnumF::kState:
            return (order == Qt::AscendingOrder) ? (*lhs->state < *rhs->state) : (*lhs->state > *rhs->state);
        case TransEnumF::kDocument:
            return (order == Qt::AscendingOrder) ? (lhs->document->size() < rhs->document->size()) : (lhs->document->size() > rhs->document->size());
        case TransEnumF::kDebit:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_debit < *rhs->lhs_debit) : (*lhs->lhs_debit > *rhs->lhs_debit);
        case TransEnumF::kCredit:
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

Qt::ItemFlags TransModelF::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const TransEnumF kColumn { index.column() };

    switch (kColumn) {
    case TransEnumF::kID:
    case TransEnumF::kSubtotal:
    case TransEnumF::kDocument:
    case TransEnumF::kState:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}

void TransModelF::IniRatio(TransShadow* trans_shadow) const
{
    *trans_shadow->lhs_ratio = 1.0;
    *trans_shadow->rhs_ratio = 1.0;
}
