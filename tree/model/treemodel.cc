#include "treemodel.h"

#include <QJsonArray>
#include <QQueue>

#include "component/constantwebsocket.h"
#include "global/nodepool.h"
#include "tree/excludeidfiltermodel.h"
#include "tree/includeunitfiltermodel.h"
#include "tree/replaceselffiltermodel.h"
#include "utils/nodeutils.h"
#include "utils/pathutils.h"
#include "websocket/jsongen.h"
#include "websocket/websocket.h"

TreeModel::TreeModel(CSectionInfo& info, CString& separator, QObject* parent)
    : QAbstractItemModel(parent)
    , section_ { info.section }
    , separator_ { separator }
    , header_ { info.node_header }
{
    InitRoot();
}

TreeModel::~TreeModel()
{
    FlushTimers();
    NodePool::Instance().Recycle(root_, section_);
    NodePool::Instance().Recycle(node_hash_, section_);
}

void TreeModel::ApplyDelete(const QUuid& node_id)
{
    auto* node { node_hash_.value(node_id, nullptr) };
    if (!node)
        return;

    const auto index { GetIndex(node_id) };
    auto* parent_node { node->parent };
    const int row { index.row() };

    // Remove pending update to prevent delayed flush after deletion
    // Stop its timer to avoid accessing recycled member.
    if (auto* timer = pending_updates_.take(node_id).timer; timer) {
        timer->stop();
        timer->deleteLater();
    }

    beginRemoveRows(index.parent(), row, row);
    parent_node->children.removeAt(row);
    endRemoveRows();

    UnregisterNode(node, parent_node);

    NodePool::Instance().Recycle(node, section_);
    node_hash_.remove(node_id);
}

void TreeModel::SyncTotalArray(const QJsonArray& total_array)
{
    if (total_array.isEmpty())
        return;

    QSet<QUuid> affected_ids {};

    for (const auto& total : total_array) {
        const QJsonObject obj { total.toObject() };

        Q_ASSERT_X(obj.contains(kId), "TreeModel::SyncTotalArray", "Missing kId in total object");
        Q_ASSERT_X(obj.contains(kInitialTotal), "TreeModel::SyncTotalArray", "Missing kInitialTotal in total object");
        Q_ASSERT_X(obj.contains(kFinalTotal), "TreeModel::SyncTotalArray", "Missing kFinalTotal in total object");

        const QUuid node_id { QUuid(obj.value(kId).toString()) };
        const double initial_total { obj.value(kInitialTotal).toString().toDouble() };
        const double final_total { obj.value(kFinalTotal).toString().toDouble() };

        const auto ids { SyncTotal(node_id, initial_total, final_total) };
        affected_ids.unite(ids);
    }

    EmitNumericChanged(affected_ids);
}

void TreeModel::ApplyNode(const QUuid& ancestor, const QJsonObject& data)
{
    Node* parent { node_hash_.value(ancestor) };
    if (!parent)
        parent = root_;

    auto* node { NodePool::Instance().Allocate(section_) };
    node->ReadJson(data);

    const auto row { parent->children.size() };

    auto parent_index { GetIndex(parent->id) };

    beginInsertRows(parent_index, row, row);
    parent->children.insert(row, node);
    node->parent = parent;
    endInsertRows();

    node_hash_.insert(node->id, node);
    RegisterNode(node);
}

QSet<QUuid> TreeModel::SyncTotal(const QUuid& node_id, double initial_total, double final_total)
{
    auto* node = GetNode(node_id);
    if (!node)
        return {};

    const node::Delta delta {
        .initial = initial_total - node->initial_total,
        .final = final_total - node->final_total,
    };

    qDebug() << Q_FUNC_INFO << "whether delta is null:" << delta.IsNull();

    // Accumulate into the current node totals
    node->initial_total = initial_total;
    node->final_total = final_total;
    node->version += 1;

    // Propagate adjusted deltas to ancestor nodes
    auto ids { UpdateAncestorTotal(node, delta) };
    ids.insert(node_id);

    emit SSyncValue();

    return ids;
}

