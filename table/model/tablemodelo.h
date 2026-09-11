/*
 * Copyright (C) 2023 YTX
 *
 * This file is part of YTX.
 *
 * YTX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YTX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with YTX. If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * TableModelO design notes:
 *
 * 1. Every entry carries a `sync_state` (kCreating / kSynced / kUpdating) that
 *    tracks its relationship to the server, replacing the old approach of
 *    tracking pending inserts/updates in separate containers.
 *    - `kCreating`: a brand-new entry added to this draft order, never sent
 *      to the server.
 *    - `kSynced`: an entry that matches what the server has.
 *    - `kUpdating`: a previously synced entry that has been edited in this
 *      session but not yet resubmitted.
 *
 * 2. No network requests are sent as edits happen. All changes accumulate
 *    locally on `entry_list_` until the user explicitly submits the order
 *    (e.g. clicking "Save"/"Release"), at which point `Finalize()` collects
 *    everything in a single batch:
 *      a) PurifyEntry() removes any kCreating entries with a null rhs_node
 *         (internal SKU never selected) — these are incomplete drafts and
 *         must never reach the server.
 *      b) Remaining entries are classified by sync_state: kCreating entries
 *         go into the insert array, kUpdating entries go into the update
 *         array, each serialized to JSON.
 *      c) After being packaged, every entry's sync_state is reset to
 *         kSynced — this submission attempt is considered closed regardless
 *         of the eventual server outcome (see note 5).
 *
 * 3. Deletions:
 *    - If the entry being removed is still kCreating (never synced), it is
 *      simply dropped from entry_list_ and recycled; the server never knew
 *      it existed, so nothing further is needed.
 *    - If the entry is kSynced or kUpdating (i.e. the server already has a
 *      record of it), its id is recorded in `pending_delete_` so Finalize()
 *      can include it in the delete array sent to the server.
 *
 * 4. There is no per-entry confirmation step after submission. This section
 *    relies on the order node's version for optimistic concurrency control:
 *    if a submission is rejected (e.g. due to a stale version), the correct
 *    recovery is to discard this editing session and reopen the order with
 *    fresh data — not to retry or roll back individual entries. This is why
 *    sync_state is reset to kSynced immediately after Finalize(), rather
 *    than waiting for a server acknowledgment.
 *
 * 5. kDeleting and kError (from the shared SyncState enum) are not used in
 *    this model; they only apply to sections with per-entry, real-time
 *    server synchronization (see TableModel/TableModelP).
 *
 * This design keeps all edits local and batched until an explicit submit,
 * avoids sending incomplete or redundant data to the server, and offloads
 * conflict handling to the order's version check rather than fine-grained
 * per-entry retry logic.
 */

#pragma once

#include "tablemodel.h"
#include "tree/node.h"

class TableModelO final : public TableModel {
    Q_OBJECT

public:
    explicit TableModelO(CTableModelArg& arg, QObject* parent = nullptr);
    ~TableModelO() override;

signals:
    // send to TableWidgetO
    void SSyncDeltaO(const QUuid& node_id, double initial_delta, double count_delta, double measure_delta);

public slots:
    void RAppendEntries(const EntryList& entry_list) override;

public:
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& /*parent*/ = QModelIndex()) const override { return entry_list_.size(); }
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;

    bool insertRows(int row, int /*count*/, const QModelIndex& parent) override;
    bool removeRows(int row, int, const QModelIndex& parent = QModelIndex()) override;

    Entry* GetEntry(const QModelIndex& index) const override { return entry_list_.at(index.row()); }
    QModelIndex GetIndex(const QUuid& entry_id) const override;

    const QList<Entry*>& GetEntryList() const { return entry_list_; }
    void Purify();
    void Finalize(QJsonObject& message);
    bool HasZeroUnitPrice() const;
    bool HasPendingUpdate() const;
    void SetNode(const NodeO* node) { d_node_ = node; }

private:
    bool UpdateInternalSku(Entry* entry, const QUuid& value, int row) override;
    bool UpdateUnitPrice(EntryO* entry, double value);
    bool UpdateMeasure(EntryO* entry, double value);
    bool UpdateCount(EntryO* entry, double value);
    bool UpdateDescription(EntryO* entry, const QString& value);
    bool UpdateTag(EntryO* entry, const QStringList& value);

private:
    const NodeO* d_node_ {};

    QList<Entry*> entry_list_ {};

    QSet<QUuid> pending_delete_ {};
};
