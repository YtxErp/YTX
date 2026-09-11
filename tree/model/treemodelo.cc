#include "treemodelo.h"

#include <QJsonArray>

#include "global/masterdataregistry.h"
#include "global/nodepool.h"
#include "utils/nodeutils.h"
#include "utils/pathutils.h"

TreeModelO::TreeModelO(CSectionInfo& info, CString& separator, QObject* parent)
    : TreeModel(info, separator, parent)
{
}

void TreeModelO::HandleStatusChanged(const QUuid& node_id, OrderStatus value)
{
    auto* d_node { DerivedPtr<NodeO>(node_hash_.value(node_id)) };
    if (!d_node)
        return;

    const int coefficient { value == OrderStatus::kReleased ? 1 : -1 };

    const node::Delta delta {
        .initial = coefficient * d_node->initial_total,
        .final = coefficient * d_node->final_total,
        .count = coefficient * d_node->count_total,
        .measure = coefficient * d_node->measure_total,
        .discount = coefficient * d_node->discount_total,
    };

    const auto ids { UpdateAncestorTotal(d_node, delta) };

    EmitNumericChanged(ids);
}

void TreeModelO::ApplyName(const QUuid& node_id, const QString& name, int version)
{
    const auto index { GetIndex(node_id) };
    if (!index.isValid())
        return;

    auto* node { static_cast<Node*>(index.internalPointer()) };
    if (!node)
        return;

    if (node->kind != NodeKind::kBranch)
        return;

    node->name = name;
    node->version = version;

    UpdateSubtreePath(node);

    const int column { std::to_underlying(NodeEnumO::kName) };
    const int row { index.row() };

    EmitDataChanged(row, row, column, column, index.parent());
}

void TreeModelO::InsertSettlement(const QSet<QUuid>& settled_set, const QUuid& settlement_id)
{
    if (settled_set.isEmpty() || settlement_id.isNull())
        return;

    QSet<QUuid> affected_ids {};

    for (auto it = node_hash_.constBegin(); it != node_hash_.constEnd(); ++it) {
        auto* node = it.value();
        Q_ASSERT(node != nullptr);

        if (!settled_set.contains(node->id))
            continue;

        auto* d_node = static_cast<NodeO*>(node);
        Q_ASSERT(d_node != nullptr);

        d_node->settlement_id = settlement_id;
        d_node->final_total = d_node->initial_total - d_node->discount_total;
        d_node->version += 1;

        const node::Delta delta {
            .final = d_node->final_total,
        };

        const auto ids { UpdateAncestorTotal(d_node, delta) };
        affected_ids.unite(ids);
        affected_ids.insert(d_node->id);
    }

    EmitColumnChanged(std::to_underlying(NodeEnumO::kFinalTotal), affected_ids);
}

void TreeModelO::RecallSettlement(const QUuid& settlement_id)
{
    if (settlement_id.isNull())
        return;

    QSet<QUuid> affected_ids {};

    for (auto it = node_hash_.constBegin(); it != node_hash_.constEnd(); ++it) {
        auto* node = it.value();
        Q_ASSERT(node != nullptr);

        auto* d_node = static_cast<NodeO*>(node);
        Q_ASSERT(d_node != nullptr);

        if (d_node->settlement_id != settlement_id)
            continue;

        const node::Delta delta {
            .final = -d_node->final_total,
        };

        d_node->settlement_id = QUuid();
        d_node->final_total = {};
        d_node->version += 1;

        const auto ids { UpdateAncestorTotal(d_node, delta) };
        affected_ids.unite(ids);
        affected_ids.insert(d_node->id);
    }

    EmitColumnChanged(std::to_underlying(NodeEnumO::kFinalTotal), affected_ids);
}

