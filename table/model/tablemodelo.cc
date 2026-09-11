#include "tablemodelo.h"

#include <QJsonArray>

#include "enum/entryenum.h"
#include "global/entrypool.h"
#include "global/masterdataregistry.h"
#include "global/partner_inventory_registry.h"

TableModelO::TableModelO(CTableModelArg& arg, QObject* parent)
    : TableModel { arg, parent }
{
}

TableModelO::~TableModelO() { EntryPool::Instance().Recycle(entry_list_, section_); }

void TableModelO::RAppendEntries(const EntryList& entry_list)
{
    if (entry_list.isEmpty())
        return;

    const auto row { entry_list_.size() };

    beginInsertRows(QModelIndex(), row, row + entry_list.size() - 1);
    entry_list_.append(entry_list);
    endInsertRows();

    sort(std::to_underlying(EntryEnumO::kRhsNode), Qt::AscendingOrder);
}

void TableModelO::Finalize(QJsonObject& message)
{
    // deleted
    {
        QJsonArray deleted_entry_array {};
        for (const auto& id : std::as_const(pending_delete_)) {
            deleted_entry_array.append(id.toString(QUuid::WithoutBraces));
        }
        message.insert(kDeletedEntryArray, deleted_entry_array);
    }

    // insert and update
    {
        QJsonArray inserted_entry_array {};
        QJsonArray updated_entry_array {};

        for (auto* entry : std::as_const(entry_list_)) {
            switch (entry->sync_state) {
            case SyncState::kCreating:
                inserted_entry_array.append(entry->WriteJson());
                break;
            case SyncState::kUpdating:
                updated_entry_array.append(entry->WriteJson());
                break;
            case SyncState::kSynced:
            case SyncState::kDeleting:
            case SyncState::kError:
                break;
            }

            entry->sync_state = SyncState::kSynced;
        }

        message.insert(kInsertedEntryArray, inserted_entry_array);
        message.insert(kUpdatedEntryArray, updated_entry_array);
    }

    // clear
    pending_delete_.clear();
}

bool TableModelO::HasZeroUnitPrice() const
{
    for (const Entry* entry : entry_list_) {
        const auto* d_entry { static_cast<const EntryO*>(entry) };

        if (qFuzzyIsNull(d_entry->unit_price))
            return true;
    }

    return false;
}

bool TableModelO::HasPendingUpdate() const
{
    if (!pending_delete_.isEmpty())
        return true;

    for (auto* entry : std::as_const(entry_list_)) {
        if (entry->sync_state == SyncState::kCreating || entry->sync_state == SyncState::kUpdating)
            return true;
    }
    return false;
}

QModelIndex TableModelO::GetIndex(const QUuid& entry_id) const
{
    int row { 0 };

    for (const auto* entry : entry_list_) {
        if (entry->id == entry_id) {
            return index(row, 0);
        }
        ++row;
    }

    return QModelIndex();
}

QVariant TableModelO::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    const EntryEnumO column { index.column() };
    auto* d_entry { static_cast<EntryO*>(index.internalPointer()) };

    switch (column) {
    case EntryEnumO::kLhsNode:
        return d_entry->lhs_node;
    case EntryEnumO::kRhsNode:
        return d_entry->rhs_node;
    case EntryEnumO::kUnitPrice:
        return d_entry->unit_price;
    case EntryEnumO::kMeasure:
        return d_entry->measure;
    case EntryEnumO::kDescription:
        return d_entry->description;
    case EntryEnumO::kCount:
        return d_entry->count;
    case EntryEnumO::kInitial:
        return d_entry->initial;
    case EntryEnumO::kExternalSku:
        return PartnerInventoryRegistry::Instance().ExternalSku(d_node_->partner_id, d_entry->rhs_node);
    case EntryEnumO::kTag:
        return d_entry->tag;
    case EntryEnumO::kStatus:
        return d_entry->status;
    }
}

