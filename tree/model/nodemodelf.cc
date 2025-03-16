#include "nodemodelf.h"

NodeModelF::NodeModelF(CNodeModelArg& arg, QObject* parent)
    : NodeModel(arg, parent)
{
    IniModel();
    ConstructTree();
}

NodeModelF::~NodeModelF() { qDeleteAll(node_hash_); }

void NodeModelF::RUpdateLeafValue(
    int node_id, double initial_debit_delta, double initial_credit_delta, double final_debit_delta, double final_credit_delta, double /*settled*/)
{
    auto* node { NodeModelUtils::GetNode(node_hash_, node_id) };
    assert(node && node->type == kTypeLeaf && "Node must be non-null and of type kTypeLeaf");

    if (initial_credit_delta == 0.0 && initial_debit_delta == 0.0 && final_debit_delta == 0.0 && final_credit_delta == 0.0)
        return;

    bool rule { node->rule };

    double initial_delta { (rule ? 1 : -1) * (initial_credit_delta - initial_debit_delta) };
    double final_delta { (rule ? 1 : -1) * (final_credit_delta - final_debit_delta) };

    node->initial_total += initial_delta;
    node->final_total += final_delta;

    sql_->SyncLeafValue(node);
    UpdateAncestorValue(node, initial_delta, final_delta);
    emit SUpdateStatusValue();
}

void NodeModelF::UpdateDefaultUnit(int default_unit)
{
    if (root_->unit == default_unit)
        return;

    root_->unit = default_unit;

    for (auto* node : std::as_const(node_hash_))
        if (node->type == kTypeBranch && node->unit != default_unit)
            NodeModelUtils::UpdateBranchUnit(root_, node);
}

QVariant NodeModelF::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* node { GetNodeByIndex(index) };
    if (node == root_)
        return QVariant();

    const NodeEnumF kColumn { index.column() };

    switch (kColumn) {
    case NodeEnumF::kName:
        return node->name;
    case NodeEnumF::kID:
        return node->id;
    case NodeEnumF::kCode:
        return node->code;
    case NodeEnumF::kDescription:
        return node->description;
    case NodeEnumF::kNote:
        return node->note;
    case NodeEnumF::kRule:
        return node->rule;
    case NodeEnumF::kType:
        return node->type;
    case NodeEnumF::kUnit:
        return node->unit;
    case NodeEnumF::kForeignTotal:
        return node->unit == root_->unit ? QVariant() : node->initial_total;
    case NodeEnumF::kLocalTotal:
        return node->final_total;
    default:
        return QVariant();
    }
}

bool NodeModelF::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    auto* node { GetNodeByIndex(index) };
    if (node == root_)
        return false;

    const NodeEnumF kColumn { index.column() };

    switch (kColumn) {
    case NodeEnumF::kCode:
        NodeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kCode, &Node::code);
        break;
    case NodeEnumF::kDescription:
        NodeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kDescription, &Node::description);
        break;
    case NodeEnumF::kNote:
        NodeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kNote, &Node::note);
        break;
    case NodeEnumF::kRule:
        UpdateRule(node, value.toBool());
        emit dataChanged(
            index.siblingAtColumn(std::to_underlying(NodeEnumF::kForeignTotal)), index.siblingAtColumn(std::to_underlying(NodeEnumF::kLocalTotal)));
        break;
    case NodeEnumF::kType:
        UpdateType(node, value.toInt());
        break;
    case NodeEnumF::kUnit:
        UpdateUnit(node, value.toInt());
        break;
    default:
        return false;
    }

    emit SResizeColumnToContents(index.column());
    return true;
}