void TreeModelO::RegisterNode(Node* node)
{
    // NOTE: In this section (Sale/Purchase), only branch nodes have a
    // meaningful hierarchical path. Leaf nodes (orders) are displayed by
    // their partner name rather than a path, so there is no leaf_path_
    // entry to register here — unlike TreeModel::RegisterPath, which
    // handles both branch and leaf kinds.

    const NodeKind kind { node->kind };

    switch (kind) {
    case NodeKind::kBranch:
        branch_path_.insert(node->id, path::Build(node, root_, separator_));
        break;

    case NodeKind::kLeaf: {
        auto* d_node { DerivedPtr<NodeO>(node) };

        if (d_node->order_status == OrderStatus::kReleased) {
            const node::Delta delta {
                .initial = d_node->initial_total,
                .final = d_node->final_total,
                .count = d_node->count_total,
                .measure = d_node->measure_total,
                .discount = d_node->discount_total,
            };

            const auto affected_ids { UpdateAncestorTotal(node, delta) };
            EmitNumericChanged(affected_ids);
        }

        break;
    }
    }
}

void TreeModelO::UnregisterNode(Node* node, Node* parent_node)
{
    const auto node_id { node->id };
    auto* d_node { DerivedPtr<NodeO>(node) };
    const NodeKind kind { d_node->kind };

    switch (kind) {
    case NodeKind::kBranch:
        for (auto* child : std::as_const(node->children)) {
            child->parent = parent_node;
            parent_node->children.emplace_back(child);
        }

        UpdateSubtreePath(node);
        branch_path_.remove(node_id);
        break;
    case NodeKind::kLeaf:
        emit SFreeWidget(section_, node_id);
        break;
    }
}

void TreeModelO::UpdateSubtreePath(const Node* node)
{
    QQueue<const Node*> queue {};
    queue.enqueue(node);

    while (!queue.isEmpty()) {
        const auto* current { queue.dequeue() };
        const NodeKind kind { current->kind };

        if (kind == NodeKind::kLeaf)
            continue;

        for (const auto* child : current->children)
            queue.enqueue(child);

        const auto path { path::Build(current, root_, separator_) };
        branch_path_.insert(current->id, path);
    }
}

QSet<QUuid> TreeModelO::UpdateAncestorTotal(Node* node, const node::Delta& delta) const
{
    QSet<QUuid> affected_ids {};

    if (!node || node == root_)
        return affected_ids;

    if (!node->parent || node->parent == root_)
        return affected_ids;

    if (delta.IsNull())
        return affected_ids;

    const auto unit { node->unit };

    for (Node* current = node->parent; current && current != root_; current = current->parent) {
        if (current->unit != unit)
            continue;

        auto* d_node { DerivedPtr<NodeO>(current) };

        d_node->initial_total += delta.initial;
        d_node->final_total += delta.final;
        d_node->count_total += delta.count;
        d_node->measure_total += delta.measure;
        d_node->discount_total += delta.discount;

        affected_ids.insert(current->id);
    }

    return affected_ids;
}

void TreeModelO::InitAncestorTotal(Node* node, const node::Delta& delta) const
{
    if (!node || !node->parent)
        return;

    if (delta.IsNull())
        return;

    const auto unit { node->unit };

    for (Node* current = node->parent; current; current = current->parent) {
        if (current->unit != unit)
            continue;

        auto* d_node { DerivedPtr<NodeO>(current) };

        d_node->initial_total += delta.initial;
        d_node->final_total += delta.final;
        d_node->count_total += delta.count;
        d_node->measure_total += delta.measure;
        d_node->discount_total += delta.discount;
    }
}

void TreeModelO::InitTreeData(const QHash<QUuid, Node*>& node_hash, QHash<QUuid, QString>& /*leaf_path*/, QHash<QUuid, QString>& branch_path)
{
    for (auto* node : node_hash) {
        switch (node->kind) {
        case NodeKind::kBranch:
            branch_path.insert(node->id, path::Build(node, separator_));
            break;
        case NodeKind::kLeaf: {
            auto* d_node { DerivedPtr<NodeO>(node) };
            if (d_node->order_status == OrderStatus::kReleased) {
                const node::Delta delta {
                    .initial = d_node->initial_total,
                    .final = d_node->final_total,
                    .count = d_node->count_total,
                    .measure = d_node->measure_total,
                    .discount = d_node->discount_total,
                };

                InitAncestorTotal(node, delta);
            }

            break;
        }
        }
    }
}