QSet<QUuid> TreeModel::ExtractLeafIds(const Node* node) const
{
    if (leaf_path_.isEmpty())
        return {};

    QQueue<const Node*> queue {};
    queue.enqueue(node);

    QSet<QUuid> leaf_ids {};

    while (!queue.isEmpty()) {
        const auto* current { queue.dequeue() };
        const NodeKind kind { current->kind };

        switch (kind) {
        case NodeKind::kBranch:
            for (const auto* child : current->children)
                queue.enqueue(child);

            break;
        case NodeKind::kLeaf:
            leaf_ids.insert(current->id);
            break;
        default:
            break;
        }
    }

    return leaf_ids;
}

void TreeModel::SyncLeafModel(const QSet<QUuid>& leaf_ids) const
{
    if (leaf_ids.isEmpty() || leaf_path_.isEmpty())
        return;

    for (const QUuid& id : leaf_ids) {
        const QString value { leaf_path_.value(id) };

        if (value.isEmpty()) {
            qWarning() << Q_FUNC_INFO << "Missing leaf path";
            continue;
        }

        leaf_model_->SetDisplay(id, value);
    }

    leaf_model_->sort(0);
}

void TreeModel::UpdateSubtreePath(const Node* node)
{
    QQueue<const Node*> queue {};
    queue.enqueue(node);

    while (!queue.isEmpty()) {
        const auto* current { queue.dequeue() };
        const auto path { path::Build(current, root_, separator_) };
        const NodeKind kind { current->kind };

        switch (kind) {
        case NodeKind::kBranch:
            for (const auto* child : current->children)
                queue.enqueue(child);

            branch_path_.insert(current->id, path);
            break;
        case NodeKind::kLeaf:
            leaf_path_.insert(current->id, path);
            break;
        }
    }
}

void TreeModel::InitLeafData()
{
    leaf_model_->Rebuild(leaf_path_);
    InitUnitSet();

    for (auto it = leaf_path_.cbegin(); it != leaf_path_.cend(); ++it) {
        const auto node_id { it.key() };

        auto* node { node_hash_.value(node_id, nullptr) };
        Q_ASSERT(node);

        UnitSetInsert(node_id, node->unit);
    }
}

void TreeModel::ApplyUpdate(const QUuid& node_id, const QJsonObject& update, int version)
{
    if (update.isEmpty()) {
        qDebug() << Q_FUNC_INFO << "Update is empty";
        return;
    }

    const auto index { GetIndex(node_id) };
    if (!index.isValid())
        return;

    auto* node { static_cast<Node*>(index.internalPointer()) };
    if (!node)
        return;

    node->ReadJson(update);
    node->version = version;

    const int row { index.row() };
    const auto [start, end] = node::CacheColumnRange(section_);
    EmitDataChanged(row, row, start, end, index.parent());
}

void TreeModel::RequestDirectionRule(Node* node, bool value)
{
    if (node->direction_rule == value)
        return;

    QJsonObject message { JsonGen::NodeDirectionRule(section_, node->id, value, node->version) };
    WebSocket::Instance()->SendMessage(WsKey::kNodeDirectionRuleUpdate, message);
}

void TreeModel::RequestStatus(Node* node, int value)
{
    if (node->kind == NodeKind::kBranch || node->status == NodeStatus(value))
        return;

    QJsonObject message { JsonGen::NodeStatus(section_, node->id, value, node->version) };
    WebSocket::Instance()->SendMessage(WsKey::kNodeStatus, message);
}

void TreeModel::ApplyDirectionRule(const QUuid& node_id, bool direction_rule, int version)
{
    const auto index { GetIndex(node_id) };
    if (!index.isValid())
        return;

    auto* node { static_cast<Node*>(index.internalPointer()) };
    if (!node)
        return;

    node->InvertTotal();
    node->direction_rule = direction_rule;
    node->version = version;

    const int row { index.row() };

    {
        const auto [start_col, end_col] = node::NumericColumnRange(section_);
        EmitDataChanged(row, row, start_col, end_col, index.parent());
    }

    {
        const int column { node::DirectionRuleColumn(section_) };
        EmitDataChanged(row, row, column, column, index.parent());
    }

    if (node->kind == NodeKind::kLeaf) {
        emit SDirectionRule(node_id, node->direction_rule);
    }

    emit SSyncValue();
}

