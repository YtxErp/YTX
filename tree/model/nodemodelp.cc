#include "nodemodelp.h"

NodeModelP::NodeModelP(CNodeModelArg& arg, QObject* parent)
    : NodeModel(arg, parent)
{
    IniModel();
    ConstructTree();
}

NodeModelP::~NodeModelP() { qDeleteAll(node_hash_); }

void NodeModelP::RUpdateLeafValue(
    int node_id, double initial_debit_delta, double initial_credit_delta, double final_debit_delta, double final_credit_delta, double /*settled_delta*/)
{
    auto* node { NodeModelUtils::GetNode(node_hash_, node_id) };
    if (!node || node == root_ || node->type != kTypeLeaf)
        return;

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

void NodeModelP::RUpdateMultiLeafTotal(const QList<int>& node_list)
{
    double old_final_total {};
    double old_initial_total {};
    double final_delta {};
    double initial_delta {};
    Node* node {};

    for (int node_id : node_list) {
        node = NodeModelUtils::GetNode(node_hash_, node_id);

        if (!node || node->type != kTypeLeaf)
            continue;

        old_final_total = node->final_total;
        old_initial_total = node->initial_total;

        sql_->ReadLeafTotal(node);
        sql_->SyncLeafValue(node);

        final_delta = node->final_total - old_final_total;
        initial_delta = node->initial_total - old_initial_total;

        UpdateAncestorValue(node, initial_delta, final_delta);
    }

    emit SUpdateStatusValue();
}

void NodeModelP::UpdateSeparator(CString& old_separator, CString& new_separator)
{
    if (old_separator == new_separator || new_separator.isEmpty())
        return;

    NodeModelUtils::UpdatePathSeparator(old_separator, new_separator, leaf_path_);
    NodeModelUtils::UpdatePathSeparator(old_separator, new_separator, branch_path_);
    NodeModelUtils::UpdatePathSeparator(old_separator, new_separator, support_path_);

    NodeModelUtils::UpdateModelSeparator(leaf_model_, leaf_path_);
    NodeModelUtils::UpdateModelSeparator(support_model_, support_path_);
    NodeModelUtils::UpdateModelSeparator(product_model_, leaf_path_);
}

bool NodeModelP::UpdateUnit(Node* node, int value)
{
    if (node->unit == value)
        return false;

    const int node_id { node->id };
    QString message { tr("Cannot change %1 unit,").arg(Path(node_id)) };

    if (NodeModelUtils::HasChildren(node, message))
        return false;

    if (NodeModelUtils::IsInternalReferenced(sql_, node_id, message))
        return false;

    if (NodeModelUtils::IsExternalReferenced(sql_, node_id, message))
        return false;

    if (NodeModelUtils::IsSupportReferenced(sql_, node_id, message))
        return false;

    if (node->type == kTypeLeaf) {
        if (value == std::to_underlying(UnitP::kPos))
            NodeModelUtils::RemoveItem(product_model_, node_id);
        else
            NodeModelUtils::AppendItem(product_model_, node_id, leaf_path_.value(node_id));
    }

    node->unit = value;
    sql_->WriteField(info_.node, kUnit, value, node_id);

    return true;
}

void NodeModelP::RemovePathLeaf(int node_id, int unit)
{
    NodeModelUtils::RemoveItem(leaf_model_, node_id);

    if (unit != std::to_underlying(UnitP::kPos))
        NodeModelUtils::RemoveItem(product_model_, node_id);
}

void NodeModelP::InsertPathLeaf(int node_id, CString& path, int unit)
{
    NodeModelUtils::AppendItem(leaf_model_, node_id, path);

    if (unit != std::to_underlying(UnitP::kPos)) {
        NodeModelUtils::AppendItem(product_model_, node_id, path);
    }
}

void NodeModelP::SortModel()
{
    product_model_->sort(0);
    support_model_->sort(0);
    leaf_model_->sort(0);
}

void NodeModelP::IniModel()
{
    leaf_model_ = new QStandardItemModel(this);
    support_model_ = new QStandardItemModel(this);
    product_model_ = new QStandardItemModel(this);

    NodeModelUtils::AppendItem(support_model_, 0, {});
}

bool NodeModelP::UpdateNameFunction(Node* node, CString& value)
{
    node->name = value;
    sql_->WriteField(info_.node, kName, value, node->id);

    NodeModelUtils::UpdatePath(leaf_path_, branch_path_, support_path_, root_, node, separator_);
    NodeModelUtils::UpdateModel(leaf_path_, leaf_model_, support_path_, support_model_, node);
    NodeModelUtils::UpdateUnitModel(leaf_path_, product_model_, node, std::to_underlying(UnitP::kPos), Filter::kExcludeSpecific);

    emit SResizeColumnToContents(std::to_underlying(NodeEnum::kName));
    emit SSearch();
    return true;
}

bool NodeModelP::UpdateAncestorValue(
    Node* node, double initial_delta, double final_delta, double /*first_delta*/, double /*second_delta*/, double /*discount_delta*/)
{
    if (!node || node == root_ || !node->parent || node->parent == root_)
        return false;

    if (initial_delta == 0.0 && final_delta == 0.0)
        return false;

    const bool kRule { node->rule };

    for (Node* current = node->parent; current && current != root_; current = current->parent) {
        bool equal { current->rule == kRule };

        current->final_total += (equal ? 1 : -1) * final_delta;
        current->initial_total += (equal ? 1 : -1) * initial_delta;
    }

    return true;
}

void NodeModelP::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.node_header.size())
        return;

    auto Compare = [column, order](const Node* lhs, const Node* rhs) -> bool {
        const NodeEnumP kColumn { column };
        switch (kColumn) {
        case NodeEnumP::kName:
            return (order == Qt::AscendingOrder) ? (lhs->name < rhs->name) : (lhs->name > rhs->name);
        case NodeEnumP::kCode:
            return (order == Qt::AscendingOrder) ? (lhs->code < rhs->code) : (lhs->code > rhs->code);
        case NodeEnumP::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case NodeEnumP::kNote:
            return (order == Qt::AscendingOrder) ? (lhs->note < rhs->note) : (lhs->note > rhs->note);
        case NodeEnumP::kRule:
            return (order == Qt::AscendingOrder) ? (lhs->rule < rhs->rule) : (lhs->rule > rhs->rule);
        case NodeEnumP::kType:
            return (order == Qt::AscendingOrder) ? (lhs->type < rhs->type) : (lhs->type > rhs->type);
        case NodeEnumP::kUnit:
            return (order == Qt::AscendingOrder) ? (lhs->unit < rhs->unit) : (lhs->unit > rhs->unit);
        case NodeEnumP::kColor:
            return (order == Qt::AscendingOrder) ? (lhs->color < rhs->color) : (lhs->color > rhs->color);
        case NodeEnumP::kCommission:
            return (order == Qt::AscendingOrder) ? (lhs->second < rhs->second) : (lhs->second > rhs->second);
        case NodeEnumP::kUnitPrice:
            return (order == Qt::AscendingOrder) ? (lhs->first < rhs->first) : (lhs->first > rhs->first);
        case NodeEnumP::kQuantity:
            return (order == Qt::AscendingOrder) ? (lhs->initial_total < rhs->initial_total) : (lhs->initial_total > rhs->initial_total);
        case NodeEnumP::kAmount:
            return (order == Qt::AscendingOrder) ? (lhs->final_total < rhs->final_total) : (lhs->final_total > rhs->final_total);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    NodeModelUtils::SortIterative(root_, Compare);
    emit layoutChanged();
}

QVariant NodeModelP::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* node { GetNodeByIndex(index) };
    if (node == root_)
        return QVariant();

    const NodeEnumP kColumn { index.column() };
    const bool kIsLeaf { node->type == kTypeLeaf };

    switch (kColumn) {
    case NodeEnumP::kName:
        return node->name;
    case NodeEnumP::kID:
        return node->id;
    case NodeEnumP::kCode:
        return node->code;
    case NodeEnumP::kDescription:
        return node->description;
    case NodeEnumP::kNote:
        return node->note;
    case NodeEnumP::kRule:
        return node->rule;
    case NodeEnumP::kType:
        return node->type;
    case NodeEnumP::kUnit:
        return node->unit;
    case NodeEnumP::kColor:
        return node->color;
    case NodeEnumP::kCommission:
        return kIsLeaf && node->second != 0 ? node->second : QVariant();
    case NodeEnumP::kUnitPrice:
        return kIsLeaf && node->first != 0 ? node->first : QVariant();
    case NodeEnumP::kQuantity:
        return node->initial_total;
    case NodeEnumP::kAmount:
        return node->final_total;
    default:
        return QVariant();
    }
}

