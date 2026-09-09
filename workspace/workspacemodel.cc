#include "workspacemodel.h"

#include <QJsonArray>

#include "component/constantwebsocket.h"
#include "global/resourcepool.h"
#include "websocket/jsongen.h"
#include "websocket/websocket.h"
#include "workspaceenum.h"

namespace workspace {
Model::Model(const QStringList& header, QObject* parent)
    : QAbstractItemModel(parent)
    , header_ { header }
{
}

Model::~Model() { FlushTimers(); }

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

    const MemberField column { index.column() };
    auto* member { static_cast<Member*>(index.internalPointer()) };

    switch (column) {
    case MemberField::kEmail:
        return member->email;
    case MemberField::kUsername:
        return member->username;
    case MemberField::kName:
        return member->name;
    case MemberField::kWorkspaceRole:
        return static_cast<int>(member->workspace_role);
    case MemberField::kSectionPermissions:
        return static_cast<int>(member->section_permissions);
    case MemberField::kCreatedTime:
        return member->created_time;
    }
}

bool Model::setData(const QModelIndex& index, const QVariant& value, int role)
{
    // Basic validation for index and role
    if (!index.isValid() || role != Qt::EditRole) {
        return false;
    }

    // Return if value is the same as the current data
    if (data(index, role) == value) {
        return false;
    }

    // Get the member pointer using {} initialization
    auto* member { static_cast<Member*>(index.internalPointer()) };
    const QUuid id { member->id };

    if (id.isNull()) {
        return false;
    }

    auto& update { pending_updates_[id] };
    update.member = member;
    auto& changes { update.changes };

    // Handle updates based on the column index
    // Assuming MemberColumn is your enum for WorkspaceMember columns
    switch (static_cast<MemberField>(index.column())) {
    case MemberField::kWorkspaceRole: {
        const int raw { value.toInt() };
        member->workspace_role = static_cast<workspace::Role>(raw);
        changes.insert(kWorkspaceRole, raw);
        break;
    }
    case MemberField::kSectionPermissions: {
        const int raw { value.toInt() };
        member->section_permissions = static_cast<section::Permissions>(raw);
        changes.insert(kSectionPermissions, raw);
        break;
    }
    case MemberField::kEmail:
    case MemberField::kUsername:
    case MemberField::kName:
    case MemberField::kCreatedTime:
        // These columns are read-only in this model
        return false;
    }

    // Mark as pending update and restart the debounce timer
    RestartTimer(id);

    // Notify views about the data change
    emit dataChanged(index, index, { Qt::DisplayRole, Qt::EditRole });
    return true;
}

void Model::sort(int column, Qt::SortOrder order)
{
    // Convert integer column to the structured enum using brace initialization
    const MemberField e_column { column };

    // Define a lambda for comparison based on the selected column and sort order
    auto Compare = [order, e_column](const Member* lhs, const Member* rhs) -> bool {
        switch (e_column) {
        case MemberField::kEmail:
            return utils::CompareMember(lhs, rhs, &Member::email, order);
        case MemberField::kUsername:
            return utils::CompareMember(lhs, rhs, &Member::username, order);
        case MemberField::kName:
            return utils::CompareMember(lhs, rhs, &Member::name, order);
        case MemberField::kWorkspaceRole:
            // Sorting by the underlying integer value of the enum
            return utils::CompareMember(lhs, rhs, &Member::workspace_role, order);
        case MemberField::kSectionPermissions:
            return utils::CompareMember(lhs, rhs, &Member::section_permissions, order);
        case MemberField::kCreatedTime:
            return utils::CompareMember(lhs, rhs, &Member::created_time, order);
        }
    };

    // Notify the view that the layout is about to change
    emit layoutAboutToBeChanged();

    // Perform the sort on the underlying data list
    std::ranges::sort(list_, Compare);

    // Notify the view that the layout has been updated
    emit layoutChanged();
}

Qt::ItemFlags Model::flags(const QModelIndex& index) const
{
    // Return early if index is invalid
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    // Initialize flags with the base class flags using brace initialization
    Qt::ItemFlags flags { QAbstractItemModel::flags(index) };

    // Convert column to enum for safe processing
    const MemberField column { index.column() };

    switch (column) {
    case MemberField::kWorkspaceRole:
    case MemberField::kSectionPermissions:
        // Enable editing for specific roles
        flags |= Qt::ItemIsEditable;
        break;

    case MemberField::kEmail:
    case MemberField::kUsername:
    case MemberField::kName:
    case MemberField::kCreatedTime:
        // Disable editing for all other columns
        flags &= ~Qt::ItemIsEditable;
        break;
    }

    return flags;
}

bool Model::removeRows(int row, int count, const QModelIndex& parent)
{
    // Basic validation
    if (count != 1 || row < 0 || row >= list_.size()) {
        return false;
    }

    // Capture the pointer before removal
    auto* member { list_.at(row) };
    const QUuid member_id { member->id };

    // Remove pending update to prevent delayed flush after deletion
    // Stop its timer to avoid accessing recycled member.
    if (auto* timer = pending_updates_.take(member_id).timer; timer) {
        timer->stop();
        timer->deleteLater();
    }

    // Notify views that rows are about to be removed
    beginRemoveRows(parent, row, row);
    list_.removeAt(row);
    endRemoveRows();

    // Send the WebSocket message using the captured ID
    const QJsonObject message { JsonGen::WorkspaceMemberDelete(member_id) };
    WebSocket::Instance()->SendMessage(WsKey::kWorkspaceMemberDelete, message);

    // Recycle the member back to the ResourcePool
    ResourcePool<Member>::Instance().Recycle(member);

    return true;
}

void Model::Rebuild(const QJsonArray& array)
{
    if (array.isEmpty()) {
        qDebug() << Q_FUNC_INFO << "Received empty member array";
    }

    // Parse outside the reset block
    QList<Member*> new_list {};
    new_list.reserve(array.size());

    for (const auto& value : array) {
        Q_ASSERT(value.isObject());

        auto* member { ResourcePool<Member>::Instance().Allocate() };
        member->ReadJson(value.toObject());
        new_list.emplaceBack(member);
    }

    std::ranges::sort(new_list, [](const auto* lhs, const auto* rhs) { return utils::CompareMember(lhs, rhs, &Member::workspace_role, Qt::DescendingOrder); });

    // Keep reset block as short as possible
    beginResetModel();

    ResourcePool<Member>::Instance().Recycle(list_);
    list_ = std::move(new_list);

    endResetModel();
}

void Model::RestartTimer(const QUuid& id)
{
    auto*& timer { pending_updates_[id].timer };

    if (!timer) {
        timer = new QTimer { this };
        timer->setSingleShot(true);

        connect(timer, &QTimer::timeout, this, [this, id]() { FlushTimer(id); });
    }

    timer->start(time_const::kAutoCloseMs);
}

void Model::FlushTimer(const QUuid& id)
{
    auto update { pending_updates_.take(id) };

    if (update.member && !update.changes.isEmpty()) {
        const int version { update.member->version };

        const QJsonObject message { JsonGen::WorkspaceMemberUpdate(id, update.changes, version) };
        WebSocket::Instance()->SendMessage(WsKey::kWorkspaceMemberUpdate, message);

        update.member->version = version + 1;
    }

    if (update.timer) {
        update.timer->stop();
        update.timer->deleteLater();
    }
}

void Model::FlushTimers()
{
    const auto ids { pending_updates_.keys() };

    for (const auto& id : ids) {
        FlushTimer(id);
    }
}

}