void TreeModel::ApplyStatus(const QUuid& node_id, int status, int version)
{
    const auto index { GetIndex(node_id) };
    if (!index.isValid())
        return;

    auto* node { static_cast<Node*>(index.internalPointer()) };
    if (!node)
        return;

    const auto node_status { static_cast<NodeStatus>(status) };

    node->status = node_status;
    node->version = version;

    // Refresh the view first, as the following status check may return early.
    {
        const int row { index.row() };
        const int column { node::StatusColumn(section_) };
        EmitDataChanged(row, row, column, column, index.parent());
    }

    if (node_status != NodeStatus::kActive) {
        leaf_path_.remove(node_id);
        leaf_model_->RemoveItem(node_id);
        return;
    }

    const QString path { path::Build(node, root_, separator_) };

    leaf_path_.insert(node_id, path);
    leaf_model_->AppendItem(path, node_id);
}

void TreeModel::SyncVersion(const QUuid& node_id, int version)
{
    auto* node { GetNode(node_id) };
    if (!node)
        return;

    node->version = version;
}

void TreeModel::ApplyReplace(const QUuid& old_node_id, const QUuid& new_node_id)
{
    auto* old_node { GetNode(old_node_id) };
    auto* new_node { GetNode(new_node_id) };

    if (!old_node || !new_node)
        return;

    const int multiplier { old_node->direction_rule == new_node->direction_rule ? 1 : -1 };

    const node::Delta delta {
        .initial = multiplier * old_node->initial_total,
        .final = multiplier * old_node->final_total,
    };

    new_node->initial_total += delta.initial;
    new_node->final_total += delta.final;

    auto ids { UpdateAncestorTotal(new_node, delta) };
    ids.insert(new_node_id);

    EmitNumericChanged(ids);

    ApplyDelete(old_node_id);
}

void TreeModel::ApplyName(const QUuid& node_id, const QString& name, int version)
{
    const auto index { GetIndex(node_id) };
    if (!index.isValid())
        return;

    auto* node { static_cast<Node*>(index.internalPointer()) };
    if (!node)
        return;

    if (node->name == name)
        return;

    node->name = name;
    node->version = version;

    UpdateSubtreePath(node);
    const auto leaf_ids { ExtractLeafIds(node) };
    SyncLeafModel(leaf_ids);

    const int column { std::to_underlying(NodeEnum::kName) };
    const int row { index.row() };

    EmitDataChanged(row, row, column, column, index.parent());
    emit SUpdateName(node->id, node->name, node->kind == NodeKind::kBranch);
}

void TreeModel::ApplyDrag(const QUuid& ancestor, const QUuid& descendant)
{
    // Get the node to be moved
    auto* node { GetNode(descendant) };
    if (!node) {
        qWarning() << "DragNode: descendant node not found, skip move";
        return;
    }

    // Get destination parent node
    auto* destination_node { GetNode(ancestor) };
    if (!destination_node) {
        qWarning() << "DragNode: destination node not found, skip move";
        return;
    }

    // Get source parent node
    Node* source_parent_node { node->parent };
    if (!source_parent_node) {
        qWarning() << "DragNode: node has no source parent (top-level or detached), skip move";
        return;
    }

    // Same parent, nothing to do
    if (source_parent_node == destination_node) {
        qWarning() << "DragNode: source and destination parent are the same, skip move";
        return;
    }

    // Check for circular dependency (destination is descendant of node)
    if (node::IsDescendant(destination_node, node)) {
        qWarning() << "DragNode: cannot move node to its descendant, skip move";
        return;
    }

    // Calculate the destination row (insert at the end)
    const qsizetype destination_child { destination_node->children.size() };
    const QModelIndex destination_parent_index { GetIndex(ancestor) };
    if (!destination_parent_index.isValid()) {
        qInfo() << "DragNode: moving node to top-level";
    }

    // Find the row of the node in its parent's children list
    const qsizetype source_row { source_parent_node->children.indexOf(node) };
    if (source_row == -1) {
        qWarning() << "DragNode: node not found in source parent's children, skip move";
        return;
    }

    // Get the source parent index
    QModelIndex source_parent_index { GetIndex(source_parent_node->id) };
    if (!source_parent_index.isValid()) {
        qInfo() << "DragNode: moving top-level node";
    }

    // Perform the row move
    if (!moveRows(source_parent_index, source_row, 1, destination_parent_index, destination_child)) {
        qWarning() << "DragNode: moveRows failed";
    }
}

