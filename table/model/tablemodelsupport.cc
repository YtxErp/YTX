#include "tablemodelsupport.h"

#include "component/constvalue.h"
#include "component/enumclass.h"
#include "global/resourcepool.h"
#include "tablemodelutils.h"

TableModelSupport::TableModelSupport(Sqlite* sql, bool rule, int node_id, CInfo& info, QObject* parent)
    : TableModel { sql, rule, node_id, info, parent }
{
    if (node_id >= 1)
        sql_->ReadSupportTransFPTS(trans_shadow_list_, node_id);
}

void TableModelSupport::RAppendSupportTrans(const TransShadow* trans_shadow)
{
    if (node_id_ != *trans_shadow->support_id)
        return;

    auto* new_trans_shadow { ResourcePool<TransShadow>::Instance().Allocate() };
    new_trans_shadow->date_time = trans_shadow->date_time;
    new_trans_shadow->id = trans_shadow->id;
    new_trans_shadow->description = trans_shadow->description;
    new_trans_shadow->code = trans_shadow->code;
    new_trans_shadow->document = trans_shadow->document;
    new_trans_shadow->state = trans_shadow->state;
    new_trans_shadow->unit_price = trans_shadow->unit_price;
    new_trans_shadow->discount_price = trans_shadow->discount_price;
    new_trans_shadow->settled = trans_shadow->settled;
    new_trans_shadow->support_id = trans_shadow->support_id;

    new_trans_shadow->lhs_ratio = trans_shadow->lhs_ratio;
    new_trans_shadow->lhs_debit = trans_shadow->lhs_debit;
    new_trans_shadow->lhs_credit = trans_shadow->lhs_credit;
    new_trans_shadow->lhs_node = trans_shadow->lhs_node;

    new_trans_shadow->rhs_node = trans_shadow->rhs_node;
    new_trans_shadow->rhs_ratio = trans_shadow->rhs_ratio;
    new_trans_shadow->rhs_debit = trans_shadow->rhs_debit;
    new_trans_shadow->rhs_credit = trans_shadow->rhs_credit;

    auto row { trans_shadow_list_.size() };

    beginInsertRows(QModelIndex(), row, row);
    trans_shadow_list_.emplaceBack(new_trans_shadow);
    endInsertRows();

    // todo 可能需要额外计算
}

void TableModelSupport::RRemoveSupportTrans(int support_id, int trans_id)
{
    if (node_id_ != support_id)
        return;

    auto idx { GetIndex(trans_id) };
    if (!idx.isValid())
        return;

    int row { idx.row() };
    beginRemoveRows(QModelIndex(), row, row);
    ResourcePool<TransShadow>::Instance().Recycle(trans_shadow_list_.takeAt(row));
    endRemoveRows();
}

void TableModelSupport::RAppendMultiSupportTransFPTS(int support_id, const QList<int>& trans_id_list)
{
    if (node_id_ != support_id)
        return;

    auto row { trans_shadow_list_.size() };
    TransShadowList trans_shadow_list {};

    sql_->ReadTransRange(trans_shadow_list, support_id, trans_id_list);
    beginInsertRows(QModelIndex(), row, row + trans_shadow_list.size() - 1);
    trans_shadow_list_.append(trans_shadow_list);
    endInsertRows();
}

QVariant TableModelSupport::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* trans_shadow { trans_shadow_list_.at(index.row()) };
    const TableEnumSupport kColumn { index.column() };

    switch (kColumn) {
    case TableEnumSupport::kID:
        return *trans_shadow->id;
    case TableEnumSupport::kDateTime:
        return *trans_shadow->date_time;
    case TableEnumSupport::kCode:
        return *trans_shadow->code;
    case TableEnumSupport::kLhsNode:
        return *trans_shadow->lhs_node;
    case TableEnumSupport::kLhsRatio:
        return *trans_shadow->lhs_ratio;
    case TableEnumSupport::kLhsDebit:
        return *trans_shadow->lhs_debit == 0 ? QVariant() : *trans_shadow->lhs_debit;
    case TableEnumSupport::kLhsCredit:
        return *trans_shadow->lhs_credit == 0 ? QVariant() : *trans_shadow->lhs_credit;
    case TableEnumSupport::kDescription:
        return *trans_shadow->description;
    case TableEnumSupport::kUnitPrice:
        return *trans_shadow->unit_price == 0 ? QVariant() : *trans_shadow->unit_price;
    case TableEnumSupport::kRhsNode:
        return *trans_shadow->rhs_node;
    case TableEnumSupport::kRhsRatio:
        return *trans_shadow->rhs_ratio;
    case TableEnumSupport::kRhsDebit:
        return *trans_shadow->rhs_debit == 0 ? QVariant() : *trans_shadow->rhs_debit;
    case TableEnumSupport::kRhsCredit:
        return *trans_shadow->rhs_credit == 0 ? QVariant() : *trans_shadow->rhs_credit;
    case TableEnumSupport::kState:
        return *trans_shadow->state ? *trans_shadow->state : QVariant();
    case TableEnumSupport::kDocument:
        return trans_shadow->document->isEmpty() ? QVariant() : trans_shadow->document->size();
    default:
        return QVariant();
    }
}

