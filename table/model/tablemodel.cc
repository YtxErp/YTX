#include "tablemodel.h"

#include "component/constantwebsocket.h"
#include "global/entrypool.h"
#include "global/entryshadowpool.h"
#include "utils/entryutils.h"
#include "websocket/jsongen.h"
#include "websocket/websocket.h"

TableModel::TableModel(CTableModelArg& arg, QObject* parent)
    : QAbstractItemModel(parent)
    , info_ { arg.info }
    , direction_rule_ { arg.direction_rule }
    , node_id_ { arg.node_id }
    , section_ { arg.info.section }
{
}

TableModel::~TableModel()
{
    FlushTimers();
    EntryShadowPool::Instance().Recycle(shadow_list_, section_);
}

void TableModel::RDirectionRule(bool value)
{
    for (auto* entry_shadow : std::as_const(shadow_list_))
        entry_shadow->balance = -entry_shadow->balance;

    direction_rule_ = value;
    EmitDataChanged(0, rowCount() - 1, std::to_underlying(EntryEnum::kBalance), std::to_underlying(EntryEnum::kBalance));
}

void TableModel::RRefreshStatus()
{
    const int column { entry::StatusColumn(section_) };
    EmitDataChanged(0, rowCount() - 1, column, column);
}

void TableModel::RRefreshField(const QUuid& entry_id, int start, int end)
{
    auto idx { GetIndex(entry_id) };
    if (!idx.isValid())
        return;

    int row { idx.row() };

    EmitDataChanged(row, row, start, end);
}

void TableModel::RAttachEntry(Entry* entry)
{
    auto* entry_shadow { EntryShadowPool::Instance().Allocate(section_) };
    const auto mode { node_id_ == entry->lhs_node ? BindingMode::kParallel : BindingMode::kCross };
    entry_shadow->BindEntry(entry, mode);

    auto row { shadow_list_.size() };

    beginInsertRows(QModelIndex(), row, row);
    shadow_list_.emplaceBack(entry_shadow);
    endInsertRows();

    const double previous_balance { row >= 1 ? shadow_list_.at(row - 1)->balance : 0.0 };
    entry_shadow->balance = CalculateBalance(entry_shadow) + previous_balance;

    const int balance_column { entry::BalanceColumn(section_) };
    EmitDataChanged(row, row, balance_column, balance_column);
}

void TableModel::RDetachEntry(const QUuid& entry_id, const QUuid& /*counter_node_id*/)
{
    auto idx { GetIndex(entry_id) };
    if (!idx.isValid())
        return;

    int row { idx.row() };
    beginRemoveRows(QModelIndex(), row, row);
    EntryShadowPool::Instance().Recycle(shadow_list_.takeAt(row), section_);
    endRemoveRows();

    AccumulateBalance(row);
}

void TableModel::RUpdateBalance(const QUuid& entry_id)
{
    const auto entry_index { GetIndex(entry_id) };
    const int row { entry_index.row() };

    const auto [debit, credit] = entry::NumericColumnRange(section_);
    EmitDataChanged(row, row, debit, credit);

    if (entry_index.isValid())
        AccumulateBalance(row);
}

void TableModel::MarkEntries(MarkOperation operation)
{
    if (shadow_list_.isEmpty())
        return;

    const QJsonObject message { JsonGen::MarkEntries(section_, node_id_, std::to_underlying(operation)) };
    WebSocket::Instance()->SendMessage(WsKey::kEntriesMark, message);

    for (auto* entry_shadow : std::as_const(shadow_list_)) {
        switch (operation) {
        case MarkOperation::kSelect:
            *entry_shadow->status = std::to_underlying(EntryStatus::kMarked);
            break;
        case MarkOperation::kClear:
            *entry_shadow->status = std::to_underlying(EntryStatus::kUnmarked);
            break;
        case MarkOperation::kToggle:
            *entry_shadow->status ^= std::to_underlying(EntryStatus::kMarked);
            break;
        }
    }

    const int column { entry::StatusColumn(section_) };
    EmitDataChanged(0, rowCount() - 1, column, column);
}