bool TableModelO::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    if (data(index, role) == value)
        return false;

    const EntryEnumO column { index.column() };

    auto* entry { static_cast<Entry*>(index.internalPointer()) };

    if (column == EntryEnumO::kStatus) {
        entry->status = value.toInt();

        emit dataChanged(index, index, { Qt::DisplayRole, Qt::EditRole });
        return true;
    }

    if (d_node_->order_status == OrderStatus::kReleased)
        return false;

    auto* d_entry { static_cast<EntryO*>(entry) };
    const double old_count { d_entry->count };
    const double old_measure { d_entry->measure };
    const double old_initial { d_entry->initial };
    const int row { index.row() };

    switch (column) {
    case EntryEnumO::kDescription:
        UpdateDescription(d_entry, value.toString());
        break;
    case EntryEnumO::kRhsNode:
        UpdateInternalSku(entry, value.toUuid(), row);
        break;
    case EntryEnumO::kUnitPrice:
        UpdateUnitPrice(d_entry, value.toDouble());
        break;
    case EntryEnumO::kMeasure:
        UpdateMeasure(d_entry, value.toDouble());
        break;
    case EntryEnumO::kCount:
        UpdateCount(d_entry, value.toDouble());
        break;
    case EntryEnumO::kTag:
        UpdateTag(d_entry, value.toStringList());
        break;
    case EntryEnumO::kStatus:
    case EntryEnumO::kLhsNode:
    case EntryEnumO::kExternalSku:
    case EntryEnumO::kInitial:
        return false;
    }

    emit dataChanged(index, index, { Qt::DisplayRole, Qt::EditRole });

    const double count_delta { d_entry->count - old_count };
    const double measure_delta { d_entry->measure - old_measure };
    const double initial_delta { d_entry->initial - old_initial };

    if (!qFuzzyIsNull(initial_delta) || !qFuzzyIsNull(count_delta) || !qFuzzyIsNull(measure_delta)) {
        emit SSyncDeltaO(d_entry->lhs_node, initial_delta, count_delta, measure_delta);
    }

    if (!qFuzzyIsNull(initial_delta))
        EmitDataChanged(row, row, std::to_underlying(EntryEnumO::kInitial), std::to_underlying(EntryEnumO::kInitial));

    return true;
}

void TableModelO::sort(int column, Qt::SortOrder order)
{
    const EntryEnumO e_column { column };

    auto Compare = [order, e_column](Entry* lhs, Entry* rhs) -> bool {
        auto* d_lhs { DerivedPtr<EntryO>(lhs) };
        auto* d_rhs { DerivedPtr<EntryO>(rhs) };

        switch (e_column) {
        case EntryEnumO::kDescription:
            return utils::CompareMember(lhs, rhs, &Entry::description, order);
        case EntryEnumO::kRhsNode:
            return utils::CompareMember(lhs, rhs, &EntryO::rhs_node, order);
        case EntryEnumO::kUnitPrice:
            return utils::CompareMember(d_lhs, d_rhs, &EntryO::unit_price, order);
        case EntryEnumO::kCount:
            return utils::CompareMember(d_lhs, d_rhs, &EntryO::count, order);
        case EntryEnumO::kMeasure:
            return utils::CompareMember(d_lhs, d_rhs, &EntryO::measure, order);
        case EntryEnumO::kInitial:
            return utils::CompareMember(d_lhs, d_rhs, &EntryO::initial, order);
        case EntryEnumO::kTag:
            return utils::CompareMember(lhs, rhs, &EntryP::tag, order);
        case EntryEnumO::kStatus:
            return utils::CompareMember(lhs, rhs, &Entry::status, order);
        case EntryEnumO::kLhsNode:
        case EntryEnumO::kExternalSku:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::ranges::sort(entry_list_, Compare);
    emit layoutChanged();
}

Qt::ItemFlags TableModelO::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const EntryEnumO column { index.column() };

    switch (column) {
    case EntryEnumO::kLhsNode:
    case EntryEnumO::kInitial:
    case EntryEnumO::kTag:
    case EntryEnumO::kExternalSku:
    case EntryEnumO::kStatus:
        flags &= ~Qt::ItemIsEditable;
        break;
    case EntryEnumO::kRhsNode:
    case EntryEnumO::kDescription:
    case EntryEnumO::kCount:
    case EntryEnumO::kMeasure:
    case EntryEnumO::kUnitPrice:
        flags |= Qt::ItemIsEditable;
        break;
    }

    if (d_node_->order_status == OrderStatus::kReleased)
        flags &= ~Qt::ItemIsEditable;

    return flags;
}

QModelIndex TableModelO::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column, entry_list_.at(row));
}

bool TableModelO::insertRows(int row, int /*count*/, const QModelIndex& parent)
{
    Q_ASSERT(row >= 0 && row <= rowCount(parent));
    if (d_node_->order_status == OrderStatus::kReleased)
        return false;

    auto* entry { EntryPool::Instance().Allocate(section_) };
    entry->id = QUuid::createUuidV7();
    entry->lhs_node = node_id_;
    entry->issued_time = QDateTime::currentDateTime();

    beginInsertRows(parent, row, row);
    entry_list_.insert(row, entry);
    endInsertRows();

    return true;
}

