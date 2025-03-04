#include "transreffetchermodel.h"

#include "component/enumclass.h"

TransRefFetcherModel::TransRefFetcherModel(int node_id, CInfo& info, Sqlite* sql, QObject* parent)
    : QAbstractItemModel { parent }
    , sql_ { sql }
    , info_ { info }
    , node_id_ { node_id }
{
    Query(node_id);
}

TransRefFetcherModel::~TransRefFetcherModel() { trans_list_.clear(); }

QModelIndex TransRefFetcherModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex TransRefFetcherModel::parent(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

int TransRefFetcherModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return trans_list_.size();
}

int TransRefFetcherModel::columnCount(const QModelIndex& /*parent*/) const { return info_.reference_header.size(); }

QVariant TransRefFetcherModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* trans { trans_list_.at(index.row()) };
    const TableEnumRefFetcher kColumn { index.column() };

    switch (kColumn) {
    case TableEnumRefFetcher::kDateTime:
        return trans->date_time;
    case TableEnumRefFetcher::kParty:
        return trans->id;
    case TableEnumRefFetcher::kLhsNode:
        return trans->lhs_node;
    case TableEnumRefFetcher::kFirst:
        return trans->lhs_debit == 0 ? QVariant() : trans->lhs_debit;
    case TableEnumRefFetcher::kSecond:
        return trans->lhs_credit == 0 ? QVariant() : trans->lhs_credit;
    case TableEnumRefFetcher::kUnitPrice:
        return trans->lhs_ratio == 0 ? QVariant() : trans->lhs_ratio;
    case TableEnumRefFetcher::kDiscountPrice:
        return trans->rhs_ratio == 0 ? QVariant() : trans->rhs_ratio;
    case TableEnumRefFetcher::kDescription:
        return trans->description;
    case TableEnumRefFetcher::kCode:
        return trans->code;
    case TableEnumRefFetcher::kGrossAmount:
        return trans->rhs_debit == 0 ? QVariant() : trans->rhs_debit;
    case TableEnumRefFetcher::kDiscount:
        return trans->discount == 0 ? QVariant() : trans->discount;
    case TableEnumRefFetcher::kNetAmount:
        return trans->rhs_credit == 0 ? QVariant() : trans->rhs_credit;
    default:
        return QVariant();
    }
}

QVariant TransRefFetcherModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return info_.reference_header.at(section);

    return QVariant();
}

void TransRefFetcherModel::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.reference_header.size() - 1)
        return;

    auto Compare = [column, order](const Trans* lhs, const Trans* rhs) -> bool {
        const TableEnumRefFetcher kColumn { column };

        switch (kColumn) {
        case TableEnumRefFetcher::kDateTime:
            return (order == Qt::AscendingOrder) ? (lhs->date_time < rhs->date_time) : (lhs->date_time > rhs->date_time);
        case TableEnumRefFetcher::kCode:
            return (order == Qt::AscendingOrder) ? (lhs->code < rhs->code) : (lhs->code > rhs->code);
        case TableEnumRefFetcher::kParty:
            return (order == Qt::AscendingOrder) ? (lhs->id < rhs->id) : (lhs->id > rhs->id);
        case TableEnumRefFetcher::kUnitPrice:
            return (order == Qt::AscendingOrder) ? (lhs->lhs_ratio < rhs->lhs_ratio) : (lhs->lhs_ratio > rhs->lhs_ratio);
        case TableEnumRefFetcher::kFirst:
            return (order == Qt::AscendingOrder) ? (lhs->lhs_debit < rhs->lhs_debit) : (lhs->lhs_debit > rhs->lhs_debit);
        case TableEnumRefFetcher::kSecond:
            return (order == Qt::AscendingOrder) ? (lhs->lhs_credit < rhs->lhs_credit) : (lhs->lhs_credit > rhs->lhs_credit);
        case TableEnumRefFetcher::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case TableEnumRefFetcher::kDiscount:
            return (order == Qt::AscendingOrder) ? (lhs->discount < rhs->discount) : (lhs->discount > rhs->discount);
        case TableEnumRefFetcher::kNetAmount:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_credit < rhs->rhs_credit) : (lhs->rhs_credit > rhs->rhs_credit);
        case TableEnumRefFetcher::kGrossAmount:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_debit < rhs->rhs_debit) : (lhs->rhs_debit > rhs->rhs_debit);
        case TableEnumRefFetcher::kDiscountPrice:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_ratio < rhs->rhs_ratio) : (lhs->rhs_ratio > rhs->rhs_ratio);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(trans_list_.begin(), trans_list_.end(), Compare);
    emit layoutChanged();
}

void TransRefFetcherModel::Query(int node_id)
{
    beginResetModel();
    if (!trans_list_.isEmpty())
        trans_list_.clear();

    if (node_id >= 1)
        sql_->TransRefFetcher(trans_list_, node_id);

    endResetModel();
}