void TableModel::AccumulateBalance(int start)
{
    if (start <= -1 || start >= shadow_list_.size() || shadow_list_.isEmpty())
        return;

    double previous_balance { start >= 1 ? shadow_list_.at(start - 1)->balance : 0.0 };

    std::accumulate(shadow_list_.begin() + start, shadow_list_.end(), previous_balance, [&](double current_balance, EntryShadow* entry_shadow) {
        if (!entry_shadow)
            return current_balance;

        entry_shadow->balance = CalculateBalance(entry_shadow) + current_balance;
        return entry_shadow->balance;
    });

    const int balance_column { entry::BalanceColumn(section_) };
    EmitDataChanged(start, rowCount() - 1, balance_column, balance_column);
}

void TableModel::RestartTimer(const QUuid& id)
{
    auto*& timer { pending_updates_[id].timer };

    if (!timer) {
        timer = new QTimer { this };
        timer->setSingleShot(true);

        connect(timer, &QTimer::timeout, this, [this, id]() { FlushTimer(id); });
    }

    timer->start(time_const::kAutoCloseMs);
}

void TableModel::FlushTimer(const QUuid& id)
{
    auto update { pending_updates_.take(id) };

    if (update.entry && !update.changes.isEmpty()) {
        const QJsonObject message { JsonGen::EntryUpdate(section_, id, update.changes, update.entry->version) };
        WebSocket::Instance()->SendMessage(WsKey::kEntryUpdate, message);
    }

    if (update.timer) {
        update.timer->stop();
        update.timer->deleteLater();
    }
}

void TableModel::FlushTimers()
{
    const auto ids { pending_updates_.keys() };

    for (const auto& id : ids) {
        FlushTimer(id);
    }
}

bool TableModel::UpdateNumeric(EntryShadow* shadow, double value, int row, NumericSide side)
{
    const double lhs_old_debit { *shadow->lhs_debit };
    const double lhs_old_credit { *shadow->lhs_credit };

    // Old value on the side currently being edited (debit or credit).
    // If it hasn't actually changed, there's nothing to update.
    const double old_value { side == NumericSide::kDebit ? lhs_old_debit : lhs_old_credit };
    if (qFuzzyCompare(old_value, value))
        return false;

    // "base" is the old value on the opposite side (the one not being edited).
    // Since debit and credit are mutually exclusive at any given time (only one
    // side holds a non-zero value), base represents the current balance sitting
    // on the other side, which the new value will be netted against.
    const double base { side == NumericSide::kDebit ? lhs_old_credit : lhs_old_debit };

    // Remaining amount after offsetting the opposite side.
    // The sign determines whether the balance belongs to debit or credit;
    // the magnitude is stored in the corresponding side.
    const double diff { std::abs(value - base) };
    const bool debit_side { (side == NumericSide::kDebit && value > base) || (side == NumericSide::kCredit && value <= base) };

    *shadow->lhs_debit = debit_side ? diff : 0.0;
    *shadow->lhs_credit = debit_side ? 0.0 : diff;

    // Mirror to RHS (Debit ↔ Credit)
    *shadow->rhs_debit = *shadow->lhs_credit;
    *shadow->rhs_credit = *shadow->lhs_debit;

    if (shadow->rhs_node->isNull())
        return false;

    const QUuid entry_id { *shadow->id };
    const QUuid rhs_id { *shadow->rhs_node };

    const auto input_side { ToValueInputSide(shadow->binding_mode) };

    QJsonObject update {};
    update.insert(input_side == InputSide::kLhs ? kLhsDebit : kRhsDebit, QString::number(*shadow->lhs_debit, 'f', numeric_const::kDecimalPlaces8));
    update.insert(input_side == InputSide::kLhs ? kLhsCredit : kRhsCredit, QString::number(*shadow->lhs_credit, 'f', numeric_const::kDecimalPlaces8));
    update.insert(input_side == InputSide::kLhs ? kRhsDebit : kLhsDebit, QString::number(*shadow->rhs_debit, 'f', numeric_const::kDecimalPlaces8));
    update.insert(input_side == InputSide::kLhs ? kRhsCredit : kLhsCredit, QString::number(*shadow->rhs_credit, 'f', numeric_const::kDecimalPlaces8));

    QJsonObject message { JsonGen::EntryValue(section_, entry_id, update, input_side, *shadow->version) };
    WebSocket::Instance()->SendMessage(WsKey::kEntryNumericUpdate, message);

    // Delta calculation follows the DICD rule (Debit - Credit).
    // After the delta is computed, both the node and the server
    // will adjust the delta value according to the node's direction rule
    // (DICD → unchanged, DDCI → inverted).
    // The right-hand side (RHS) node must always mirror the left-hand side (LHS),
    // therefore its delta is the opposite of LHS delta.
    // This ensures overall balance (LHS + RHS = 0).
    const double lhs_initial_delta { *shadow->lhs_debit - *shadow->lhs_credit - (lhs_old_debit - lhs_old_credit) };

    if (!qFuzzyIsNull(lhs_initial_delta)) {
        AccumulateBalance(row);
        const int balance_column { entry::BalanceColumn(section_) };
        EmitDataChanged(row, row, balance_column, balance_column);
        emit SUpdateBalance(rhs_id, *shadow->id);
    }

    return true;
}