bool TableModelO::removeRows(int row, int /*count*/, const QModelIndex& parent)
{
    Q_ASSERT(row >= 0 && row <= rowCount(parent) - 1);
    if (d_node_->order_status == OrderStatus::kReleased)
        return false;

    auto* d_entry = DerivedPtr<EntryO>(entry_list_.at(row));

    const auto lhs_node { d_entry->lhs_node };
    const auto rhs_node { d_entry->rhs_node };
    const auto entry_id { d_entry->id };

    beginRemoveRows(parent, row, row);
    entry_list_.removeAt(row);
    endRemoveRows();

    if (!rhs_node.isNull()) {
        const double count_delta { -d_entry->count };
        const double measure_delta { -d_entry->measure };
        const double initial_delta { -d_entry->initial };

        if (!qFuzzyIsNull(count_delta) || !qFuzzyIsNull(measure_delta) || !qFuzzyIsNull(initial_delta)) {
            emit SSyncDeltaO(lhs_node, initial_delta, count_delta, measure_delta);
        }
    }

    if (d_entry->sync_state != SyncState::kCreating)
        pending_delete_.insert(entry_id);

    EntryPool::Instance().Recycle(d_entry, section_);
    return true;
}

/// @brief Update entry by internal product ID (rhs_node)
/// @note Responsibility: Handle insertion and update, not deletion
/// @note rhs_node must be valid, external_sku is auto-filled
bool TableModelO::UpdateInternalSku(Entry* entry, const QUuid& value, int row)
{
    if (value.isNull())
        return false;

    auto* d_entry { static_cast<EntryO*>(entry) };

    auto old_rhs_node { d_entry->rhs_node };
    if (old_rhs_node == value)
        return false;

    const double old_unit_price { d_entry->unit_price };

    d_entry->rhs_node = value;

    const auto unit_price { PartnerInventoryRegistry::Instance().UnitPrice(d_node_->partner_id, value) };
    d_entry->unit_price = unit_price.value_or(MasterDataRegistry::Instance().InventoryUnitPrice(value));

    const bool price_changed { !qFuzzyIsNull(old_unit_price - d_entry->unit_price) };

    if (price_changed) {
        d_entry->initial = d_entry->measure * d_entry->unit_price;
    }

    if (d_entry->sync_state == SyncState::kSynced)
        d_entry->sync_state = SyncState::kUpdating;

    const auto& registry { PartnerInventoryRegistry::Instance() };

    if (registry.ExternalSku(d_node_->partner_id, old_rhs_node) != registry.ExternalSku(d_node_->partner_id, value)) {
        EmitDataChanged(row, row, std::to_underlying(EntryEnumO::kExternalSku), std::to_underlying(EntryEnumO::kExternalSku));
    }

    if (price_changed) {
        EmitDataChanged(row, row, std::to_underlying(EntryEnumO::kUnitPrice), std::to_underlying(EntryEnumO::kInitial));
    }

    return price_changed;
}

bool TableModelO::UpdateUnitPrice(EntryO* entry, double value)
{
    if (qFuzzyCompare(entry->unit_price, value))
        return false;

    entry->initial = entry->measure * value;
    entry->unit_price = value;

    if (entry->sync_state == SyncState::kSynced)
        entry->sync_state = SyncState::kUpdating;

    return true;
}

bool TableModelO::UpdateMeasure(EntryO* entry, double value)
{
    if (qFuzzyCompare(entry->measure, value))
        return false;

    entry->initial = entry->unit_price * value;
    entry->measure = value;

    if (entry->sync_state == SyncState::kSynced)
        entry->sync_state = SyncState::kUpdating;

    return true;
}

bool TableModelO::UpdateCount(EntryO* entry, double value)
{
    if (qFuzzyCompare(entry->count, value))
        return false;

    entry->count = value;

    if (entry->sync_state == SyncState::kSynced)
        entry->sync_state = SyncState::kUpdating;

    return true;
}

bool TableModelO::UpdateDescription(EntryO* entry, const QString& value)
{
    if (entry->description == value)
        return false;

    entry->description = value;

    if (entry->sync_state == SyncState::kSynced)
        entry->sync_state = SyncState::kUpdating;

    return true;
}

bool TableModelO::UpdateTag(EntryO* entry, const QStringList& value)
{
    if (entry->tag == value)
        return false;

    entry->tag = value;

    if (entry->sync_state == SyncState::kSynced)
        entry->sync_state = SyncState::kUpdating;

    return true;
}

// Purify newly inserted entries:
// - Entries with null `rhs_node` (internal SKU not selected) are removed.
// - The entry objects are recycled via EntryPool.
void TableModelO::Purify()
{
    for (auto i = entry_list_.size() - 1; i >= 0; --i) {
        auto* entry { entry_list_[i] };

        if (!entry->rhs_node.isNull())
            continue;

        beginRemoveRows(QModelIndex(), i, i);
        EntryPool::Instance().Recycle(entry_list_.takeAt(i), section_);
        endRemoveRows();
    }
}
