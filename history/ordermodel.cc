#include "ordermodel.h"

#include <QJsonArray>

#include "global/resourcepool.h"
#include "utils/templateutils.h"

namespace history {

OrderModel::OrderModel(CSectionInfo& info, QObject* parent)
    : QAbstractItemModel { parent }
    , info_ { info }
{
}

OrderModel::~OrderModel() { ResourcePool<OrderRow>::Instance().Recycle(list_); }

QModelIndex OrderModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column, list_.at(row));
}

QModelIndex OrderModel::parent(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

int OrderModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return list_.size();
}

int OrderModel::columnCount(const QModelIndex& /*parent*/) const { return info_.node_referenced_header.size(); }

QVariant OrderModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return info_.node_referenced_header.at(section);

    return QVariant();
}

void OrderModel::Rebuild(const QJsonArray& array)
{
    if (array.isEmpty()) {
        qDebug() << Q_FUNC_INFO << "Received empty array";
    }

    QList<OrderRow*> new_list {};
    new_list.reserve(array.size());

    for (const auto& value : array) {
        Q_ASSERT(value.isObject());

        auto* reference { ResourcePool<OrderRow>::Instance().Allocate() };
        reference->ReadJson(value.toObject());

        new_list.emplaceBack(reference);
    }

    std::ranges::sort(new_list, [](const auto* lhs, const auto* rhs) { return utils::CompareMember(lhs, rhs, &OrderRow::issued_time, Qt::DescendingOrder); });

    beginResetModel();

    ResourcePool<OrderRow>::Instance().Recycle(list_);
    list_ = std::move(new_list);

    endResetModel();
}
}
