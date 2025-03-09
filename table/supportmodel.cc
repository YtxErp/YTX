#include "supportmodel.h"

#include "component/constvalue.h"
#include "component/enumclass.h"
#include "global/resourcepool.h"
#include "model/transmodelutils.h"

SupportModel::SupportModel(Sqlite* sql, bool rule, int node_id, CInfo& info, QObject* parent)
    : QAbstractItemModel { parent }
    , sql_ { sql }
    , rule_ { rule }
    , info_ { info }
    , node_id_ { node_id }
{
    if (node_id >= 1)
        sql_->ReadSupportTrans(trans_shadow_list_, node_id);
}

SupportModel::~SupportModel() { ResourcePool<TransShadow>::Instance().Recycle(trans_shadow_list_); }

void SupportModel::RAppendSupportTrans(const TransShadow* trans_shadow)
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
    new_trans_shadow->discount = trans_shadow->discount;
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
}

void SupportModel::RRemoveSupportTrans(int support_id, int trans_id)
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

void SupportModel::RAppendMultiSupportTrans(int support_id, const QList<int>& trans_id_list)
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

    auto* trans_shadow { trans_shadow_list_.at(index.row()) };
    const TransSearchEnum kColumn { index.column() };

    switch (kColumn) {
    case TransSearchEnum::kID:
        return *trans_shadow->id;
    case TransSearchEnum::kDateTime:
        return *trans_shadow->date_time;
    case TransSearchEnum::kCode:
        return *trans_shadow->code;
    case TransSearchEnum::kLhsNode:
        return *trans_shadow->lhs_node;
    case TransSearchEnum::kLhsRatio:
        return *trans_shadow->lhs_ratio == 0 ? QVariant() : *trans_shadow->lhs_ratio;
    case TransSearchEnum::kLhsDebit:
        return *trans_shadow->lhs_debit == 0 ? QVariant() : *trans_shadow->lhs_debit;
    case TransSearchEnum::kLhsCredit:
        return *trans_shadow->lhs_credit == 0 ? QVariant() : *trans_shadow->lhs_credit;
    case TransSearchEnum::kDescription:
        return *trans_shadow->description;
    case TransSearchEnum::kRhsNode:
        return *trans_shadow->rhs_node;
    case TransSearchEnum::kRhsRatio:
        return *trans_shadow->rhs_ratio == 0 ? QVariant() : *trans_shadow->rhs_ratio;
    case TransSearchEnum::kRhsDebit:
        return *trans_shadow->rhs_debit == 0 ? QVariant() : *trans_shadow->rhs_debit;
    case TransSearchEnum::kRhsCredit:
        return *trans_shadow->rhs_credit == 0 ? QVariant() : *trans_shadow->rhs_credit;
    case TransSearchEnum::kState:
        return *trans_shadow->state ? *trans_shadow->state : QVariant();
    case TransSearchEnum::kDocument:
        return trans_shadow->document->isEmpty() ? QVariant() : trans_shadow->document->size();
    default:
        return QVariant();
    }
}

bool SupportModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    const TransSearchEnum kColumn { index.column() };
    const int kRow { index.row() };

    auto* trans_shadow { trans_shadow_list_.at(kRow) };

    switch (kColumn) {
    case TransSearchEnum::kState:
        TransModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kState, value.toBool(), &TransShadow::state);
        break;
    default:
        return false;
    }

    return true;
}

void SupportModel::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.search_trans_header.size() - 1)
        return;

    auto Compare = [column, order](const TransShadow* lhs, const TransShadow* rhs) -> bool {
        const TransSearchEnum kColumn { column };

        switch (kColumn) {
        case TransSearchEnum::kDateTime:
            return (order == Qt::AscendingOrder) ? (*lhs->date_time < *rhs->date_time) : (*lhs->date_time > *rhs->date_time);
        case TransSearchEnum::kCode:
            return (order == Qt::AscendingOrder) ? (*lhs->code < *rhs->code) : (*lhs->code > *rhs->code);
        case TransSearchEnum::kLhsNode:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_node < *rhs->lhs_node) : (*lhs->lhs_node > *rhs->lhs_node);
        case TransSearchEnum::kLhsRatio:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_ratio < *rhs->lhs_ratio) : (*lhs->lhs_ratio > *rhs->lhs_ratio);
        case TransSearchEnum::kLhsDebit:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_debit < *rhs->lhs_debit) : (*lhs->lhs_debit > *rhs->lhs_debit);
        case TransSearchEnum::kLhsCredit:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_credit < *rhs->lhs_credit) : (*lhs->lhs_credit > *rhs->lhs_credit);
        case TransSearchEnum::kDescription:
            return (order == Qt::AscendingOrder) ? (*lhs->description < *rhs->description) : (*lhs->description > *rhs->description);
        case TransSearchEnum::kRhsNode:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_node < *rhs->rhs_node) : (*lhs->rhs_node > *rhs->rhs_node);
        case TransSearchEnum::kRhsRatio:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_ratio < *rhs->rhs_ratio) : (*lhs->rhs_ratio > *rhs->rhs_ratio);
        case TransSearchEnum::kRhsDebit:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_debit < *rhs->rhs_debit) : (*lhs->rhs_debit > *rhs->rhs_debit);
        case TransSearchEnum::kRhsCredit:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_credit < *rhs->rhs_credit) : (*lhs->rhs_credit > *rhs->rhs_credit);
        case TransSearchEnum::kState:
            return (order == Qt::AscendingOrder) ? (*lhs->state < *rhs->state) : (*lhs->state > *rhs->state);
        case TransSearchEnum::kDocument:
            return (order == Qt::AscendingOrder) ? (lhs->document->size() < rhs->document->size()) : (lhs->document->size() > rhs->document->size());
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(trans_shadow_list_.begin(), trans_shadow_list_.end(), Compare);
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

bool SupportModel::RemoveMultiSupportTrans(const QMultiHash<int, int>& node_trans)
{
    if (!node_trans.contains(node_id_))
        return false;

    const auto lsit { node_trans.values(node_id_) };

    for (int i = trans_shadow_list_.size() - 1; i >= 0; --i) {
        int trans_id { *trans_shadow_list_.at(i)->id };

        if (lsit.contains(trans_id)) {
            beginRemoveRows(QModelIndex(), i, i);
            ResourcePool<TransShadow>::Instance().Recycle(trans_shadow_list_.takeAt(i));
            endRemoveRows();
        }
    }

    return true;
}

QModelIndex SupportModel::GetIndex(int trans_id) const
{
    int row { 0 };

    for (const auto* trans_shadow : trans_shadow_list_) {
        if (*trans_shadow->id == trans_id) {
            return index(row, 0);
        }
        ++row;
    }
    return QModelIndex();
}