bool TableModelSupport::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    const TableEnumSupport kColumn { index.column() };
    const int kRow { index.row() };

    auto* trans_shadow { trans_shadow_list_.at(kRow) };

    switch (kColumn) {
    case TableEnumSupport::kDateTime:
        TableModelUtils::UpdateField(sql_, trans_shadow, info_.transaction, value.toString(), kDateTime, &TransShadow::date_time);
        break;
    case TableEnumSupport::kCode:
        TableModelUtils::UpdateField(sql_, trans_shadow, info_.transaction, value.toString(), kCode, &TransShadow::code);
        break;
    case TableEnumSupport::kState:
        TableModelUtils::UpdateField(sql_, trans_shadow, info_.transaction, value.toBool(), kState, &TransShadow::state);
        break;
    case TableEnumSupport::kDescription:
        TableModelUtils::UpdateField(
            sql_, trans_shadow, info_.transaction, value.toString(), kDescription, &TransShadow::description, [this]() { emit SSearch(); });
        break;
    default:
        return false;
    }

    emit SResizeColumnToContents(index.column());
    return true;
}

void TableModelSupport::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.search_trans_header.size() - 1)
        return;

    auto Compare = [column, order](const TransShadow* lhs, const TransShadow* rhs) -> bool {
        const TableEnumSupport kColumn { column };

        switch (kColumn) {
        case TableEnumSupport::kDateTime:
            return (order == Qt::AscendingOrder) ? (*lhs->date_time < *rhs->date_time) : (*lhs->date_time > *rhs->date_time);
        case TableEnumSupport::kCode:
            return (order == Qt::AscendingOrder) ? (*lhs->code < *rhs->code) : (*lhs->code > *rhs->code);
        case TableEnumSupport::kLhsNode:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_node < *rhs->lhs_node) : (*lhs->lhs_node > *rhs->lhs_node);
        case TableEnumSupport::kLhsRatio:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_ratio < *rhs->lhs_ratio) : (*lhs->lhs_ratio > *rhs->lhs_ratio);
        case TableEnumSupport::kLhsDebit:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_debit < *rhs->lhs_debit) : (*lhs->lhs_debit > *rhs->lhs_debit);
        case TableEnumSupport::kLhsCredit:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_credit < *rhs->lhs_credit) : (*lhs->lhs_credit > *rhs->lhs_credit);
        case TableEnumSupport::kDescription:
            return (order == Qt::AscendingOrder) ? (*lhs->description < *rhs->description) : (*lhs->description > *rhs->description);
        case TableEnumSupport::kRhsNode:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_node < *rhs->rhs_node) : (*lhs->rhs_node > *rhs->rhs_node);
        case TableEnumSupport::kRhsRatio:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_ratio < *rhs->rhs_ratio) : (*lhs->rhs_ratio > *rhs->rhs_ratio);
        case TableEnumSupport::kRhsDebit:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_debit < *rhs->rhs_debit) : (*lhs->rhs_debit > *rhs->rhs_debit);
        case TableEnumSupport::kRhsCredit:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_credit < *rhs->rhs_credit) : (*lhs->rhs_credit > *rhs->rhs_credit);
        case TableEnumSupport::kState:
            return (order == Qt::AscendingOrder) ? (*lhs->state < *rhs->state) : (*lhs->state > *rhs->state);
        case TableEnumSupport::kDocument:
            return (order == Qt::AscendingOrder) ? (lhs->document->size() < rhs->document->size()) : (lhs->document->size() > rhs->document->size());
        case TableEnumSupport::kUnitPrice:
            return (order == Qt::AscendingOrder) ? (*lhs->unit_price < *rhs->unit_price) : (*lhs->unit_price > *rhs->unit_price);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(trans_shadow_list_.begin(), trans_shadow_list_.end(), Compare);
    emit layoutChanged();
}

Qt::ItemFlags TableModelSupport::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const TableEnumSupport kColumn { index.column() };

    switch (kColumn) {
    case TableEnumSupport::kCode:
    case TableEnumSupport::kDescription:
    case TableEnumSupport::kDateTime:
        flags |= Qt::ItemIsEditable;
        break;
    default:
        flags &= ~Qt::ItemIsEditable;
        break;
    }

    return flags;
}

QVariant TableModelSupport::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return info_.support_header.at(section);

    return QVariant();
}

int TableModelSupport::columnCount(const QModelIndex& /*parent*/) const { return info_.support_header.size(); }

bool TableModelSupport::RemoveMultiTrans(const QList<int>& trans_id_list)
{
    if (trans_id_list.isEmpty())
        return false;

    int trans_id {};

    for (int i = trans_shadow_list_.size() - 1; i >= 0; --i) {
        trans_id = *trans_shadow_list_.at(i)->id;

        if (trans_id_list.contains(trans_id)) {
            beginRemoveRows(QModelIndex(), i, i);
            ResourcePool<TransShadow>::Instance().Recycle(trans_shadow_list_.takeAt(i));
            endRemoveRows();
        }
    }

    return true;
}
