#include "entryhub.h"

#include <QJsonArray>

#include "component/constant.h"
#include "global/entrypool.h"
#include "utils/entryutils.h"
#include "websocket/jsongen.h"

EntryHub::EntryHub(CSectionInfo& info, QObject* parent)
    : QObject(parent)
    , section_ { info.section }
    , info_ { info }
{
}

EntryHub::~EntryHub() { Reset(); }

void EntryHub::DeleteDoubleLeaf(const QHash<QUuid, QSet<QUuid>>& leaf_entry)
{
    emit SDeleteEntryHash(leaf_entry);

    // Recycle entry resources
    for (auto it = leaf_entry.constBegin(); it != leaf_entry.constEnd(); ++it) {
        const QSet<QUuid>& entry_set = it.value();
        for (const QUuid& entry_id : entry_set) {
            EntryPool::Instance().Recycle(entry_cache_.take(entry_id), section_);
        }
    }
}

void EntryHub::UpdateEntryRate(const QUuid& entry_id, const QJsonObject& update, InputSide /*input_side*/, int version)
{
    auto it = entry_cache_.constFind(entry_id);
    if (it != entry_cache_.constEnd()) {
        auto* d_entry = static_cast<Entry*>(it.value());

        d_entry->version = version;

        d_entry->lhs_rate = update[kLhsRate].toString().toDouble();
        d_entry->rhs_rate = update[kRhsRate].toString().toDouble();

        const int unit_cost { std::to_underlying(EntryEnum::kLhsRate) };

        emit SRefreshField(d_entry->lhs_node, entry_id, unit_cost, unit_cost);
        emit SRefreshField(d_entry->rhs_node, entry_id, unit_cost, unit_cost);
    }
}

void EntryHub::UpdateEntryNumeric(const QUuid& entry_id, const QJsonObject& update, int version)
{
    auto it = entry_cache_.constFind(entry_id);
    if (it != entry_cache_.constEnd()) {
        auto* d_entry = static_cast<Entry*>(it.value());

        d_entry->version = version;

        d_entry->lhs_debit = update[kLhsDebit].toString().toDouble();
        d_entry->lhs_credit = update[kLhsCredit].toString().toDouble();
        d_entry->rhs_debit = update[kRhsDebit].toString().toDouble();
        d_entry->rhs_credit = update[kRhsCredit].toString().toDouble();

        QUuid rhs_id { d_entry->rhs_node };
        QUuid lhs_id { d_entry->lhs_node };

        emit SUpdateBalance(rhs_id, entry_id);
        emit SUpdateBalance(lhs_id, entry_id);
    }
}

void EntryHub::Reset() { EntryPool::Instance().Recycle(entry_cache_, section_); }

void EntryHub::InsertEntry(const QJsonObject& data)
{
    auto* entry = EntryPool::Instance().Allocate(section_);
    entry->ReadJson(data);

    entry_cache_.insert(entry->id, entry);

    emit SAttachEntry(entry->lhs_node, entry);
    emit SAttachEntry(entry->rhs_node, entry);
}

void EntryHub::DeleteEntry(const QUuid& entry_id)
{
    auto it = entry_cache_.constFind(entry_id);
    if (it == entry_cache_.constEnd()) {
        return;
    }

    auto* entry = it.value();

    emit SDetachEntry(entry->lhs_node, entry_id);
    emit SDetachEntry(entry->rhs_node, entry_id);

    entry_cache_.erase(it);
    EntryPool::Instance().Recycle(entry, section_);
}

void EntryHub::UpdateEntry(const QUuid& id, const QJsonObject& update, int version)
{
    auto it = entry_cache_.constFind(id);
    if (it != entry_cache_.constEnd()) {
        auto* entry = it.value();

        entry->ReadJson(update);
        entry->version = version;
        entry->sync_state = SyncState::kSynced;

        const auto [start, end] = entry::CacheColumnRange(section_);

        emit SRefreshField(entry->lhs_node, id, start, end);
        emit SRefreshField(entry->rhs_node, id, start, end);
    };
}

void EntryHub::UpdateVersion(const QUuid& id, int version)
{
    auto it = entry_cache_.find(id);
    if (it != entry_cache_.end()) {
        auto* entry { it.value() };
        entry->sync_state = SyncState::kSynced;
        entry->version = version;
    }
}

void EntryHub::UpdateEntryLinkedNode(const QUuid& id, const QJsonObject& update, InputSide input_side)
{
    Entry* entry {};

    const auto field { input_side == InputSide::kRhs ? kRhsNode : kLhsNode };
    const QUuid new_node_id { update.value(field).toString() };

    auto it = entry_cache_.constFind(id);
    if (it != entry_cache_.constEnd()) {
        entry = it.value();

        const int version { update.value(kVersion).toInt() };
        entry->version = version;

        QUuid old_node_id {};
        QUuid lhs_node {};

        switch (input_side) {
        case InputSide::kRhs:
            old_node_id = entry->rhs_node;
            lhs_node = entry->lhs_node;
            entry->rhs_node = new_node_id;
            break;

        case InputSide::kLhs:
            old_node_id = entry->lhs_node;
            lhs_node = entry->rhs_node;
            entry->lhs_node = new_node_id;
            break;
        }

        const int linked_node_column { entry::LinkedNodeColumn(section_) };

        emit SDetachEntry(old_node_id, id);
        emit SRefreshField(lhs_node, id, linked_node_column, linked_node_column);
    } else {
        entry = EntryPool::Instance().Allocate(section_);
        entry->ReadJson(update);
        entry_cache_.insert(entry->id, entry);
    }

    emit SAttachEntry(new_node_id, entry);
}

