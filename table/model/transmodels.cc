#include "transmodels.h"

#include <QDateTime>

#include "component/constvalue.h"
#include "global/resourcepool.h"
#include "transmodelutils.h"

TransModelS::TransModelS(CTransModelArg& arg, QObject* parent)
    : TransModel { arg, parent }
{
    assert(node_id_ >= 1 && "Assertion failed: node_id_ must be positive (>= 1)");
    sql_->ReadTrans(trans_shadow_list_, node_id_);
    IniInsideSet();
}

bool TransModelS::removeRows(int row, int /*count*/, const QModelIndex& parent)
{
    assert(row >= 0 && row <= rowCount(parent) - 1 && "Row must be in the valid range [0, rowCount(parent) - 1]");

    auto* trans_shadow { trans_shadow_list_.at(row) };
    int rhs_node_id { *trans_shadow->rhs_node };

    beginRemoveRows(parent, row, row);
    trans_shadow_list_.removeAt(row);
    endRemoveRows();

    if (rhs_node_id != 0) {
        if (int support_id = *trans_shadow->support_id; support_id != 0)
            emit SRemoveOneTransS(section_, support_id, *trans_shadow->id);

        sql_->RemoveTrans(*trans_shadow->id);
    }

    inside_set_.remove(rhs_node_id);
    ResourcePool<TransShadow>::Instance().Recycle(trans_shadow);
    return true;
}

bool TransModelS::UpdateRatio(TransShadow* trans_shadow, double value)
{
    double unit_price { *trans_shadow->lhs_ratio };
    if (std::abs(unit_price - value) < kTolerance || value < 0)
        return false;

    *trans_shadow->lhs_ratio = value;
    *trans_shadow->rhs_ratio = value;

    if (*trans_shadow->rhs_node == 0)
        return false;

    sql_->WriteField(info_.trans, kUnitPrice, value, *trans_shadow->id);
    return true;
}

bool TransModelS::UpdateInsideProduct(TransShadow* trans_shadow, int value)
{
    if (*trans_shadow->rhs_node == value || inside_set_.contains(value))
        return false;

    inside_set_.remove(*trans_shadow->rhs_node);
    *trans_shadow->rhs_node = value;
    inside_set_.insert(value);
    return true;
}

void TransModelS::IniInsideSet()
{
    std::ranges::for_each(trans_shadow_list_, [this](const auto* shadow) {
        if (shadow->rhs_node) {
            inside_set_.insert(*shadow->rhs_node);
        }
    });
}

QVariant TransModelS::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* trans_shadow { trans_shadow_list_.at(index.row()) };
    const TransEnumS kColumn { index.column() };

    switch (kColumn) {
    case TransEnumS::kID:
        return *trans_shadow->id;
    case TransEnumS::kDateTime:
        return *trans_shadow->date_time;
    case TransEnumS::kCode:
        return *trans_shadow->code;
    case TransEnumS::kUnitPrice:
        return *trans_shadow->lhs_ratio == 0 ? QVariant() : *trans_shadow->lhs_ratio;
    case TransEnumS::kDescription:
        return *trans_shadow->description;
    case TransEnumS::kDocument:
        return trans_shadow->document->isEmpty() ? QVariant() : trans_shadow->document->size();
    case TransEnumS::kState:
        return *trans_shadow->state ? *trans_shadow->state : QVariant();
    case TransEnumS::kInsideProduct:
        return *trans_shadow->rhs_node == 0 ? QVariant() : *trans_shadow->rhs_node;
    case TransEnumS::kOutsideProduct:
        return *trans_shadow->support_id == 0 ? QVariant() : *trans_shadow->support_id;
    default:
        return QVariant();
    }
}

