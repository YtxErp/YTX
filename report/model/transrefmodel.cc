#include "transrefmodel.h"

#include "component/enumclass.h"
#include "global/resourcepool.h"

TransRefModel::TransRefModel(Sql* sql, CInfo& info, int unit, QObject* parent)
    : QAbstractItemModel { parent }
    , sql_ { sql }
    , info_ { info }
    , unit_ { unit }
{
}

TransRefModel::~TransRefModel() { ResourcePool<Trans>::Instance().Recycle(trans_list_); }

QModelIndex TransRefModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex TransRefModel::parent(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

int TransRefModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return trans_list_.size();
}

int TransRefModel::columnCount(const QModelIndex& /*parent*/) const { return info_.trans_ref_header.size(); }

QVariant TransRefModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* trans { trans_list_.at(index.row()) };
    const TransRefEnum kColumn { index.column() };

    switch (kColumn) {
    case TransRefEnum::kDateTime:
        return trans->date_time;
    case TransRefEnum::kPP:
        return trans->rhs_node;
    case TransRefEnum::kSection:
        return trans->id;
    case TransRefEnum::kOrderNode:
        return trans->lhs_node;
    case TransRefEnum::kOutsideProduct:
        return trans->support_id == 0 ? QVariant() : trans->support_id;
    case TransRefEnum::kFirst:
        return trans->lhs_debit == 0 ? QVariant() : trans->lhs_debit;
    case TransRefEnum::kSecond:
        return trans->lhs_credit == 0 ? QVariant() : trans->lhs_credit;
    case TransRefEnum::kUnitPrice:
        return trans->lhs_ratio == 0 ? QVariant() : trans->lhs_ratio;
    case TransRefEnum::kDiscountPrice:
        return trans->rhs_ratio == 0 ? QVariant() : trans->rhs_ratio;
    case TransRefEnum::kDescription:
        return trans->description;
    case TransRefEnum::kGrossAmount:
        return trans->rhs_debit == 0 ? QVariant() : trans->rhs_debit;
    default:
        return QVariant();
    }
}

QVariant TransRefModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return info_.trans_ref_header.at(section);

    return QVariant();
}

void TransRefModel::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.trans_ref_header.size() - 1)
        return;

    auto Compare = [column, order](const Trans* lhs, const Trans* rhs) -> bool {
        const TransRefEnum kColumn { column };

        switch (kColumn) {
        case TransRefEnum::kOutsideProduct:
            return (order == Qt::AscendingOrder) ? (lhs->support_id < rhs->support_id) : (lhs->support_id > rhs->support_id);
        case TransRefEnum::kDateTime:
            return (order == Qt::AscendingOrder) ? (lhs->date_time < rhs->date_time) : (lhs->date_time > rhs->date_time);
        case TransRefEnum::kPP:
            return (order == Qt::AscendingOrder) ? (lhs->id < rhs->id) : (lhs->id > rhs->id);
        case TransRefEnum::kUnitPrice:
            return (order == Qt::AscendingOrder) ? (lhs->lhs_ratio < rhs->lhs_ratio) : (lhs->lhs_ratio > rhs->lhs_ratio);
        case TransRefEnum::kFirst:
            return (order == Qt::AscendingOrder) ? (lhs->lhs_debit < rhs->lhs_debit) : (lhs->lhs_debit > rhs->lhs_debit);
        case TransRefEnum::kSecond:
            return (order == Qt::AscendingOrder) ? (lhs->lhs_credit < rhs->lhs_credit) : (lhs->lhs_credit > rhs->lhs_credit);
        case TransRefEnum::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case TransRefEnum::kGrossAmount:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_debit < rhs->rhs_debit) : (lhs->rhs_debit > rhs->rhs_debit);
        case TransRefEnum::kDiscountPrice:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_ratio < rhs->rhs_ratio) : (lhs->rhs_ratio > rhs->rhs_ratio);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(trans_list_.begin(), trans_list_.end(), Compare);
    emit layoutChanged();
}

void TransRefModel::RResetModel(int node_id, const QDateTime& start, const QDateTime& end)
{
    if (node_id <= 0)
        return;

    beginResetModel();
    if (!trans_list_.isEmpty())
        ResourcePool<Trans>::Instance().Recycle(trans_list_);

    if (node_id >= 1)
        sql_->ReadTransRef(trans_list_, node_id, unit_, start, end);

    endResetModel();
}