/**
 * @brief Returns SQL to remove indirect relationships in a branch (obsolete).
 *
 * This version was used when the closure table stored all distances (full closure).
 * It decreases the distance of affected ancestor-descendant pairs by 1
 * when a node and its descendants are removed.
 *
 * Deprecated: current schema only stores direct relationships (distance = 1).
 */
// QString EntryHub::QSRemoveBranch() const
// {
//     return QString(R"(
//         WITH related_nodes AS (
//             SELECT DISTINCT fp1.ancestor, fp2.descendant
//             FROM %1 AS fp1
//             INNER JOIN %1 AS fp2 ON fp1.descendant = fp2.ancestor
//             WHERE fp2.ancestor = :node_id AND fp2.descendant != :node_id AND fp1.ancestor != :node_id
//         )
//         UPDATE %1
//         SET distance = distance - 1
//         WHERE (ancestor, descendant) IN (
//             SELECT ancestor, descendant FROM related_nodes)
//     )")
//         .arg(info_.path);
// }

// QString DbHub::QSDragNodeFirst() const
// {
//     return QString(R"(
//             WITH related_nodes AS (
//                 SELECT DISTINCT fp1.ancestor, fp2.descendant
//                 FROM %1 AS fp1
//                 INNER JOIN %1 AS fp2 ON fp1.descendant = fp2.ancestor
//                 WHERE fp2.ancestor = :node_id AND fp1.ancestor != :node_id
//             )
//             DELETE FROM %1
//             WHERE (ancestor, descendant) IN (
//             SELECT ancestor, descendant FROM related_nodes)
//             )")
//         .arg(info_.path);
// }

// QString DbHub::QSDragNodeSecond() const
// {
//     return QString(R"(
//             INSERT INTO %1 (ancestor, descendant, distance)
//             SELECT fp1.ancestor, fp2.descendant, fp1.distance + fp2.distance + 1
//             FROM %1 AS fp1
//             INNER JOIN %1 AS fp2
//             WHERE fp1.descendant = :destination_node_id AND fp2.ancestor = :node_id
//             )")
//         .arg(info_.path);
// }

// bool EntryHub::WriteRelationship(const QUuid& node_id, const QUuid& parent_id, QSqlQuery& query, qlonglong version) const
// {
//     auto part_old = QString(R"(
//     INSERT INTO %1 (ancestor, descendant, distance)
//     SELECT ancestor, :node_id, distance + 1 FROM %1
//     WHERE descendant = :parent
//     UNION ALL
//     SELECT :node_id, :node_id, 0
//     )")
//                         .arg(info_.path);
// }

// QString EntryHub::BuildSelect(const QString& table, QStringList& condition) const
// {
//     static const auto kWhere { QStringLiteral(" WHERE ") };
//     static const auto kAnd { QStringLiteral(" AND ") };

//     const QString sql {QString("SELECT * FROM %1").arg(table)};

//     if (condition.isEmpty()) {
//         return sql;
//     }

//     for (QString& cond : condition) {
//         cond = cond.simplified();
//         if (!cond.isEmpty()) {
//             cond = QStringLiteral("(") + cond + QStringLiteral(")");
//         }
//     }

//     return sql + kWhere + condition.join(kAnd);
// }

void EntryHub::AckTable(const QUuid& node_id, const QJsonArray& array)
{
    if (array.isEmpty())
        return;

    const EntryList entry_list { ProcessEntryArray(array) };
    emit SAppendEntries(node_id, entry_list);
}

void EntryHub::SearchEntry(const QJsonArray& array)
{
    const EntryList list { ProcessEntryArray(array) };
    emit SSearchEntry(list);
}

void EntryHub::PeriodClose(const QJsonArray& array)
{
    const EntryList list { ProcessEntryArray(array) };

    for (auto* entry : list) {
        emit SAttachEntry(entry->lhs_node, entry);
    }
}

EntryList EntryHub::ProcessEntryArray(const QJsonArray& array)
{
    EntryList list {};

    for (const auto& value : array) {
        Q_ASSERT(value.isObject());

        const QJsonObject obj { value.toObject() };
        const QUuid id { QUuid(obj.value(kId).toString()) };

        Entry* entry {};

        if (auto it = entry_cache_.constFind(id); it != entry_cache_.constEnd()) {
            entry = it.value();
        } else {
            entry = EntryPool::Instance().Allocate(section_);
            entry->ReadJson(obj);
            entry_cache_.insert(id, entry);
        }

        list.emplaceBack(entry);
    }

    return list;
}

void EntryHub::MarkEntries(const QUuid& node_id, MarkOperation operation)
{
    QSet<QUuid> affected_node {};

    for (auto* entry : std::as_const(entry_cache_)) {
        if (entry->lhs_node == node_id || entry->rhs_node == node_id) {
            MarkEntry(entry, operation);

            affected_node.insert(entry->lhs_node);
            affected_node.insert(entry->rhs_node);
        }
    }

    emit SRefreshStatus(affected_node);
}

void EntryHub::MarkEntry(Entry* entry, MarkOperation operation)
{
    switch (operation) {
    case MarkOperation::kSelect:
        entry->status = std::to_underlying(EntryStatus::kMarked);
        break;
    case MarkOperation::kClear:
        entry->status = std::to_underlying(EntryStatus::kUnmarked);
        break;
    case MarkOperation::kToggle:
        entry->status ^= std::to_underlying(EntryStatus::kMarked);
        break;
    default:
        break;
    }
}