bool TableModel::UpdateRate(EntryShadow* shadow, double value)
{
    if (value < 0)
        return false;

    const double old_value { *shadow->lhs_rate };
    if (qFuzzyCompare(old_value, value))
        return false;

    *shadow->lhs_rate = value;
    *shadow->rhs_rate = value;

    if (shadow->rhs_node->isNull())
        return false;

    QJsonObject update {};
    update.insert(kVersion, *shadow->version);
    update.insert(kLhsRate, QString::number(value, 'f', numeric_const::kDecimalPlaces8));
    update.insert(kRhsRate, QString::number(value, 'f', numeric_const::kDecimalPlaces8));

    QJsonObject message { JsonGen::EntryValue(section_, *shadow->id, update, ToValueInputSide(shadow->binding_mode), *shadow->version) };
    WebSocket::Instance()->SendMessage(WsKey::kEntryRateUpdate, message);

    return true;
}

bool TableModel::UpdateLinkedNode(EntryShadow* shadow, const QUuid& value, int row)
{
    if (value.isNull())
        return false;

    const QUuid old_node { *shadow->rhs_node };
    if (old_node == value)
        return false;

    *shadow->rhs_node = value;

    const QUuid entry_id { *shadow->id };
    auto* entry { shadow->entry };

    QJsonObject message { JsonGen::EntryMessage(section_, entry_id) };

    if (*shadow->sync_state == SyncState::kCreating) {
        message.insert(kEntry, shadow->WriteJson());
        WebSocket::Instance()->SendMessage(WsKey::kEntryInsert, message);

        const double lhs_debit { *shadow->lhs_debit };
        const double lhs_credit { *shadow->lhs_credit };

        if (!qFuzzyIsNull(lhs_debit - lhs_credit)) {
            AccumulateBalance(row);
            const int balance_column { entry::BalanceColumn(section_) };
            EmitDataChanged(row, row, balance_column, balance_column);
        }

        emit STransferEntry(entry);
    }

    if (*shadow->sync_state == SyncState::kSynced) {
        const auto input_side { ToLinkedNodeInputSide(shadow->binding_mode) };
        const auto field { input_side == InputSide::kLhs ? kLhsNode : kRhsNode };

        QJsonObject update {};
        update.insert(field, value.toString(QUuid::WithoutBraces));

        message.insert(kVersion, *shadow->version);
        message.insert(kUpdate, update);
        message.insert(kInputSide, std::to_underlying(input_side));

        WebSocket::Instance()->SendMessage(WsKey::kEntryLinkedNodeUpdate, message);

        emit SDetachEntry(old_node, entry_id);
    }

    emit SAttachEntry(value, entry);
    return true;
}

