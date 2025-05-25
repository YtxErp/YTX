#include "nodemodelp.h"

#include "tree/excludesetfiltermodel.h"

NodeModelP::NodeModelP(CNodeModelArg& arg, QObject* parent)
    : NodeModel(arg, parent)
{
    IniModel();
    ConstructTree();
}

NodeModelP::~NodeModelP() { qDeleteAll(node_hash_); }

void NodeModelP::RSyncLeafValue(
    int node_id, double initial_debit_delta, double initial_credit_delta, double final_debit_delta, double final_credit_delta, double /*settled_delta*/)
{
    auto* node { NodeModelUtils::GetNode(node_hash_, node_id) };
    assert(node && node->node_type == kTypeLeaf && "Node must be non-null and of type kTypeLeaf");

    if (initial_credit_delta == 0.0 && initial_debit_delta == 0.0 && final_debit_delta == 0.0 && final_credit_delta == 0.0)
        return;

    bool rule { node->rule };

    double initial_delta { (rule ? 1 : -1) * (initial_credit_delta - initial_debit_delta) };
    double final_delta { (rule ? 1 : -1) * (final_credit_delta - final_debit_delta) };

    node->initial_total += initial_delta;
    node->final_total += final_delta;

    sql_->UpdateLeafValue(node);
    UpdateAncestorValue(node, initial_delta, final_delta);
    emit SSyncStatusValue();
}

void NodeModelP::RemoveUnitSet(int node_id, int unit)
{
    if (unit == std::to_underlying(UnitP::kPos))
        pset_.remove(node_id);
}

void NodeModelP::InsertUnitSet(int node_id, int unit)
{
    if (unit == std::to_underlying(UnitP::kPos)) {
        pset_.insert(node_id);
    }
}

bool NodeModelP::UpdateAncestorValue(Node* node, double initial_delta, double final_delta, double /*first*/, double /*second*/, double /*discount*/)
{
    assert(node && node != root_ && node->parent && "Invalid node: node must be non-null, not the root, and must have a valid parent");

    if (node->parent == root_)
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
    assert(column >= 0 && column < info_.node_header.size() && "Column index out of range");

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
            return (order == Qt::AscendingOrder) ? (lhs->node_type < rhs->node_type) : (lhs->node_type > rhs->node_type);
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
    const bool kIsLeaf { node->node_type == kTypeLeaf };

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
        return node->node_type;
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
        NodeModelUtils::UpdateField(sql_, node, info_.node, kCode, value.toString(), &Node::code);
        break;
    case NodeEnumP::kDescription:
        NodeModelUtils::UpdateField(sql_, node, info_.node, kDescription, value.toString(), &Node::description);
        break;
    case NodeEnumP::kNote:
        NodeModelUtils::UpdateField(sql_, node, info_.node, kNote, value.toString(), &Node::note);
        break;
    case NodeEnumP::kRule:
        UpdateRule(node, value.toBool());
        emit dataChanged(index.siblingAtColumn(std::to_underlying(NodeEnumP::kQuantity)), index.siblingAtColumn(std::to_underlying(NodeEnumP::kAmount)));
        break;
    case NodeEnumP::kType:
        UpdateType(node, value.toInt());
        break;
    case NodeEnumP::kColor:
        NodeModelUtils::UpdateField(sql_, node, info_.node, kColor, value.toString(), &Node::color, true);
        break;
    case NodeEnumP::kUnit:
        UpdateUnit(node, value.toInt());
        break;
    case NodeEnumP::kCommission:
        NodeModelUtils::UpdateField(sql_, node, info_.node, kCommission, value.toDouble(), &Node::second, true);
        break;
    case NodeEnumP::kUnitPrice:
        NodeModelUtils::UpdateField(sql_, node, info_.node, kUnitPrice, value.toDouble(), &Node::first, true);
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

QSortFilterProxyModel* NodeModelP::ExcludeUnitModel(int /*unit*/)
{
    auto* model { new ExcludeSetFilterModel(&pset_, this) };
    model->setSourceModel(leaf_model_);
    QObject::connect(this, &NodeModel::SSyncFilterModel, model, &ExcludeSetFilterModel::RSyncFilterModel);
    return model;
}
