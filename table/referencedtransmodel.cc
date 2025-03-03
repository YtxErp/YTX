#include "referencedtransmodel.h"

#include "component/enumclass.h"

ReferencedTransModel::ReferencedTransModel(int node_id, CInfo& info, Sqlite* sql, QObject* parent)
    : QAbstractItemModel { parent }
    , sql_ { sql }
    , info_ { info }
    , node_id_ { node_id }
{
    Query(node_id);
}

ReferencedTransModel::~ReferencedTransModel() { trans_list_.clear(); }

QModelIndex ReferencedTransModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex ReferencedTransModel::parent(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

int ReferencedTransModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return trans_list_.size();
}

int ReferencedTransModel::columnCount(const QModelIndex& /*parent*/) const { return info_.reference_header.size(); }

QVariant ReferencedTransModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* trans { trans_list_.at(index.row()) };
    const TableEnumReferenceP kColumn { index.column() };

    switch (kColumn) {
    case TableEnumReferenceP::kDateTime:
        return trans->date_time;
    case TableEnumReferenceP::kParty:
        return trans->id;
    case TableEnumReferenceP::kLhsNode:
        return trans->lhs_node;
    case TableEnumReferenceP::kFirst:
        return trans->lhs_debit == 0 ? QVariant() : trans->lhs_debit;
    case TableEnumReferenceP::kSecond:
        return trans->lhs_credit == 0 ? QVariant() : trans->lhs_credit;
    case TableEnumReferenceP::kUnitPrice:
        return trans->lhs_ratio == 0 ? QVariant() : trans->lhs_ratio;
    case TableEnumReferenceP::kDiscountPrice:
        return trans->rhs_ratio == 0 ? QVariant() : trans->rhs_ratio;
    case TableEnumReferenceP::kDescription:
        return trans->description;
    case TableEnumReferenceP::kCode:
        return trans->code;
    case TableEnumReferenceP::kGrossAmount:
        return trans->rhs_debit == 0 ? QVariant() : trans->rhs_debit;
    case TableEnumReferenceP::kDiscount:
        return trans->discount == 0 ? QVariant() : trans->discount;
    case TableEnumReferenceP::kNetAmount:
        return trans->rhs_credit == 0 ? QVariant() : trans->rhs_credit;
    default:
        return QVariant();
    }
}

QVariant ReferencedTransModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return info_.reference_header.at(section);

    return QVariant();
}

void ReferencedTransModel::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.reference_header.size() - 1)
        return;

    auto Compare = [column, order](const Trans* lhs, const Trans* rhs) -> bool {
        const TableEnumReferenceP kColumn { column };

        switch (kColumn) {
        case TableEnumReferenceP::kDateTime:
            return (order == Qt::AscendingOrder) ? (lhs->date_time < rhs->date_time) : (lhs->date_time > rhs->date_time);
        case TableEnumReferenceP::kCode:
            return (order == Qt::AscendingOrder) ? (lhs->code < rhs->code) : (lhs->code > rhs->code);
        case TableEnumReferenceP::kParty:
            return (order == Qt::AscendingOrder) ? (lhs->id < rhs->id) : (lhs->id > rhs->id);
        case TableEnumReferenceP::kUnitPrice:
            return (order == Qt::AscendingOrder) ? (lhs->lhs_ratio < rhs->lhs_ratio) : (lhs->lhs_ratio > rhs->lhs_ratio);
        case TableEnumReferenceP::kFirst:
            return (order == Qt::AscendingOrder) ? (lhs->lhs_debit < rhs->lhs_debit) : (lhs->lhs_debit > rhs->lhs_debit);
        case TableEnumReferenceP::kSecond:
            return (order == Qt::AscendingOrder) ? (lhs->lhs_credit < rhs->lhs_credit) : (lhs->lhs_credit > rhs->lhs_credit);
        case TableEnumReferenceP::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case TableEnumReferenceP::kDiscount:
            return (order == Qt::AscendingOrder) ? (lhs->discount < rhs->discount) : (lhs->discount > rhs->discount);
        case TableEnumReferenceP::kNetAmount:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_credit < rhs->rhs_credit) : (lhs->rhs_credit > rhs->rhs_credit);
        case TableEnumReferenceP::kGrossAmount:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_debit < rhs->rhs_debit) : (lhs->rhs_debit > rhs->rhs_debit);
        case TableEnumReferenceP::kDiscountPrice:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_ratio < rhs->rhs_ratio) : (lhs->rhs_ratio > rhs->rhs_ratio);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(trans_list_.begin(), trans_list_.end(), Compare);
    emit layoutChanged();
}

void ReferencedTransModel::Query(int node_id)
{
    beginResetModel();
    if (!trans_list_.isEmpty())
        trans_list_.clear();

    if (node_id >= 1)
        sql_->ReadReferencedTrans(trans_list_, node_id);

    endResetModel();
}
