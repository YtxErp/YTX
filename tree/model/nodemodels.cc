#include "nodemodels.h"

#include "tree/includesetfiltermodel.h"

NodeModelS::NodeModelS(CNodeModelArg& arg, QObject* parent)
    : NodeModel(arg, parent)
{
    IniModel();
    NodeModelUtils::AppendItem(leaf_model_, 0, {});
    ConstructTree();
}

NodeModelS::~NodeModelS() { qDeleteAll(node_hash_); }

void NodeModelS::RUpdateStakeholder(int old_node_id, int new_node_id)
{
    for (auto* node : std::as_const(node_hash_)) {
        if (node->employee == old_node_id)
            node->employee = new_node_id;
    }
}

void NodeModelS::RSyncDouble(int node_id, int column, double value)
{
    assert(node_id >= 1 && "node_id must be positive");

    if (column != std::to_underlying(NodeEnumS::kAmount) || value == 0.0)
        return;

    auto* node { node_hash_.value(node_id) };
    if (!node || node == root_ || node->type != kTypeLeaf)
        return;

    node->final_total += value;
    sql_->WriteField(info_.node, kAmount, node->final_total, node_id);

    UpdateAncestorValue(node, 0.0, value);
}

void NodeModelS::RUpdateMultiLeafTotal(const QList<int>& node_list)
{
    for (int node_id : node_list) {
        auto* node { NodeModelUtils::GetNode(node_hash_, node_id) };

        assert(node && node->type == kTypeLeaf && "Node must be non-null and of type kTypeLeaf");

        const double old_final_total { node->final_total };

        sql_->ReadLeafTotal(node);
        sql_->SyncLeafValue(node);

        const double final_delta { node->final_total - old_final_total };

        UpdateAncestorValue(node, 0.0, final_delta);
    }
}

QList<int> NodeModelS::PartyList(CString& text, int unit) const
{
    QList<int> list {};

    for (auto* node : node_hash_)
        if (node->unit == unit && node->name.contains(text))
            list.emplaceBack(node->id);

    return list;
}

const QSet<int>* NodeModelS::UnitSet(int unit) const
{
    const UnitS kUnit { unit };

    switch (kUnit) {
    case UnitS::kCust:
        return &cset_;
    case UnitS::kVend:
        return &vset_;
    case UnitS::kEmp:
        return &eset_;
    default:
        return nullptr;
    }
}

QSortFilterProxyModel* NodeModelS::IncludeUnitModel(int unit)
{
    auto* set { UnitSet(unit) };
    auto* model { new IncludeSetFilterModel(set, this) };
    model->setSourceModel(leaf_model_);
    QObject::connect(this, &NodeModel::SSyncFilterModel, model, &IncludeSetFilterModel::RSyncFilterModel);
    return model;
}

bool NodeModelS::UpdateAncestorValue(Node* node, double /*initial*/, double final_delta, double /*first*/, double /*second*/, double /*discount*/)
{
    assert(node && node != root_ && node->parent && "Invalid node: node must be non-null, not the root, and must have a valid parent");

    if (node->parent == root_)
        return false;

    if (final_delta == 0.0)
        return false;

    const int kUnit { node->unit };

    for (Node* current = node->parent; current && current != root_; current = current->parent) {
        if (current->unit == kUnit) {
            current->final_total += final_delta;
        }
    }

    return true;
}

void NodeModelS::RemoveUnitSet(int node_id, int unit)
{
    const UnitS kUnit { unit };

    switch (kUnit) {
    case UnitS::kCust:
        cset_.remove(node_id);
        break;
    case UnitS::kVend:
        vset_.remove(node_id);
        break;
    case UnitS::kEmp:
        eset_.remove(node_id);
        break;
    default:
        break;
    }
}

void NodeModelS::InsertUnitSet(int node_id, int unit)
{
    const UnitS kUnit { unit };

    switch (kUnit) {
    case UnitS::kCust:
        cset_.insert(node_id);
        break;
    case UnitS::kVend:
        vset_.insert(node_id);
        break;
    case UnitS::kEmp:
        eset_.insert(node_id);
        break;
    default:
        break;
    }
}