bool NodeModelP::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    auto* node { GetNodeByIndex(index) };
    if (node == root_)
        return false;

    const NodeEnumP kColumn { index.column() };

    switch (kColumn) {
    case NodeEnumP::kCode:
        NodeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kCode, &Node::code);
        break;
    case NodeEnumP::kDescription:
        NodeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kDescription, &Node::description);
        break;
    case NodeEnumP::kNote:
        NodeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kNote, &Node::note);
        break;
    case NodeEnumP::kRule:
        UpdateRule(node, value.toBool());
        emit dataChanged(index.siblingAtColumn(std::to_underlying(NodeEnumP::kQuantity)), index.siblingAtColumn(std::to_underlying(NodeEnumP::kAmount)));
        break;
    case NodeEnumP::kType:
        UpdateType(node, value.toInt());
        break;
    case NodeEnumP::kColor:
        NodeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kColor, &Node::color, true);
        break;
    case NodeEnumP::kUnit:
        UpdateUnit(node, value.toInt());
        break;
    case NodeEnumP::kCommission:
        NodeModelUtils::UpdateField(sql_, node, info_.node, value.toDouble(), kCommission, &Node::second, true);
        break;
    case NodeEnumP::kUnitPrice:
        NodeModelUtils::UpdateField(sql_, node, info_.node, value.toDouble(), kUnitPrice, &Node::first, true);
        break;
    default:
        return false;
    }

    emit SResizeColumnToContents(index.column());
    return true;
}

Qt::ItemFlags NodeModelP::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const NodeEnumP kColumn { index.column() };

    switch (kColumn) {
    case NodeEnumP::kName:
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        flags &= ~Qt::ItemIsEditable;
        break;
    case NodeEnumP::kQuantity:
    case NodeEnumP::kAmount:
    case NodeEnumP::kColor:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}

bool NodeModelP::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
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
    if (!node || node->parent == destination_parent || NodeModelUtils::IsDescendant(destination_parent, node))
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
    NodeModelUtils::UpdateUnitModel(leaf_path_, product_model_, node, std::to_underlying(UnitP::kPos), Filter::kExcludeSpecific);

    emit SUpdateName(node_id, node->name, node->type == kTypeBranch);
    emit SResizeColumnToContents(std::to_underlying(NodeEnum::kName));

    return true;
}
