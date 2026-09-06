#include "treemodelp.h"

#include "utils/nodeutils.h"

TreeModelP::TreeModelP(CSectionInfo& info, CString& separator, QObject* parent)
    : TreeModel(info, separator, parent)
{
    leaf_model_ = new ItemModel(this);
}

void TreeModelP::UpdateAmount(const QUuid& node_id, double initial_delta)
{
    Q_ASSERT(!node_id.isNull());

    if (qFuzzyIsNull(initial_delta))
        return;

    auto* node { node_hash_.value(node_id) };
    if (!node || node == root_ || node->kind != NodeKind::kLeaf)
        return;

    node->initial_total += initial_delta;
    node->version += 1;

    const node::Delta delta {
        .initial = initial_delta,
    };

    auto ids { UpdateAncestorTotal(node, delta) };
    ids.insert(node_id);

    EmitColumnChanged(std::to_underlying(NodeEnumP::kInitialTotal), ids);
}

QSet<QUuid>* TreeModelP::UnitSet(NodeUnit unit)
{
    switch (unit) {
    case NodeUnit::PCustomer:
        return &cset_;
    case NodeUnit::PVendor:
        return &vset_;
    case NodeUnit::PEmployee:
        return &eset_;
    default:
        return nullptr;
    }
}

void TreeModelP::ClearUnitSet()
{
    cset_.clear();
    vset_.clear();
    eset_.clear();
}

void TreeModelP::InitUnitSet()
{
    // Empty employee is a valid state for orders.
    eset_.insert(QUuid());
    leaf_model_->AppendItem(QString(), QUuid());
}

QSet<QUuid> TreeModelP::UpdateAncestorTotal(Node* node, const node::Delta& delta) const
{
    QSet<QUuid> affected_ids {};

    if (!node || node == root_)
        return affected_ids;

    if (!node->parent || node->parent == root_)
        return affected_ids;

    if (qFuzzyIsNull(delta.initial))
        return affected_ids;

    const auto unit { node->unit };

    for (Node* current = node->parent; current && current != root_; current = current->parent) {
        if (current->unit != unit)
            continue;

        current->initial_total += delta.initial;

        affected_ids.insert(current->id);
    }

    return affected_ids;
}

void TreeModelP::InitAncestorTotal(Node* node, const node::Delta& delta) const
{
    if (!node || node == root_ || !node->parent || node->parent == root_)
        return;

    if (qFuzzyIsNull(delta.initial))
        return;

    const auto unit { node->unit };

    for (Node* current = node->parent; current && current != root_; current = current->parent) {
        if (current->unit != unit)
            continue;

        current->initial_total += delta.initial;
    }
}

void TreeModelP::sort(int column, Qt::SortOrder order)
{
    const NodeEnumP e_column { column };

    auto Compare = [e_column, order](const Node* lhs, const Node* rhs) -> bool {
        auto* d_lhs = DerivedPtr<NodeP>(lhs);
        auto* d_rhs = DerivedPtr<NodeP>(rhs);

        switch (e_column) {
        case NodeEnumP::kName:
            return utils::CompareMember(lhs, rhs, &Node::name, order);
        case NodeEnumP::kCode:
            return utils::CompareMember(lhs, rhs, &Node::code, order);
        case NodeEnumP::kDescription:
            return utils::CompareMember(lhs, rhs, &Node::description, order);
        case NodeEnumP::kKind:
            return utils::CompareMember(lhs, rhs, &Node::kind, order);
        case NodeEnumP::kUnit:
            return utils::CompareMember(lhs, rhs, &Node::unit, order);
        case NodeEnumP::kStatus:
            return utils::CompareMember(lhs, rhs, &Node::status, order);
        case NodeEnumP::kPaymentTerm:
            return utils::CompareMember(d_lhs, d_rhs, &NodeP::payment_term, order);
        case NodeEnumP::kInitialTotal:
            return utils::CompareMember(lhs, rhs, &Node::initial_total, order);
        case NodeEnumP::kColor:
            return utils::CompareColor(lhs, rhs, order);
        case NodeEnumP::kTag:
            return utils::CompareMember(lhs, rhs, &Node::tag, order);
        case NodeEnumP::kDocument:
            return (order == Qt::AscendingOrder) ? (d_lhs->document.size() < d_rhs->document.size()) : (d_lhs->document.size() > d_rhs->document.size());
        }
    };

    emit layoutAboutToBeChanged();
    node::SortSubtree(root_, Compare);
    emit layoutChanged();
}