void TreeModelO::sort(int column, Qt::SortOrder order)
{
    const NodeEnumO e_column { column };

    auto DisplayName = [](const NodeO* node) -> const QString {
        if (node->kind == NodeKind::kBranch)
            return node->name;

        return MasterDataRegistry::Instance().PartnerName(node->partner_id);
    };

    auto Compare = [e_column, order, DisplayName](const Node* lhs, const Node* rhs) -> bool {
        auto* d_lhs = DerivedPtr<NodeO>(lhs);
        auto* d_rhs = DerivedPtr<NodeO>(rhs);

        switch (e_column) {
        case NodeEnumO::kName:
            return utils::CompareString(DisplayName(d_lhs), DisplayName(d_rhs), order);
        case NodeEnumO::kCode:
            return utils::CompareMember(lhs, rhs, &Node::code, order);
        case NodeEnumO::kDescription:
            return utils::CompareMember(lhs, rhs, &Node::description, order);
        case NodeEnumO::kDirectionRule:
            return utils::CompareMember(lhs, rhs, &Node::direction_rule, order);
        case NodeEnumO::kKind:
            return utils::CompareMember(lhs, rhs, &Node::kind, order);
        case NodeEnumO::kUnit:
            return utils::CompareMember(lhs, rhs, &Node::unit, order);
        case NodeEnumO::kEmployeeId:
            return utils::CompareMember(d_lhs, d_rhs, &NodeO::employee_id, order);
        case NodeEnumO::kIssuedTime:
            return utils::CompareMember(d_lhs, d_rhs, &NodeO::issued_time, order);
        case NodeEnumO::kCountTotal:
            return utils::CompareMember(d_lhs, d_rhs, &NodeO::count_total, order);
        case NodeEnumO::kMeasureTotal:
            return utils::CompareMember(d_lhs, d_rhs, &NodeO::measure_total, order);
        case NodeEnumO::kDiscountTotal:
            return utils::CompareMember(d_lhs, d_rhs, &NodeO::discount_total, order);
        case NodeEnumO::kStatus:
            return utils::CompareMember(d_lhs, d_rhs, &NodeO::order_status, order);
        case NodeEnumO::kInitialTotal:
            return utils::CompareMember(lhs, rhs, &Node::initial_total, order);
        case NodeEnumO::kFinalTotal:
            return utils::CompareMember(lhs, rhs, &Node::final_total, order);
        case NodeEnumO::kTag:
            return utils::CompareMember(lhs, rhs, &Node::tag, order);
        }
    };

    emit layoutAboutToBeChanged();
    node::SortSubtree(root_, Compare);
    emit layoutChanged();
}

QVariant TreeModelO::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    auto* d_node { static_cast<NodeO*>(index.internalPointer()) };
    Q_ASSERT(d_node != nullptr);

    const NodeEnumO column { index.column() };
    const bool is_branch { d_node->kind == NodeKind::kBranch };

    switch (column) {
    case NodeEnumO::kName:
        return is_branch ? d_node->name : MasterDataRegistry::Instance().PartnerName(d_node->partner_id);
    case NodeEnumO::kCode:
        return d_node->code;
    case NodeEnumO::kDescription:
        return d_node->description;
    case NodeEnumO::kDirectionRule:
        return d_node->direction_rule;
    case NodeEnumO::kKind:
        return std::to_underlying(d_node->kind);
    case NodeEnumO::kUnit:
        return std::to_underlying(d_node->unit);
    case NodeEnumO::kEmployeeId:
        return d_node->employee_id;
    case NodeEnumO::kIssuedTime:
        return is_branch ? QVariant() : d_node->issued_time;
    case NodeEnumO::kCountTotal:
        return d_node->count_total;
    case NodeEnumO::kMeasureTotal:
        return d_node->measure_total;
    case NodeEnumO::kDiscountTotal:
        return d_node->discount_total;
    case NodeEnumO::kStatus:
        return std::to_underlying(d_node->order_status);
    case NodeEnumO::kInitialTotal:
        return d_node->initial_total;
    case NodeEnumO::kFinalTotal:
        return d_node->final_total;
    case NodeEnumO::kTag:
        return d_node->tag;
    }
}