QModelIndex TreeModel::parent(const QModelIndex& index) const
{
    // root_'s index is QModelIndex(), root_'s id == -1
    if (!index.isValid())
        return {};

    auto* node { static_cast<Node*>(index.internalPointer()) };
    Q_ASSERT(node);

    auto* parent { node->parent };
    Q_ASSERT(parent);

    if (parent == root_)
        return {};

    // Parent node should have a parent (grandparent)
    auto* grandparent { parent->parent };
    Q_ASSERT(grandparent);

    // Find parent's row in grandparent's children
    const qsizetype row { grandparent->children.indexOf(parent) };
    Q_ASSERT(row >= 0);

    return createIndex(static_cast<int>(row), 0, parent);
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return {};

    auto* parent_node { GetNodeByIndex(parent) };
    auto* node { parent_node->children.at(row) };

    Q_ASSERT(node);

    return createIndex(row, column, node);
}

int TreeModel::rowCount(const QModelIndex& parent) const { return GetNodeByIndex(parent)->children.size(); }

QMimeData* TreeModel::mimeData(const QModelIndexList& indexes) const
{
    auto* mime_data { new QMimeData() };
    if (indexes.isEmpty())
        return mime_data;

    auto first_index { indexes.first() };

    if (first_index.isValid()) {
        auto* node { static_cast<Node*>(first_index.internalPointer()) };
        mime_data->setData(kYTX, node->id.toRfc4122());
    }

    return mime_data;
}

bool TreeModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
        return false;

    auto* destination_parent { GetNodeByIndex(parent) };
    if (!destination_parent) {
        qWarning() << "dropMimeData: destination parent not found";
        return false;
    }

    if (destination_parent->kind != NodeKind::kBranch)
        return false;

    const auto mime { data->data(kYTX) };
    if (mime.isEmpty()) {
        qWarning() << "dropMimeData: MIME data is empty";
        return false;
    }

    const QUuid node_id { QUuid::fromRfc4122(mime) };
    if (node_id.isNull()) {
        qWarning() << "dropMimeData: invalid UUID in MIME data";
        return false;
    }

    auto* node { node_hash_.value(node_id) };
    if (!node) {
        qWarning() << "dropMimeData: node not found (possibly deleted during drag)" << node_id;
        return false;
    }

    qInfo() << "[UI] dropMimeData";

    if (node->parent == destination_parent || node::IsDescendant(destination_parent, node))
        return false;

    int destination_child { row };
    if (row == -1) {
        destination_child = destination_parent->children.size();
    }
    destination_child = qBound(0, destination_child, destination_parent->children.size());

    auto* source_parent_node { node->parent };
    if (!source_parent_node) {
        qWarning() << "dropMimeData: source parent is null (node was orphaned during drag)";
        return false;
    }

    qsizetype source_row { source_parent_node->children.indexOf(node) };
    if (source_row == -1) {
        qCritical() << "dropMimeData: source row not found for node:" << node->id;
        return false;
    }

    QModelIndex source_parent_index { GetIndex(source_parent_node->id) };

    if (moveRows(source_parent_index, source_row, 1, parent, destination_child)) {
        const auto message { JsonGen::NodeDrag(section_, node_id, destination_parent->id) };
        WebSocket::Instance()->SendMessage(WsKey::kNodeDrag, message);
        return true;
    }

    return false;
}

