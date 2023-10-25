#include "transsearchmodel.h"

#include "component/enumclass.h"

TransSearchModel::TransSearchModel(CInfo& info, Sqlite* sql, QObject* parent)
    : QAbstractItemModel { parent }
    , sql_ { sql }
    , info_ { info }
{
}

QModelIndex TransSearchModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex TransSearchModel::parent(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

int TransSearchModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return trans_list_.size();
}

int TransSearchModel::columnCount(const QModelIndex& /*parent*/) const { return info_.search_trans_header.size(); }

QVariant TransSearchModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* trans { trans_list_.at(index.row()) };
    const TransSearchEnum kColumn { index.column() };

    switch (kColumn) {
    case TransSearchEnum::kID:
        return trans->id;
    case TransSearchEnum::kDateTime:
        return trans->date_time;
    case TransSearchEnum::kCode:
        return trans->code;
    case TransSearchEnum::kLhsNode:
        return trans->lhs_node;
    case TransSearchEnum::kLhsRatio:
        return trans->lhs_ratio == 0 ? QVariant() : trans->lhs_ratio;
    case TransSearchEnum::kLhsDebit:
        return trans->lhs_debit == 0 ? QVariant() : trans->lhs_debit;
    case TransSearchEnum::kLhsCredit:
        return trans->lhs_credit == 0 ? QVariant() : trans->lhs_credit;
    case TransSearchEnum::kDescription:
        return trans->description;
    case TransSearchEnum::kSupportID:
        return trans->support_id == 0 ? QVariant() : trans->support_id;
    case TransSearchEnum::kDiscount:
        return trans->discount == 0 ? QVariant() : trans->discount;
    case TransSearchEnum::kDocument:
        return trans->document.isEmpty() ? QVariant() : trans->document.size();
    case TransSearchEnum::kState:
        return trans->state ? trans->state : QVariant();
    case TransSearchEnum::kRhsCredit:
        return trans->rhs_credit == 0 ? QVariant() : trans->rhs_credit;
    case TransSearchEnum::kRhsDebit:
        return trans->rhs_debit == 0 ? QVariant() : trans->rhs_debit;
    case TransSearchEnum::kRhsRatio:
        return trans->rhs_ratio == 0 ? QVariant() : trans->rhs_ratio;
    case TransSearchEnum::kRhsNode:
        return trans->rhs_node;
    default:
        return QVariant();
    }
}

QVariant TransSearchModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return info_.search_trans_header.at(section);

    return QVariant();
}

void TransSearchModel::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.search_trans_header.size() - 1)
        return;

    auto Compare = [column, order](const Trans* lhs, const Trans* rhs) -> bool {
        const TransSearchEnum kColumn { column };

        switch (kColumn) {
        case TransSearchEnum::kDateTime:
            return (order == Qt::AscendingOrder) ? (lhs->date_time < rhs->date_time) : (lhs->date_time > rhs->date_time);
        case TransSearchEnum::kCode:
            return (order == Qt::AscendingOrder) ? (lhs->code < rhs->code) : (lhs->code > rhs->code);
        case TransSearchEnum::kLhsNode:
            return (order == Qt::AscendingOrder) ? (lhs->lhs_node < rhs->lhs_node) : (lhs->lhs_node > rhs->lhs_node);
        case TransSearchEnum::kLhsRatio:
            return (order == Qt::AscendingOrder) ? (lhs->lhs_ratio < rhs->lhs_ratio) : (lhs->lhs_ratio > rhs->lhs_ratio);
        case TransSearchEnum::kLhsDebit:
            return (order == Qt::AscendingOrder) ? (lhs->lhs_debit < rhs->lhs_debit) : (lhs->lhs_debit > rhs->lhs_debit);
        case TransSearchEnum::kLhsCredit:
            return (order == Qt::AscendingOrder) ? (lhs->lhs_credit < rhs->lhs_credit) : (lhs->lhs_credit > rhs->lhs_credit);
        case TransSearchEnum::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case TransSearchEnum::kSupportID:
            return (order == Qt::AscendingOrder) ? (lhs->support_id < rhs->support_id) : (lhs->support_id > rhs->support_id);
        case TransSearchEnum::kDiscount:
            return (order == Qt::AscendingOrder) ? (lhs->discount < rhs->discount) : (lhs->discount > rhs->discount);
        case TransSearchEnum::kDocument:
            return (order == Qt::AscendingOrder) ? (lhs->document.size() < rhs->document.size()) : (lhs->document.size() > rhs->document.size());
        case TransSearchEnum::kState:
            return (order == Qt::AscendingOrder) ? (lhs->state < rhs->state) : (lhs->state > rhs->state);
        case TransSearchEnum::kRhsCredit:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_credit < rhs->rhs_credit) : (lhs->rhs_credit > rhs->rhs_credit);
        case TransSearchEnum::kRhsDebit:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_debit < rhs->rhs_debit) : (lhs->rhs_debit > rhs->rhs_debit);
        case TransSearchEnum::kRhsRatio:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_ratio < rhs->rhs_ratio) : (lhs->rhs_ratio > rhs->rhs_ratio);
        case TransSearchEnum::kRhsNode:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_node < rhs->rhs_node) : (lhs->rhs_node > rhs->rhs_node);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(trans_list_.begin(), trans_list_.end(), Compare);
    emit layoutChanged();
}

void TransSearchModel::Query(const QString& text)
{
    beginResetModel();
    if (!trans_list_.isEmpty())
        trans_list_.clear();

    if (!text.isEmpty())
        sql_->SearchTrans(trans_list_, text);
    endResetModel();
}