bool TransModelS::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    const TransEnumS kColumn { index.column() };
    const int kRow { index.row() };

    auto* trans_shadow { trans_shadow_list_.at(kRow) };
    int old_rhs_node { *trans_shadow->rhs_node };
    int old_sup_node { *trans_shadow->support_id };

    bool rhs_changed { false };
    bool sup_changed { false };

    switch (kColumn) {
    case TransEnumS::kDateTime:
        TransModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kDateTime, value.toString(), &TransShadow::date_time);
        break;
    case TransEnumS::kCode:
        TransModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kCode, value.toString(), &TransShadow::code);
        break;
    case TransEnumS::kInsideProduct:
        rhs_changed = UpdateInsideProduct(trans_shadow, value.toInt());
        break;
    case TransEnumS::kUnitPrice:
        UpdateRatio(trans_shadow, value.toDouble());
        break;
    case TransEnumS::kDescription:
        TransModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kDescription, value.toString(), &TransShadow::description, [this]() { emit SSearch(); });
        break;
    case TransEnumS::kState:
        TransModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kState, value.toBool(), &TransShadow::state);
        break;
    case TransEnumS::kOutsideProduct:
        sup_changed = TransModelUtils::UpdateField(sql_, trans_shadow, info_.trans, kOutsideProduct, value.toInt(), &TransShadow::support_id);
        break;
    default:
        return false;
    }

    if (rhs_changed) {
        if (old_rhs_node == 0) {
            sql_->WriteTrans(trans_shadow);

            if (*trans_shadow->support_id != 0) {
                emit SAppendOneTransS(section_, *trans_shadow->support_id, *trans_shadow->id);
            }
        } else
            sql_->WriteField(info_.trans, kInsideProduct, value.toInt(), *trans_shadow->id);
    }

    if (sup_changed) {
        if (old_sup_node != 0)
            emit SRemoveOneTransS(section_, old_sup_node, *trans_shadow->id);

        if (*trans_shadow->support_id != 0 && *trans_shadow->id != 0) {
            emit SAppendOneTransS(section_, *trans_shadow->support_id, *trans_shadow->id);
        }
    }

    emit SResizeColumnToContents(index.column());
    return true;
}

void TransModelS::sort(int column, Qt::SortOrder order)
{
    assert(column >= 0 && "Column index out of range");
    if (column >= info_.trans_header.size() - 1)
        return;

    auto Compare = [column, order](TransShadow* lhs, TransShadow* rhs) -> bool {
        const TransEnumS kColumn { column };

        switch (kColumn) {
        case TransEnumS::kDateTime:
            return (order == Qt::AscendingOrder) ? (*lhs->date_time < *rhs->date_time) : (*lhs->date_time > *rhs->date_time);
        case TransEnumS::kCode:
            return (order == Qt::AscendingOrder) ? (*lhs->code < *rhs->code) : (*lhs->code > *rhs->code);
        case TransEnumS::kUnitPrice:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_ratio < *rhs->lhs_ratio) : (*lhs->lhs_ratio > *rhs->lhs_ratio);
        case TransEnumS::kDescription:
            return (order == Qt::AscendingOrder) ? (*lhs->description < *rhs->description) : (*lhs->description > *rhs->description);
        case TransEnumS::kDocument:
            return (order == Qt::AscendingOrder) ? (lhs->document->size() < rhs->document->size()) : (lhs->document->size() > rhs->document->size());
        case TransEnumS::kState:
            return (order == Qt::AscendingOrder) ? (*lhs->state < *rhs->state) : (*lhs->state > *rhs->state);
        case TransEnumS::kOutsideProduct:
            return (order == Qt::AscendingOrder) ? (*lhs->support_id < *rhs->support_id) : (*lhs->support_id > *rhs->support_id);
        case TransEnumS::kInsideProduct:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_node < *rhs->rhs_node) : (*lhs->rhs_node > *rhs->rhs_node);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(trans_shadow_list_.begin(), trans_shadow_list_.end(), Compare);
    emit layoutChanged();
}

Qt::ItemFlags TransModelS::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const TransEnumS kColumn { index.column() };

    switch (kColumn) {
    case TransEnumS::kID:
    case TransEnumS::kDocument:
    case TransEnumS::kState:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
    }

    return flags;
}