void NodeModelS::sort(int column, Qt::SortOrder order)
{
    assert(column >= 0 && column < info_.node_header.size() && "Column index out of range");

    auto Compare = [column, order](const Node* lhs, const Node* rhs) -> bool {
        const NodeEnumS kColumn { column };
        switch (kColumn) {
        case NodeEnumS::kName:
            return (order == Qt::AscendingOrder) ? (lhs->name < rhs->name) : (lhs->name > rhs->name);
        case NodeEnumS::kCode:
            return (order == Qt::AscendingOrder) ? (lhs->code < rhs->code) : (lhs->code > rhs->code);
        case NodeEnumS::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case NodeEnumS::kNote:
            return (order == Qt::AscendingOrder) ? (lhs->note < rhs->note) : (lhs->note > rhs->note);
        case NodeEnumS::kType:
            return (order == Qt::AscendingOrder) ? (lhs->type < rhs->type) : (lhs->type > rhs->type);
        case NodeEnumS::kUnit:
            return (order == Qt::AscendingOrder) ? (lhs->unit < rhs->unit) : (lhs->unit > rhs->unit);
        case NodeEnumS::kDeadline:
            return (order == Qt::AscendingOrder) ? (lhs->date_time < rhs->date_time) : (lhs->date_time > rhs->date_time);
        case NodeEnumS::kEmployee:
            return (order == Qt::AscendingOrder) ? (lhs->employee < rhs->employee) : (lhs->employee > rhs->employee);
        case NodeEnumS::kPaymentTerm:
            return (order == Qt::AscendingOrder) ? (lhs->first < rhs->first) : (lhs->first > rhs->first);
        case NodeEnumS::kTaxRate:
            return (order == Qt::AscendingOrder) ? (lhs->second < rhs->second) : (lhs->second > rhs->second);
        case NodeEnumS::kAmount:
            return (order == Qt::AscendingOrder) ? (lhs->final_total < rhs->final_total) : (lhs->final_total > rhs->final_total);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    NodeModelUtils::SortIterative(root_, Compare);
    emit layoutChanged();
}

QVariant NodeModelS::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* node { GetNodeByIndex(index) };
    if (node == root_)
        return QVariant();

    const NodeEnumS kColumn { index.column() };
    const bool kIsLeaf { node->type == kTypeLeaf };

    switch (kColumn) {
    case NodeEnumS::kName:
        return node->name;
    case NodeEnumS::kID:
        return node->id;
    case NodeEnumS::kCode:
        return node->code;
    case NodeEnumS::kDescription:
        return node->description;
    case NodeEnumS::kNote:
        return node->note;
    case NodeEnumS::kType:
        return node->type;
    case NodeEnumS::kUnit:
        return node->unit;
    case NodeEnumS::kDeadline:
        return kIsLeaf && node->rule == kRuleMS ? node->date_time : QVariant();
    case NodeEnumS::kEmployee:
        return kIsLeaf && node->employee != 0 ? node->employee : QVariant();
    case NodeEnumS::kPaymentTerm:
        return kIsLeaf && node->first != 0 ? node->first : QVariant();
    case NodeEnumS::kTaxRate:
        return kIsLeaf && node->second != 0 ? node->second : QVariant();
    case NodeEnumS::kAmount:
        return node->final_total;
    default:
        return QVariant();
    }
}

bool NodeModelS::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    auto* node { GetNodeByIndex(index) };
    if (node == root_)
        return false;

    const NodeEnumS kColumn { index.column() };

    switch (kColumn) {
    case NodeEnumS::kCode:
        NodeModelUtils::UpdateField(sql_, node, info_.node, kCode, value.toString(), &Node::code);
        break;
    case NodeEnumS::kDescription:
        NodeModelUtils::UpdateField(sql_, node, info_.node, kDescription, value.toString(), &Node::description);
        break;
    case NodeEnumS::kNote:
        NodeModelUtils::UpdateField(sql_, node, info_.node, kNote, value.toString(), &Node::note);
        break;
    case NodeEnumS::kType:
        UpdateType(node, value.toInt());
        break;
    case NodeEnumS::kUnit:
        UpdateUnit(node, value.toInt());
        break;
    case NodeEnumS::kDeadline:
        NodeModelUtils::UpdateField(sql_, node, info_.node, kDeadline, value.toString(), &Node::date_time, true);
        break;
    case NodeEnumS::kEmployee:
        NodeModelUtils::UpdateField(sql_, node, info_.node, kEmployee, value.toInt(), &Node::employee, true);
        break;
    case NodeEnumS::kPaymentTerm:
        NodeModelUtils::UpdateField(sql_, node, info_.node, kPaymentTerm, value.toDouble(), &Node::first, true);
        break;
    case NodeEnumS::kTaxRate:
        NodeModelUtils::UpdateField(sql_, node, info_.node, kTaxRate, value.toDouble(), &Node::second, true);
        break;
    default:
        return false;
    }

    emit SResizeColumnToContents(index.column());
    return true;
}

Qt::ItemFlags NodeModelS::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const NodeEnumS kColumn { index.column() };

    switch (kColumn) {
    case NodeEnumS::kName:
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        flags &= ~Qt::ItemIsEditable;
        break;
    case NodeEnumS::kAmount:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}

bool NodeModelS::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
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
        UpdateAncestorValue(node, 0.0, -node->final_total);

        destination_parent->children.insert(begin_row, node);
        node->parent = destination_parent;
        UpdateAncestorValue(node, 0.0, node->final_total);

        endMoveRows();
    }

    sql_->DragNode(destination_parent->id, node_id);
    NodeModelUtils::UpdatePath(leaf_path_, branch_path_, support_path_, root_, node, separator_);
    NodeModelUtils::UpdateModel(leaf_path_, leaf_model_, support_path_, support_model_, node);

    emit SUpdateName(node_id, node->name, node->type == kTypeBranch);
    emit SResizeColumnToContents(std::to_underlying(NodeEnumS::kName));
    return true;
}
