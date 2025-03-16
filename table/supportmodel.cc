#include "supportmodel.h"

#include "component/enumclass.h"

SupportModel::SupportModel(Sqlite* sql, int support_id, CInfo& info, QObject* parent)
    : QAbstractItemModel { parent }
    , sql_ { sql }
    , info_ { info }
    , support_id_ { support_id }
{
    assert(support_id >= 1 && "support_id must be positive");
    sql_->ReadSupportTrans(trans_list_, support_id);
}

void SupportModel::RAppendOneTransS(int support_id, int trans_id)
{
    assert(support_id_ == support_id && "support_id_ must match support_id");
    assert(trans_id >= 1 && "trans_id must be positive");

    auto row { trans_list_.size() };
    TransList trans_list {};

    sql_->RetrieveTransRange(trans_list, { trans_id });
    beginInsertRows(QModelIndex(), row, row);
    trans_list_.append(trans_list);
    endInsertRows();
}

void SupportModel::RRemoveOneTransS(int support_id, int trans_id)
{
    assert(support_id_ == support_id && "support_id_ must match support_id");
    assert(trans_id >= 1 && "trans_id must be positive");

    auto idx { GetIndex(trans_id) };
    if (!idx.isValid())
        return;

    int row { idx.row() };
    beginRemoveRows(QModelIndex(), row, row);
    trans_list_.removeAt(row);
    endRemoveRows();
}

void SupportModel::RAppendMultiTransS(int support_id, const QSet<int>& trans_id_set)
{
    assert(support_id_ == support_id && "Support ID mismatch detected!");

    auto row { trans_list_.size() };
    TransList trans_shadow_list {};

    sql_->RetrieveTransRange(trans_shadow_list, trans_id_set);
    beginInsertRows(QModelIndex(), row, row + trans_shadow_list.size() - 1);
    trans_list_.append(trans_shadow_list);
    endInsertRows();
}

QModelIndex SupportModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column);
}

QVariant SupportModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* trans_shadow { trans_list_.at(index.row()) };
    const TransSearchEnum kColumn { index.column() };

    switch (kColumn) {
    case TransSearchEnum::kID:
        return trans_shadow->id;
    case TransSearchEnum::kDateTime:
        return trans_shadow->date_time;
    case TransSearchEnum::kCode:
        return trans_shadow->code;
    case TransSearchEnum::kLhsNode:
        return trans_shadow->lhs_node;
    case TransSearchEnum::kLhsRatio:
        return trans_shadow->lhs_ratio == 0 ? QVariant() : trans_shadow->lhs_ratio;
    case TransSearchEnum::kLhsDebit:
        return trans_shadow->lhs_debit == 0 ? QVariant() : trans_shadow->lhs_debit;
    case TransSearchEnum::kLhsCredit:
        return trans_shadow->lhs_credit == 0 ? QVariant() : trans_shadow->lhs_credit;
    case TransSearchEnum::kDescription:
        return trans_shadow->description;
    case TransSearchEnum::kRhsNode:
        return trans_shadow->rhs_node;
    case TransSearchEnum::kRhsRatio:
        return trans_shadow->rhs_ratio == 0 ? QVariant() : trans_shadow->rhs_ratio;
    case TransSearchEnum::kRhsDebit:
        return trans_shadow->rhs_debit == 0 ? QVariant() : trans_shadow->rhs_debit;
    case TransSearchEnum::kRhsCredit:
        return trans_shadow->rhs_credit == 0 ? QVariant() : trans_shadow->rhs_credit;
    case TransSearchEnum::kState:
        return trans_shadow->state ? trans_shadow->state : QVariant();
    case TransSearchEnum::kDocument:
        return trans_shadow->document.isEmpty() ? QVariant() : trans_shadow->document.size();
    default:
        return QVariant();
    }
}

void SupportModel::sort(int column, Qt::SortOrder order)
{
    assert(column >= 0 && column <= info_.search_trans_header.size() - 1 && "Column index out of range in search_trans_header");

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
        case TransSearchEnum::kRhsNode:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_node < rhs->rhs_node) : (lhs->rhs_node > rhs->rhs_node);
        case TransSearchEnum::kRhsRatio:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_ratio < rhs->rhs_ratio) : (lhs->rhs_ratio > rhs->rhs_ratio);
        case TransSearchEnum::kRhsDebit:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_debit < rhs->rhs_debit) : (lhs->rhs_debit > rhs->rhs_debit);
        case TransSearchEnum::kRhsCredit:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_credit < rhs->rhs_credit) : (lhs->rhs_credit > rhs->rhs_credit);
        case TransSearchEnum::kState:
            return (order == Qt::AscendingOrder) ? (lhs->state < rhs->state) : (lhs->state > rhs->state);
        case TransSearchEnum::kDocument:
            return (order == Qt::AscendingOrder) ? (lhs->document.size() < rhs->document.size()) : (lhs->document.size() > rhs->document.size());
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(trans_list_.begin(), trans_list_.end(), Compare);
    emit layoutChanged();
}

Qt::ItemFlags SupportModel::flags(const QModelIndex& index) const
{
    return index.isValid() ? (QAbstractItemModel::flags(index) & ~Qt::ItemIsEditable) : Qt::NoItemFlags;
}

QVariant SupportModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return info_.search_trans_header.at(section);

    return QVariant();
}

void SupportModel::RRemoveMultiTransS(int support_id, const QSet<int>& trans_id_set)
{
    assert(support_id_ == support_id && "Support ID mismatch detected!");

    for (int i = trans_list_.size() - 1; i >= 0; --i) {
        int trans_id { trans_list_[i]->id };

        if (trans_id_set.contains(trans_id)) {
            beginRemoveRows(QModelIndex(), i, i);
            trans_list_.removeAt(i);
            endRemoveRows();
        }
    }
}

QModelIndex SupportModel::GetIndex(int trans_id) const
{
    int row { 0 };

    for (const auto* trans_shadow : trans_list_) {
        if (trans_shadow->id == trans_id) {
            return index(row, 0);
        }
        ++row;
    }
    return QModelIndex();
}