bool TreeModel::moveRows(const QModelIndex& sourceParent, int sourceRow, int count, const QModelIndex& destinationParent, int destinationChild)
{
    if (sourceParent == destinationParent) {
        qWarning() << "moveRows: same parent move is not supported";
        return false;
    }

    auto* source_parent { GetNodeByIndex(sourceParent) };
    auto* destination_parent { GetNodeByIndex(destinationParent) };

    Q_ASSERT_X(source_parent, "TreeModel::moveRows", "Source parent is null");
    Q_ASSERT_X(destination_parent, "TreeModel::moveRows", "Destination parent is null");
    Q_ASSERT_X(count == 1, "TreeModel::moveRows", "Only single-row move is supported");
    Q_ASSERT_X(sourceRow >= 0 && sourceRow < source_parent->children.size(), "TreeModel::moveRows", "Source row is out of bounds");
    Q_ASSERT_X(destinationChild >= 0 && destinationChild <= destination_parent->children.size(), "TreeModel::moveRows", "Destination child is out of bounds");

    if (!beginMoveRows(sourceParent, sourceRow, sourceRow, destinationParent, destinationChild)) {
        qWarning() << "moveRows: beginMoveRows failed - invalid move operation";
        return false;
    }

    auto* node { source_parent->children.takeAt(sourceRow) };
    if (!node) {
        qCritical() << "moveRows: Node extraction failed!";
        return false;
    }

    const node::Delta delta_source {
        .initial = -node->initial_total,
        .final = -node->final_total,
    };

    const auto ids_source { UpdateAncestorTotal(node, delta_source) };

    destination_parent->children.insert(destinationChild, node);
    node->parent = destination_parent;

    const node::Delta delta_destination {
        .initial = node->initial_total,
        .final = node->final_total,
    };

    auto ids_destination { UpdateAncestorTotal(node, delta_destination) };
    endMoveRows();

    EmitNumericChanged(ids_destination.unite(ids_source));

    UpdateSubtreePath(node);
    const auto leaf_ids { ExtractLeafIds(node) };
    SyncLeafModel(leaf_ids);

    emit SUpdateName(node->id, node->name, node->kind == NodeKind::kBranch);

    return true;
}

ItemModel* TreeModel::PathModel(QWidget* parent) const
{
    auto* model { new ItemModel(parent) };

    QHash<QUuid, QString> paths {};
    paths.reserve(leaf_path_.size() + branch_path_.size());

    paths.insert(leaf_path_);
    paths.insert(branch_path_);

    model->Rebuild(paths);

    return model;
}

void TreeModel::UpdateSeparator(CString& old_separator, CString& new_separator)
{
    Q_ASSERT(!new_separator.isEmpty());
    Q_ASSERT(!old_separator.isEmpty());

    if (old_separator == new_separator)
        return;

    auto update_path_separator = [&](QHash<QUuid, QString>& source_path) {
        for (auto& path : source_path)
            path.replace(old_separator, new_separator);
    };

    update_path_separator(leaf_path_);
    update_path_separator(branch_path_);

    leaf_model_->SetSeparator(old_separator, new_separator);
}

void TreeModel::SearchName(QList<Node*>& node_list, CString& name) const
{
    if (name.isEmpty())
        return;

    for (const auto& [id, node_ptr] : node_hash_.asKeyValueRange()) {
        if (!node_ptr)
            continue;

        if (node_ptr->name.contains(name, Qt::CaseInsensitive)) {
            node_list.emplaceBack(node_ptr);
        }
    }
}

void TreeModel::SearchTag(QList<Node*>& node_list, const QSet<QString>& tag_set) const
{
    if (tag_set.isEmpty())
        return;

    for (const auto& [id, node] : node_hash_.asKeyValueRange()) {
        Q_ASSERT(node && "TreeModel::SearchTag encountered null node in cache");

        const QStringList& tags { node->tag };

        if (std::any_of(tags.cbegin(), tags.cend(), [&tag_set](const QString& tag_id) { return tag_set.contains(tag_id); })) {
            node_list.emplaceBack(node);
        }
    }
}