QVariant TreeModelP::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    auto* d_node { static_cast<NodeP*>(index.internalPointer()) };
    Q_ASSERT(d_node != nullptr);

    const NodeEnumP column { index.column() };

    switch (column) {
    case NodeEnumP::kName:
        return d_node->name;
    case NodeEnumP::kCode:
        return d_node->code;
    case NodeEnumP::kDescription:
        return d_node->description;
    case NodeEnumP::kKind:
        return std::to_underlying(d_node->kind);
    case NodeEnumP::kTag:
        return d_node->tag;
    case NodeEnumP::kUnit:
        return std::to_underlying(d_node->unit);
    case NodeEnumP::kPaymentTerm:
        return d_node->payment_term;
    case NodeEnumP::kInitialTotal:
        return d_node->initial_total;
    case NodeEnumP::kColor:
        return d_node->color;
    case NodeEnumP::kDocument:
        return d_node->document;
    case NodeEnumP::kStatus:
        return std::to_underlying(d_node->status);
    }
}

bool TreeModelP::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    if (data(index, role) == value)
        return false;

    auto* node { static_cast<Node*>(index.internalPointer()) };
    auto* d_node { static_cast<NodeP*>(node) };

    Q_ASSERT(node != nullptr);
    Q_ASSERT(d_node != nullptr);

    const QUuid id { node->id };
    auto& update { pending_updates_[id] };
    update.node = node;
    auto& changes { update.changes };

    const NodeEnumP column { index.column() };

    switch (column) {
    case NodeEnumP::kCode:
        node::UpdateField(changes, node, kCode, value.toString(), &Node::code, [id, this]() { RestartTimer(id); });
        break;
    case NodeEnumP::kTag:
        node::UpdateStringList(changes, node, kTag, value.toStringList(), &Node::tag, [id, this]() { RestartTimer(id); });
        break;
    case NodeEnumP::kDescription:
        node::UpdateField(changes, node, kDescription, value.toString(), &Node::description, [id, this]() { RestartTimer(id); });
        break;
    case NodeEnumP::kPaymentTerm:
        node::UpdateField(changes, d_node, kPaymentTerm, value.toInt(), &NodeP::payment_term, [id, this]() { RestartTimer(id); });
        break;
    case NodeEnumP::kColor:
        node::UpdateField(changes, node, kColor, value.toString(), &Node::color, [id, this]() { RestartTimer(id); });
        break;
    case NodeEnumP::kDocument:
        node::UpdateStringList(changes, node, kDocument, value.toStringList(), &Node::document, [id, this]() { RestartTimer(id); });
        break;
    case NodeEnumP::kStatus:
        RequestStatus(node, value.toInt());
        break;
    case NodeEnumP::kName:
    case NodeEnumP::kKind:
    case NodeEnumP::kUnit:
    case NodeEnumP::kInitialTotal:
        return false;
    }

    emit dataChanged(index, index, { Qt::DisplayRole, Qt::EditRole });
    return true;
}

Qt::ItemFlags TreeModelP::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };

    auto* node { static_cast<Node*>(index.internalPointer()) };
    if (node->sync_state == SyncState::kDeleting)
        return flags & ~Qt::ItemIsEditable;

    const NodeEnumP column { index.column() };

    switch (column) {
    case NodeEnumP::kName:
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        flags &= ~Qt::ItemIsEditable;
        break;
    case NodeEnumP::kInitialTotal:
    case NodeEnumP::kUnit:
    case NodeEnumP::kColor:
    case NodeEnumP::kTag:
    case NodeEnumP::kDocument:
    case NodeEnumP::kKind:
        flags &= ~Qt::ItemIsEditable;
        break;
    case NodeEnumP::kStatus:
    case NodeEnumP::kPaymentTerm:
    case NodeEnumP::kCode:
    case NodeEnumP::kDescription:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}