QModelIndex TableModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column, shadow_list_.at(row));
}

bool TableModel::insertRows(int row, int /*count*/, const QModelIndex& parent)
{
    auto* entry { EntryPool::Instance().Allocate(section_) };
    entry->id = QUuid::createUuidV7();

    auto* entry_shadow { EntryShadowPool::Instance().Allocate(section_) };
    entry_shadow->BindEntry(entry, BindingMode::kParallel);

    if (section_ == Section::kFinance) {
        *entry_shadow->lhs_rate = 1.0;
        *entry_shadow->rhs_rate = 1.0;
    }

    {
        *entry_shadow->lhs_node = node_id_;
        entry_shadow->balance = row >= 1 ? shadow_list_.at(row - 1)->balance : 0.0;
    }

    beginInsertRows(parent, row, row);
    shadow_list_.insert(row, entry_shadow);
    endInsertRows();

    return true;
}

QVariant TableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return info_.entry_header.at(section);

    return QVariant();
}

QModelIndex TableModel::GetIndex(const QUuid& entry_id) const
{
    int row { 0 };

    for (const auto* entry_shadow : shadow_list_) {
        if (*entry_shadow->id == entry_id) {
            return index(row, 0);
        }
        ++row;
    }
    return QModelIndex();
}

QVariant TableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    auto* shadow { static_cast<EntryShadow*>(index.internalPointer()) };

    const EntryEnum column { index.column() };

    switch (column) {
    case EntryEnum::kIssuedTime:
        return *shadow->issued_time;
    case EntryEnum::kLhsNode:
        return *shadow->lhs_node;
    case EntryEnum::kCode:
        return *shadow->code;
    case EntryEnum::kLhsRate:
        return *shadow->lhs_rate;
    case EntryEnum::kDescription:
        return *shadow->description;
    case EntryEnum::kRhsNode:
        return *shadow->rhs_node;
    case EntryEnum::kStatus:
        return *shadow->status;
    case EntryEnum::kDocument:
        return *shadow->document;
    case EntryEnum::kTag:
        return *shadow->tag;
    case EntryEnum::kDebit:
        return *shadow->lhs_debit;
    case EntryEnum::kCredit:
        return *shadow->lhs_credit;
    case EntryEnum::kBalance:
        return shadow->balance;
    }
}

bool TableModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    if (data(index, role) == value)
        return false;

    auto* shadow { static_cast<EntryShadow*>(index.internalPointer()) };

    const QUuid id { *shadow->id };
    auto& update { pending_updates_[id] };
    update.entry = shadow->entry;
    auto& changes { update.changes };

    const EntryEnum column { index.column() };
    const int row { index.row() };

    switch (column) {
    case EntryEnum::kIssuedTime:
        entry::UpdateShadowIssuedTime(changes, shadow, kIssuedTime, value.toDateTime(), &EntryShadow::issued_time, [this, id]() { RestartTimer(id); });
        break;
    case EntryEnum::kCode:
        entry::UpdateShadowField(changes, shadow, kCode, value.toString(), &EntryShadow::code, [this, id]() { RestartTimer(id); });
        break;
    case EntryEnum::kStatus:
        entry::UpdateShadowField(changes, shadow, kStatus, value.toInt(), &EntryShadow::status, [this, id]() { RestartTimer(id); });
        break;
    case EntryEnum::kDescription:
        entry::UpdateShadowField(changes, shadow, kDescription, value.toString(), &EntryShadow::description, [this, id]() { RestartTimer(id); });
        break;
    case EntryEnum::kDocument:
        entry::UpdateShadowStringList(changes, shadow, kDocument, value.toStringList(), &EntryShadow::document, [this, id]() { RestartTimer(id); });
        break;
    case EntryEnum::kTag:
        entry::UpdateShadowStringList(changes, shadow, kTag, value.toStringList(), &EntryShadow::tag, [this, id]() { RestartTimer(id); });
        break;
    case EntryEnum::kLhsRate:
        UpdateRate(shadow, value.toDouble());
        break;
    case EntryEnum::kRhsNode:
        UpdateLinkedNode(shadow, value.toUuid(), row);
        break;
    case EntryEnum::kDebit:
        UpdateNumeric(shadow, value.toDouble(), row, NumericSide::kDebit);
        break;
    case EntryEnum::kCredit:
        UpdateNumeric(shadow, value.toDouble(), row, NumericSide::kCredit);
        break;
    case EntryEnum::kLhsNode:
    case EntryEnum::kBalance:
        return false;
    }

    emit dataChanged(index, index, { Qt::DisplayRole, Qt::EditRole });
    return true;
}