void TreeModel::Reset()
{
    FlushTimers();

    beginResetModel();
    ClearTree();
    InitUnitSet();
    endResetModel();
}

QModelIndex TreeModel::GetIndex(const QUuid& node_id) const
{
    if (node_id.isNull())
        return {};

    Node* node { node_hash_.value(node_id, nullptr) };
    if (!node) {
        qCritical() << Q_FUNC_INFO << "node_id not found in node_hash_";
        return {};
    }

    if (!node->parent) {
        qCritical() << Q_FUNC_INFO << "node is a root node";
        return {};
    }

    const qsizetype row { node->parent->children.indexOf(node) };
    Q_ASSERT(row >= 0);

    return createIndex(static_cast<int>(row), 0, node);
}

QString TreeModel::Path(const QUuid& node_id) const
{
    if (auto it = leaf_path_.constFind(node_id); it != leaf_path_.constEnd())
        return it.value();

    if (auto it = branch_path_.constFind(node_id); it != branch_path_.constEnd())
        return it.value();

    return {};
}

QSortFilterProxyModel* TreeModel::ExcludeId(const QUuid& node_id, QObject* parent) const
{
    auto* model { new ExcludeIdFilterModel(node_id, parent) };
    model->setSourceModel(leaf_model_);
    return model;
}

QSortFilterProxyModel* TreeModel::IncludeUnit(NodeUnit unit, QObject* parent)
{
    auto* set { UnitSet(unit) };
    auto* model { new IncludeUnitFilterModel(set, parent) };
    model->setSourceModel(leaf_model_);
    return model;
}

QSortFilterProxyModel* TreeModel::ReplaceSelf(const QUuid& node_id, NodeUnit unit, QObject* parent)
{
    auto* set { UnitSet(unit) };
    auto* model { new ReplaceSelfFilterModel(node_id, set, parent) };
    model->setSourceModel(leaf_model_);
    return model;
}

void TreeModel::AckNode(const QUuid& node_id) const
{
    if (node_hash_.contains(node_id))
        return;

    const auto message { JsonGen::NodeAck(section_, node_id) };
    WebSocket::Instance()->SendMessage(WsKey::kOrderNodeAck, message);
}

void TreeModel::InitTreeData(const QHash<QUuid, Node*>& node_hash, QHash<QUuid, QString>& leaf_path, QHash<QUuid, QString>& branch_path)
{
    for (auto* node : node_hash) {
        const QString path { path::Build(node, separator_) };

        switch (node->kind) {
        case NodeKind::kBranch:
            branch_path.insert(node->id, path);
            break;

        case NodeKind::kLeaf:
            if (node->status == NodeStatus::kActive)
                leaf_path.insert(node->id, path);

            const node::Delta delta {
                .initial = node->initial_total,
                .final = node->final_total,
            };

            InitAncestorTotal(node, delta);
            break;
        }
    }
}

Node* TreeModel::GetNodeByIndex(const QModelIndex& index) const
{
    if (index.isValid() && index.internalPointer())
        return static_cast<Node*>(index.internalPointer());

    return root_;
}

QSet<QUuid> TreeModel::UpdateAncestorTotal(Node* node, const node::Delta& delta) const
{
    QSet<QUuid> affected_ids {};

    if (!node || node == root_)
        return affected_ids;

    if (!node->parent || node->parent == root_)
        return affected_ids;

    if (delta.IsNull())
        return affected_ids;

    const auto unit { node->unit };
    const bool direction_rule { node->direction_rule };

    // NOTE: When ancestor nodes receive deltas from a leaf node,
    // the adjustment rule is different from leaf calculation:
    // - If the ancestor has the same direction rule as the leaf, add the delta.
    // - If the ancestor has the opposite direction rule, subtract the delta.
    for (Node* current = node->parent; current && current != root_; current = current->parent) {
        if (current->unit != unit)
            continue;

        const int multiplier { current->direction_rule == direction_rule ? 1 : -1 };

        current->final_total += multiplier * delta.final;
        current->initial_total += multiplier * delta.initial;

        affected_ids.insert(current->id);
    }

    return affected_ids;
}