void NodeModelF::sort(int column, Qt::SortOrder order)
{
    assert(column >= 0 && column < info_.node_header.size() && "Column index out of range");

    auto Compare = [column, order](const Node* lhs, const Node* rhs) -> bool {
        const NodeEnumF kColumn { column };
        switch (kColumn) {
        case NodeEnumF::kName:
            return (order == Qt::AscendingOrder) ? (lhs->name < rhs->name) : (lhs->name > rhs->name);
        case NodeEnumF::kCode:
            return (order == Qt::AscendingOrder) ? (lhs->code < rhs->code) : (lhs->code > rhs->code);
        case NodeEnumF::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case NodeEnumF::kNote:
            return (order == Qt::AscendingOrder) ? (lhs->note < rhs->note) : (lhs->note > rhs->note);
        case NodeEnumF::kRule:
            return (order == Qt::AscendingOrder) ? (lhs->rule < rhs->rule) : (lhs->rule > rhs->rule);
        case NodeEnumF::kType:
            return (order == Qt::AscendingOrder) ? (lhs->type < rhs->type) : (lhs->type > rhs->type);
        case NodeEnumF::kUnit:
            return (order == Qt::AscendingOrder) ? (lhs->unit < rhs->unit) : (lhs->unit > rhs->unit);
        case NodeEnumF::kForeignTotal:
            return (order == Qt::AscendingOrder) ? (lhs->initial_total < rhs->initial_total) : (lhs->initial_total > rhs->initial_total);
        case NodeEnumF::kLocalTotal:
            return (order == Qt::AscendingOrder) ? (lhs->final_total < rhs->final_total) : (lhs->final_total > rhs->final_total);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    NodeModelUtils::SortIterative(root_, Compare);
    emit layoutChanged();
}

Qt::ItemFlags NodeModelF::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const NodeEnumF kColumn { index.column() };

    switch (kColumn) {
    case NodeEnumF::kName:
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        flags &= ~Qt::ItemIsEditable;
        break;
    case NodeEnumF::kForeignTotal:
    case NodeEnumF::kLocalTotal:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}

bool NodeModelF::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
        return false;

    auto* destination_parent { GetNodeByIndex(parent) };
    if (destination_parent->type != kTypeBranch)
        return false;

    int node_id {};

    if (auto mime { data->data(kNodeID) }; !mime.isEmpty())
        node_id = QVariant(mime).toInt();

    auto* node { NodeModelUtils::GetNode(node_hash_, node_id) };
    assert(node && "Node must be non-null");

    if (node->parent == destination_parent || NodeModelUtils::IsDescendant(destination_parent, node))
        return false;

    auto begin_row { row == -1 ? destination_parent->children.size() : row };
    auto source_row { node->parent->children.indexOf(node) };
    auto source_index { createIndex(node->parent->children.indexOf(node), 0, node) };

    if (beginMoveRows(source_index.parent(), source_row, source_row, parent, begin_row)) {
        node->parent->children.removeAt(source_row);
        UpdateAncestorValue(node, -node->initial_total, -node->final_total);

        destination_parent->children.insert(begin_row, node);
        node->parent = destination_parent;
        UpdateAncestorValue(node, node->initial_total, node->final_total);

        endMoveRows();
    }

    sql_->DragNode(destination_parent->id, node_id);
    NodeModelUtils::UpdatePath(leaf_path_, branch_path_, support_path_, root_, node, separator_);
    NodeModelUtils::UpdateModel(leaf_path_, leaf_model_, support_path_, support_model_, node);
    emit SUpdateName(node_id, node->name, node->type == kTypeBranch);
    emit SResizeColumnToContents(std::to_underlying(NodeEnum::kName));

    return true;
}

bool NodeModelF::UpdateUnit(Node* node, int value)
{
    if (node->unit == value)
        return false;

    int node_id { node->id };
    auto message { tr("Cannot change %1 unit,").arg(Path(node_id)) };

    if (NodeModelUtils::IsInternalReferenced(sql_, node_id, message))
        return false;

    if (NodeModelUtils::IsSupportReferenced(sql_, node_id, message))
        return false;

    node->unit = value;
    sql_->WriteField(info_.node, kUnit, value, node_id);

    if (node->type == kTypeBranch)
        NodeModelUtils::UpdateBranchUnit(root_, node);

    return true;
}

bool NodeModelF::UpdateAncestorValue(Node* node, double initial_delta, double final_delta, double /*first*/, double /*second*/, double /*discount*/)
{
    assert(node && node != root_ && node->parent && "Invalid node: node must be non-null, not the root, and must have a valid parent");

    if (node->parent == root_)
        return false;

    if (initial_delta == 0.0 && final_delta == 0.0)
        return false;

    const int kUnit { node->unit };
    const bool kRule { node->rule };

    for (Node* current = node->parent; current && current != root_; current = current->parent) {
        bool equal { current->rule == kRule };
        current->final_total += (equal ? 1 : -1) * final_delta;

        if (current->unit == kUnit) {
            current->initial_total += (equal ? 1 : -1) * initial_delta;
        }
    }

    return true;
}
