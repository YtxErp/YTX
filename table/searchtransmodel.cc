#include "searchtransmodel.h"

#include "component/enumclass.h"

SearchTransModel::SearchTransModel(CInfo& info, Sqlite* sql, QObject* parent)
    : QAbstractItemModel { parent }
    , sql_ { sql }
    , info_ { info }
{
}

SearchTransModel::~SearchTransModel() { trans_list_.clear(); }

QModelIndex SearchTransModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex SearchTransModel::parent(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

int SearchTransModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return trans_list_.size();
}

int SearchTransModel::columnCount(const QModelIndex& /*parent*/) const { return info_.search_trans_header.size(); }

QVariant SearchTransModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* trans { trans_list_.at(index.row()) };
    const TableEnumSearch kColumn { index.column() };

    switch (kColumn) {
    case TableEnumSearch::kID:
        return trans->id;
    case TableEnumSearch::kDateTime:
        return trans->date_time;
    case TableEnumSearch::kCode:
        return trans->code;
    case TableEnumSearch::kLhsNode:
        return trans->lhs_node;
    case TableEnumSearch::kLhsRatio:
        return trans->lhs_ratio;
    case TableEnumSearch::kLhsDebit:
        return trans->lhs_debit == 0 ? QVariant() : trans->lhs_debit;
    case TableEnumSearch::kLhsCredit:
        return trans->lhs_credit == 0 ? QVariant() : trans->lhs_credit;
    case TableEnumSearch::kDescription:
        return trans->description;
    case TableEnumSearch::kUnitPrice:
        return trans->unit_price == 0 ? QVariant() : trans->unit_price;
    case TableEnumSearch::kSupportID:
        return trans->support_id == 0 ? QVariant() : trans->support_id;
    case TableEnumSearch::kDiscountPrice:
        return trans->discount_price == 0 ? QVariant() : trans->discount_price;
    case TableEnumSearch::kSettled:
        return trans->settled == 0 ? QVariant() : trans->settled == 0;
    case TableEnumSearch::kRhsNode:
        return trans->rhs_node;
    case TableEnumSearch::kRhsRatio:
        return trans->rhs_ratio;
    case TableEnumSearch::kRhsDebit:
        return trans->rhs_debit == 0 ? QVariant() : trans->rhs_debit;
    case TableEnumSearch::kRhsCredit:
        return trans->rhs_credit == 0 ? QVariant() : trans->rhs_credit;
    case TableEnumSearch::kState:
        return trans->state ? trans->state : QVariant();
    case TableEnumSearch::kDocument:
        return trans->document.isEmpty() ? QVariant() : trans->document.size();
    default:
        return QVariant();
    }
}

QVariant SearchTransModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return info_.search_trans_header.at(section);

    return QVariant();
}

void SearchTransModel::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.search_trans_header.size() - 1)
        return;

    auto Compare = [column, order](const Trans* lhs, const Trans* rhs) -> bool {
        const TableEnumSearch kColumn { column };

        switch (kColumn) {
        case TableEnumSearch::kDateTime:
            return (order == Qt::AscendingOrder) ? (lhs->date_time < rhs->date_time) : (lhs->date_time > rhs->date_time);
        case TableEnumSearch::kCode:
            return (order == Qt::AscendingOrder) ? (lhs->code < rhs->code) : (lhs->code > rhs->code);
        case TableEnumSearch::kLhsNode:
            return (order == Qt::AscendingOrder) ? (lhs->lhs_node < rhs->lhs_node) : (lhs->lhs_node > rhs->lhs_node);
        case TableEnumSearch::kLhsRatio:
            return (order == Qt::AscendingOrder) ? (lhs->lhs_ratio < rhs->lhs_ratio) : (lhs->lhs_ratio > rhs->lhs_ratio);
        case TableEnumSearch::kLhsDebit:
            return (order == Qt::AscendingOrder) ? (lhs->lhs_debit < rhs->lhs_debit) : (lhs->lhs_debit > rhs->lhs_debit);
        case TableEnumSearch::kLhsCredit:
            return (order == Qt::AscendingOrder) ? (lhs->lhs_credit < rhs->lhs_credit) : (lhs->lhs_credit > rhs->lhs_credit);
        case TableEnumSearch::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case TableEnumSearch::kRhsNode:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_node < rhs->rhs_node) : (lhs->rhs_node > rhs->rhs_node);
        case TableEnumSearch::kRhsRatio:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_ratio < rhs->rhs_ratio) : (lhs->rhs_ratio > rhs->rhs_ratio);
        case TableEnumSearch::kRhsDebit:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_debit < rhs->rhs_debit) : (lhs->rhs_debit > rhs->rhs_debit);
        case TableEnumSearch::kRhsCredit:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_credit < rhs->rhs_credit) : (lhs->rhs_credit > rhs->rhs_credit);
        case TableEnumSearch::kState:
            return (order == Qt::AscendingOrder) ? (lhs->state < rhs->state) : (lhs->state > rhs->state);
        case TableEnumSearch::kDocument:
            return (order == Qt::AscendingOrder) ? (lhs->document.size() < rhs->document.size()) : (lhs->document.size() > rhs->document.size());
        case TableEnumSearch::kUnitPrice:
            return (order == Qt::AscendingOrder) ? (lhs->unit_price < rhs->unit_price) : (lhs->unit_price > rhs->unit_price);
        case TableEnumSearch::kSupportID:
            return (order == Qt::AscendingOrder) ? (lhs->support_id < rhs->support_id) : (lhs->support_id > rhs->support_id);
        case TableEnumSearch::kDiscountPrice:
            return (order == Qt::AscendingOrder) ? (lhs->discount_price < rhs->discount_price) : (lhs->discount_price > rhs->discount_price);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(trans_list_.begin(), trans_list_.end(), Compare);
    emit layoutChanged();
}

void SearchTransModel::Query(const QString& text)
{
    beginResetModel();
    if (!trans_list_.isEmpty())
        trans_list_.clear();

    if (!text.isEmpty())
        sql_->SearchTrans(trans_list_, text);
    endResetModel();
}