void TreeModel::InitAncestorTotal(Node* node, const node::Delta& delta) const
{
    if (!node || !node->parent)
        return;

    if (delta.IsNull())
        return;

    const auto unit { node->unit };
    const bool direction_rule { node->direction_rule };

    // - If the ancestor has the same direction rule as the leaf, add the delta.
    // - If the ancestor has the opposite direction rule, subtract the delta.
    for (Node* current = node->parent; current; current = current->parent) {
        if (current->unit != unit)
            continue;

        const int multiplier { current->direction_rule == direction_rule ? 1 : -1 };

        current->final_total += multiplier * delta.final;
        current->initial_total += multiplier * delta.initial;
    }
}

void TreeModel::EmitNumericChanged(const QSet<QUuid>& ids)
{
    for (const QUuid& id : ids) {
        const QModelIndex index = GetIndex(id);
        if (!index.isValid())
            continue;

        const int row { index.row() };
        const auto [start, end] = node::NumericColumnRange(section_);
        EmitDataChanged(row, row, start, end, index.parent());
    }
}

void TreeModel::EmitColumnChanged(int column, const QSet<QUuid>& ids)
{
    for (const QUuid& id : ids) {
        const QModelIndex index = GetIndex(id);
        if (!index.isValid())
            continue;

        const int row { index.row() };

        EmitDataChanged(row, row, column, column, index.parent());
    }
}

void TreeModel::EmitDataChanged(int start_row, int end_row, int start_column, int end_column, const QModelIndex& parent)
{
    if (start_row < 0 || end_row >= rowCount(parent) || start_row > end_row) {
        qDebug() << Q_FUNC_INFO << "invalid row range" << start_row << end_row << "rowCount" << rowCount(parent);
        return;
    }

    if (start_column < 0 || end_column >= columnCount(parent) || start_column > end_column) {
        qDebug() << Q_FUNC_INFO << "invalid column range" << start_column << end_column << "columnCount" << columnCount(parent);
        return;
    }

    const QModelIndex top_left { index(start_row, start_column, parent) };
    const QModelIndex bottom_right { index(end_row, end_column, parent) };

    Q_ASSERT(top_left.parent() == bottom_right.parent());

    emit dataChanged(top_left, bottom_right, QList<int> { Qt::DisplayRole, Qt::EditRole });
}

void TreeModel::RestartTimer(const QUuid& id)
{
    auto*& timer { pending_updates_[id].timer };

    if (!timer) {
        timer = new QTimer { this };
        timer->setSingleShot(true);
        connect(timer, &QTimer::timeout, this, [this, id]() { FlushTimer(id); });
    }

    timer->start(time_const::kAutoCloseMs);
}

void TreeModel::FlushTimer(const QUuid& id)
{
    auto update { pending_updates_.take(id) };

    if (update.node && !update.changes.isEmpty()) {
        const QJsonObject message { JsonGen::NodeUpdate(section_, id, update.changes, update.node->version) };
        WebSocket::Instance()->SendMessage(WsKey::kNodeUpdate, message);
    }

    if (update.timer) {
        update.timer->stop();
        update.timer->deleteLater();
    }
}

void TreeModel::FlushTimers()
{
    const auto ids { pending_updates_.keys() };

    for (const auto& id : ids) {
        FlushTimer(id);
    }
}

void TreeModel::ApplyTree(const QJsonObject& data)
{
    QHash<QUuid, Node*> new_hash {};
    QHash<QUuid, QString> new_leaf_path {};
    QHash<QUuid, QString> new_branch_path {};

    BuildTreeData(data, new_hash, new_leaf_path, new_branch_path);

    beginResetModel();

    node_hash_ = std::move(new_hash);
    leaf_path_ = std::move(new_leaf_path);
    branch_path_ = std::move(new_branch_path);

    path::AttachRootNodes(node_hash_, root_);
    node::SortSubtree(root_, [](const Node* lhs, const Node* rhs) { return utils::CompareString(lhs->name, rhs->name, Qt::AscendingOrder); });

    InitLeafData();

    endResetModel();

    emit SInitStatus();
}

