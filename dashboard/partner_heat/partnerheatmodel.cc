#include "partnerheatmodel.h"

#include <QJsonArray>

#include "global/resourcepool.h"
#include "partnerheatenum.h"
#include "utils/templateutils.h"

namespace partner_heat {

Model::Model(const QStringList& header, QObject* parent)
    : QAbstractItemModel(parent)
    , header_ { header }
{
}

Model::~Model() { ResourcePool<Row>::Instance().Recycle(list_); }

QVariant Model::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return header_.at(section);

    return QVariant();
}

QModelIndex Model::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column, list_.at(row));
}

QVariant Model::data(const QModelIndex& index, int role) const
{
    // Basic validation to prevent out-of-bounds access
    if (!index.isValid() || index.row() >= list_.size()) {
        return {};
    }

    // Only respond to display and edit roles
    if (role != Qt::DisplayRole && role != Qt::EditRole) {
        return {};
    }

    const RowField column { index.column() };
    auto* entry { static_cast<Row*>(index.internalPointer()) };

    switch (column) {
    case RowField::kPartnerNode:
        return entry->partner_node.toString(QUuid::WithoutBraces);
    case RowField::kOrderCount:
        return entry->order_count;
    case RowField::kInventoryDiversity:
        return entry->inventory_diversity;
    case RowField::kActiveMonths:
        return entry->active_months;
    case RowField::kActiveDays:
        return entry->active_days;
    case RowField::kTotalQuantity:
        return entry->total_quantity;
    case RowField::kHeatScore:
        return entry->heat_score;
    case RowField::kPlaceholder:
        return {};
    }
}

void Model::sort(int column, Qt::SortOrder order)
{
    // Convert integer column to the structured enum using brace initialization
    const RowField e_column { column };

    // Define a lambda for comparison based on the selected column and sort order
    auto Compare = [order, e_column](const Row* lhs, const Row* rhs) -> bool {
        switch (e_column) {
        case RowField::kPartnerNode:
            return utils::CompareMember(lhs, rhs, &Row::partner_node, order);
        case RowField::kOrderCount:
            return utils::CompareMember(lhs, rhs, &Row::order_count, order);
        case RowField::kInventoryDiversity:
            return utils::CompareMember(lhs, rhs, &Row::inventory_diversity, order);
        case RowField::kActiveMonths:
            return utils::CompareMember(lhs, rhs, &Row::active_months, order);
        case RowField::kActiveDays:
            return utils::CompareMember(lhs, rhs, &Row::active_days, order);
        case RowField::kTotalQuantity:
            return utils::CompareMember(lhs, rhs, &Row::total_quantity, order);
        case RowField::kHeatScore:
            return utils::CompareMember(lhs, rhs, &Row::heat_score, order);
        case RowField::kPlaceholder:
            return false;
        }
        return false;
    };

    // Notify the view that the layout is about to change
    emit layoutAboutToBeChanged();

    // Perform the sort on the underlying data list
    std::ranges::sort(list_, Compare);

    // Notify the view that the layout has been updated
    emit layoutChanged();
}

void Model::Rebuild(const QJsonArray& array)
{
    if (array.isEmpty()) {
        qDebug() << Q_FUNC_INFO << "Received empty member array";
    }

    // Parse outside the reset block
    QList<Row*> new_list {};
    new_list.reserve(array.size());

    for (const auto& value : array) {
        Q_ASSERT(value.isObject());

        auto* member { ResourcePool<Row>::Instance().Allocate() };
        member->ReadJson(value.toObject());
        new_list.emplaceBack(member);
    }

    std::ranges::sort(new_list, [](const auto* lhs, const auto* rhs) { return utils::CompareMember(lhs, rhs, &Row::heat_score, Qt::DescendingOrder); });

    // Keep reset block as short as possible
    beginResetModel();
    ResourcePool<Row>::Instance().Recycle(list_);
    list_ = std::move(new_list);
    endResetModel();
}
}