void TableModel::sort(int column, Qt::SortOrder order)
{
    const EntryEnum e_column { column };
    if (e_column == EntryEnum::kBalance)
        return;

    auto Compare = [order, e_column](const EntryShadow* lhs, const EntryShadow* rhs) -> bool {
        switch (e_column) {
        case EntryEnum::kCode:
            return utils::CompareShadowMember(lhs, rhs, &EntryShadow::code, order);
        case EntryEnum::kDescription:
            return utils::CompareShadowMember(lhs, rhs, &EntryShadow::description, order);
        case EntryEnum::kIssuedTime:
            return utils::CompareShadowMember(lhs, rhs, &EntryShadow::issued_time, order);
        case EntryEnum::kLhsRate:
            return utils::CompareShadowMember(lhs, rhs, &EntryShadow::lhs_rate, order);
        case EntryEnum::kRhsNode:
            return utils::CompareShadowMember(lhs, rhs, &EntryShadow::rhs_node, order);
        case EntryEnum::kStatus:
            return utils::CompareShadowMember(lhs, rhs, &EntryShadow::status, order);
        case EntryEnum::kDocument:
            return (order == Qt::AscendingOrder) ? (lhs->document->size() < rhs->document->size()) : (lhs->document->size() > rhs->document->size());
        case EntryEnum::kTag:
            return utils::CompareShadowMember(lhs, rhs, &EntryShadow::tag, order);
        case EntryEnum::kDebit:
            return utils::CompareShadowMember(lhs, rhs, &EntryShadow::lhs_debit, order);
        case EntryEnum::kCredit:
            return utils::CompareShadowMember(lhs, rhs, &EntryShadow::lhs_credit, order);
        case EntryEnum::kLhsNode:
        case EntryEnum::kBalance:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::ranges::sort(shadow_list_, Compare);
    emit layoutChanged();

    AccumulateBalance(0);
}

Qt::ItemFlags TableModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };

    auto* shadow { static_cast<EntryShadow*>(index.internalPointer()) };
    if (*shadow->sync_state == SyncState::kDeleting)
        return flags & ~Qt::ItemIsEditable;

    const EntryEnum column { index.column() };

    switch (column) {
    case EntryEnum::kBalance:
    case EntryEnum::kDocument:
    case EntryEnum::kTag:
    case EntryEnum::kStatus:
    case EntryEnum::kLhsNode:
        flags &= ~Qt::ItemIsEditable;
        break;
    case EntryEnum::kIssuedTime:
    case EntryEnum::kLhsRate:
    case EntryEnum::kRhsNode:
    case EntryEnum::kCode:
    case EntryEnum::kDescription:
    case EntryEnum::kDebit:
    case EntryEnum::kCredit:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}

bool TableModel::removeRows(int row, int /*count*/, const QModelIndex& parent)
{
    Q_ASSERT(row >= 0 && row <= rowCount(parent) - 1);

    // Capture the shadow and IDs using {} initialization
    auto* shadow { shadow_list_.at(row) };
    if (*shadow->sync_state == SyncState::kDeleting)
        return true;

    const auto entry_id { *shadow->id };
    const int version { *shadow->version };

    CancelPendingUpdate(entry_id);

    if (*shadow->sync_state == SyncState::kCreating) {
        beginRemoveRows(parent, row, row);
        shadow_list_.removeAt(row);
        endRemoveRows();

        // Recycle the entry before recycling the shadow.
        // The shadow holds a pointer to the entry, so the entry must be released first.
        EntryPool::Instance().Recycle(shadow->entry, section_);
        EntryShadowPool::Instance().Recycle(shadow, section_);

        return true;
    }

    *shadow->sync_state = SyncState::kDeleting;

    QJsonObject message { JsonGen::EntryMessage(section_, entry_id) };
    message.insert(kVersion, version);
    WebSocket::Instance()->SendMessage(WsKey::kEntryDelete, message);

    return true;
}

void TableModel::EmitDataChanged(int start_row, int end_row, int start_column, int end_column, const QModelIndex& parent)
{
    // top_left and bottom_right must share the same parent, behavior is undefined otherwise
    Q_ASSERT(!parent.isValid() || parent.model() == this);

    if (start_row < 0 || end_row >= rowCount(parent) || start_row > end_row) {
        qDebug() << "EmitDataChanged: invalid row range" << start_row << end_row << "rowCount" << rowCount(parent);
        return;
    }

    if (start_column < 0 || end_column >= columnCount(parent) || start_column > end_column) {
        qDebug() << "EmitDataChanged: invalid column range" << start_column << end_column << "columnCount" << columnCount(parent);
        return;
    }

    const QModelIndex top_left { index(start_row, start_column, parent) };
    const QModelIndex bottom_right { index(end_row, end_column, parent) };

    Q_ASSERT(top_left.parent() == bottom_right.parent());

    emit dataChanged(top_left, bottom_right, QList<int> { Qt::DisplayRole, Qt::EditRole });
}

void TableModel::CancelPendingUpdate(const QUuid& entry_id)
{
    // Remove pending update to prevent delayed flush after deletion
    // Stop its timer to avoid accessing recycled member.
    if (auto* timer = pending_updates_.take(entry_id).timer; timer) {
        timer->stop();
        timer->deleteLater();
    }
}

void TableModel::RDeleteEntries(const QSet<QUuid>& entry_id_set)
{
    int min_row { std::numeric_limits<int>::max() };

    for (int i = shadow_list_.size() - 1; i >= 0; --i) {
        const auto entry_id { *shadow_list_[i]->id };

        if (entry_id_set.contains(entry_id)) {
            min_row = i;

            beginRemoveRows(QModelIndex(), i, i);
            EntryShadowPool::Instance().Recycle(shadow_list_.takeAt(i), section_);
            endRemoveRows();
        }
    }

    if (min_row != std::numeric_limits<int>::max())
        AccumulateBalance(min_row);
}

void TableModel::RAppendEntries(const EntryList& entry_list)
{
    EntryShadowList shadow_list {};
    shadow_list.reserve(entry_list.size());

    for (auto* entry : entry_list) {
        auto* entry_shadow { EntryShadowPool::Instance().Allocate(section_) };
        const auto mode { node_id_ == entry->lhs_node ? BindingMode::kParallel : BindingMode::kCross };
        entry_shadow->BindEntry(entry, mode);
        shadow_list.emplaceBack(entry_shadow);
    }

    auto row { shadow_list_.size() };
    beginInsertRows(QModelIndex(), row, row + entry_list.size() - 1);
    shadow_list_.append(shadow_list);
    endInsertRows();

    sort(std::to_underlying(EntryEnum::kIssuedTime), Qt::AscendingOrder);
    AccumulateBalance(row);
}