void TreeModel::BuildTreeData(const QJsonObject& data, QHash<QUuid, Node*>& node_hash, QHash<QUuid, QString>& leaf_path, QHash<QUuid, QString>& branch_path)
{
    const QJsonArray node_array { data.value(kNodeArray).toArray() };
    const QJsonArray path_array { data.value(kPathArray).toArray() };

    node_hash.reserve(node_array.size());

    for (const auto& value : node_array) {
        Q_ASSERT(value.isObject());

        auto* node { NodePool::Instance().Allocate(section_) };
        node->ReadJson(value.toObject());

        node_hash.insert(node->id, node);
    }

    const auto paths { path::Parse(path_array) };
    path::BuildHierarchy(node_hash, paths);

    InitTreeData(node_hash, leaf_path, branch_path);

    qDebug() << kSectionString.value(section_) << "nodes:" << node_hash.size() << "," << "leaf paths:" << leaf_path.size() << ","
             << "branch paths:" << branch_path.size();
}

// Initialize the root node.
// Root is always represented by an empty QUuid as its ID.
// Root always has direction_rule = true by definition.
void TreeModel::InitRoot()
{
    if (root_ == nullptr) {
        root_ = NodePool::Instance().Allocate(section_);
        root_->kind = NodeKind::kBranch;
        root_->direction_rule = false;
        root_->name = QString();
        root_->id = QUuid();
    }

    if (!root_) {
        qCritical() << "InitRoot: root node allocation failed!";
    }

    Q_ASSERT(root_ != nullptr);
}

void TreeModel::ClearTree()
{
    NodePool::Instance().Recycle(node_hash_, section_);

    root_->children.clear();
    leaf_path_.clear();
    branch_path_.clear();

    leaf_model_->Reset();
    ClearUnitSet();
}

void TreeModel::RegisterNode(Node* node)
{
    // NOTE: Unlike UnregisterPath, RegisterPath does not call
    // UpdateAncestorTotal here. Newly inserted nodes in this section
    // (Finance/Task/Inventory/Partner) are always created
    // with empty/default totals — values are filled in later by the user
    // — so there is nothing meaningful to propagate to ancestors at
    // registration time. Ancestor totals only need to be adjusted when a
    // node with actual data is removed (see UnregisterPath).

    const auto path { path::Build(node, root_, separator_) };
    const NodeKind kind { node->kind };

    switch (kind) {
    case NodeKind::kBranch:
        branch_path_.insert(node->id, path);
        break;
    case NodeKind::kLeaf:
        leaf_path_.insert(node->id, path);
        leaf_model_->AppendItem(path, node->id);
        UnitSetInsert(node->id, node->unit);
        break;
    }
}

void TreeModel::UnregisterNode(Node* node, Node* parent_node)
{
    // NOTE: A leaf node being removed may already carry real data
    // (initial_total/final_total), so its contribution must be reversed
    // from all ancestors via UpdateAncestorTotal (negated deltas) before
    // the node is dropped from the path index.

    const auto node_id { node->id };
    const NodeKind kind { node->kind };

    switch (kind) {
    case NodeKind::kBranch: {
        for (auto* child : std::as_const(node->children)) {
            child->parent = parent_node;
            parent_node->children.emplace_back(child);
        }

        UpdateSubtreePath(node);
        const auto leaf_ids { ExtractLeafIds(node) };
        SyncLeafModel(leaf_ids);

        branch_path_.remove(node_id);
        emit SUpdateName(node_id, node->name, true);
    } break;
    case NodeKind::kLeaf: {
        leaf_path_.remove(node_id);
        leaf_model_->RemoveItem(node_id);
        UnitSetRemove(node_id, node->unit);

        const node::Delta delta {
            .initial = -node->initial_total,
            .final = -node->final_total,
        };

        const auto ids { UpdateAncestorTotal(node, delta) };
        EmitNumericChanged(ids);

        emit SFreeWidget(section_, node_id);
        emit SInitStatus();
    } break;
    }
}