bool TreeModelO::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    if (data(index, role) == value)
        return false;

    auto* node { static_cast<Node*>(index.internalPointer()) };

    const QUuid id { node->id };
    auto& update { pending_updates_[id] };
    update.node = node;
    auto& changes { update.changes };

    const NodeEnumO column { index.column() };

    switch (column) {
    case NodeEnumO::kTag:
        node::UpdateStringList(changes, node, kTag, value.toStringList(), &Node::tag, [id, this]() { RestartTimer(id); });
        break;
    default:
        return false;
    }

    emit dataChanged(index, index, { Qt::DisplayRole, Qt::EditRole });
    return true;
}

Qt::ItemFlags TreeModelO::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };

    const NodeEnumO column { index.column() };
    switch (column) {
    case NodeEnumO::kName:
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        break;
    case NodeEnumO::kCode:
    case NodeEnumO::kDescription:
    case NodeEnumO::kIssuedTime:
    case NodeEnumO::kEmployeeId:
    case NodeEnumO::kStatus:
    case NodeEnumO::kTag:
    case NodeEnumO::kDirectionRule:
    case NodeEnumO::kKind:
    case NodeEnumO::kUnit:
    case NodeEnumO::kCountTotal:
    case NodeEnumO::kMeasureTotal:
    case NodeEnumO::kInitialTotal:
    case NodeEnumO::kDiscountTotal:
    case NodeEnumO::kFinalTotal:
        flags &= ~Qt::ItemIsEditable;
        break;
    }

    return flags;
}

bool TreeModelO::moveRows(const QModelIndex& sourceParent, int sourceRow, int count, const QModelIndex& destinationParent, int destinationChild)
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

    QSet<QUuid> ids_source {};
    QSet<QUuid> ids_destination {};

    if (!beginMoveRows(sourceParent, sourceRow, sourceRow, destinationParent, destinationChild)) {
        qWarning() << "moveRows: beginMoveRows failed - invalid move operation";
        return false;
    }

    auto* node { DerivedPtr<NodeO>(source_parent->children.takeAt(sourceRow)) };
    Q_ASSERT(node);

    const bool update_ancestor { node->kind == NodeKind::kBranch || node->order_status == OrderStatus::kReleased };

    if (update_ancestor) {
        const node::Delta delta {
            .initial = -node->initial_total,
            .final = -node->final_total,
            .count = -node->count_total,
            .measure = -node->measure_total,
            .discount = -node->discount_total,
        };

        ids_source = UpdateAncestorTotal(node, delta);
    }

    destination_parent->children.insert(destinationChild, node);
    node->parent = destination_parent;

    if (update_ancestor) {
        const node::Delta delta {
            .initial = node->initial_total,
            .final = node->final_total,
            .count = node->count_total,
            .measure = node->measure_total,
            .discount = node->discount_total,
        };

        ids_destination = UpdateAncestorTotal(node, delta);
    }

    endMoveRows();

    EmitNumericChanged(ids_destination.unite(ids_source));

    return true;
}

void TreeModelO::ApplyTree(const QJsonObject& data)
{
    QHash<QUuid, Node*> new_hash {};
    QHash<QUuid, QString> new_leaf_path {};
    QHash<QUuid, QString> new_branch_path {};

    BuildTreeData(data, new_hash, new_leaf_path, new_branch_path);

    beginResetModel();

    ClearTree();

    node_hash_ = std::move(new_hash);
    branch_path_ = std::move(new_branch_path);

    path::AttachRootNodes(node_hash_, root_);
    node::SortSubtree(root_, [](const Node* lhs, const Node* rhs) { return utils::CompareString(lhs->name, rhs->name, Qt::AscendingOrder); });

    endResetModel();
}

void TreeModelO::ClearTree()
{
    NodePool::Instance().Recycle(node_hash_, section_);

    root_->children.clear();
    branch_path_.clear();
}
