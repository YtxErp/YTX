#include "nodemodelt.h"

NodeModelT::NodeModelT(CNodeModelArg& arg, QObject* parent)
    : NodeModel(arg, parent)
{
    IniModel();
    ConstructTree();
}

NodeModelT::~NodeModelT() { qDeleteAll(node_hash_); }

void NodeModelT::RSyncLeafValue(
    int node_id, double initial_debit_delta, double initial_credit_delta, double final_debit_delta, double final_credit_delta, double /*settled*/)
{
    auto* node { NodeModelUtils::GetNode(node_hash_, node_id) };
    assert(node && node->node_type == kTypeLeaf && "Node must be non-null and of type kTypeLeaf");

    if (initial_credit_delta == 0.0 && initial_debit_delta == 0.0 && final_debit_delta == 0.0 && final_credit_delta == 0.0)
        return;

    bool direction_rule { node->direction_rule };

    double initial_delta { (direction_rule ? 1 : -1) * (initial_credit_delta - initial_debit_delta) };
    double final_delta { (direction_rule ? 1 : -1) * (final_credit_delta - final_debit_delta) };

    node->initial_total += initial_delta;
    node->final_total += final_delta;

    sql_->UpdateLeafValue(node);
    UpdateAncestorValue(node, initial_delta, final_delta);

    emit SSyncStatusValue();
}

void NodeModelT::RSyncDouble(int node_id, int column, double value)
{
    assert(node_id >= 1 && "node_id must be positive");

    if (column != std::to_underlying(NodeEnumT::kUnitCost) || value == 0.0)
        return;

    auto* node { node_hash_.value(node_id) };
    assert(node && node->node_type == kTypeLeaf && "Node must be non-null and of type kTypeLeaf");

    if (node->unit != std::to_underlying(UnitT::kProd))
        return;

    node->first += value;
    sql_->WriteField(info_.node, kUnitCost, node->first, node_id);
}

QVariant NodeModelT::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* node { GetNodeByIndex(index) };
    if (node == root_)
        return QVariant();

    const NodeEnumT kColumn { index.column() };
    const bool kIsLeaf { node->node_type == kTypeLeaf };

    switch (kColumn) {
    case NodeEnumT::kName:
        return node->name;
    case NodeEnumT::kID:
        return node->id;
    case NodeEnumT::kCode:
        return node->code;
    case NodeEnumT::kDescription:
        return node->description;
    case NodeEnumT::kNote:
        return node->note;
    case NodeEnumT::kRule:
        return node->direction_rule;
    case NodeEnumT::kType:
        return node->node_type;
    case NodeEnumT::kUnit:
        return node->unit;
    case NodeEnumT::kColor:
        return node->color;
    case NodeEnumT::kDateTime:
        return kIsLeaf ? node->date_time : QVariant();
    case NodeEnumT::kFinished:
        return kIsLeaf && node->finished ? node->finished : QVariant();
    case NodeEnumT::kUnitCost:
        return kIsLeaf && node->first != 0 ? node->first : QVariant();
    case NodeEnumT::kDocument:
        return node->document.isEmpty() ? QVariant() : node->document.size();
    case NodeEnumT::kQuantity:
        return node->initial_total;
    case NodeEnumT::kAmount:
        return node->final_total;
    default:
        return QVariant();
    }
}

bool NodeModelT::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    auto* node { GetNodeByIndex(index) };
    if (node == root_)
        return false;

    const NodeEnumT kColumn { index.column() };

    switch (kColumn) {
    case NodeEnumT::kCode:
        NodeModelUtils::UpdateField(sql_, node, info_.node, kCode, value.toString(), &Node::code);
        break;
    case NodeEnumT::kDescription:
        NodeModelUtils::UpdateField(sql_, node, info_.node, kDescription, value.toString(), &Node::description);
        break;
    case NodeEnumT::kNote:
        NodeModelUtils::UpdateField(sql_, node, info_.node, kNote, value.toString(), &Node::note);
        break;
    case NodeEnumT::kRule:
        UpdateRule(node, value.toBool());
        emit dataChanged(index.siblingAtColumn(std::to_underlying(NodeEnumT::kQuantity)), index.siblingAtColumn(std::to_underlying(NodeEnumT::kAmount)));
        break;
    case NodeEnumT::kType:
        UpdateType(node, value.toInt());
        break;
    case NodeEnumT::kColor:
        NodeModelUtils::UpdateField(sql_, node, info_.node, kColor, value.toString(), &Node::color, true);
        break;
    case NodeEnumT::kDateTime:
        NodeModelUtils::UpdateField(sql_, node, info_.node, kDateTime, value.toString(), &Node::date_time, true);
        break;
    case NodeEnumT::kUnit:
        UpdateUnit(node, value.toInt());
        break;
    case NodeEnumT::kFinished:
        NodeModelUtils::UpdateField(sql_, node, info_.node, kFinished, value.toBool(), &Node::finished, true);
        break;
    default:
        return false;
    }

    emit SResizeColumnToContents(index.column());
    return true;
}

void NodeModelT::sort(int column, Qt::SortOrder order)
{
    assert(column >= 0 && column < info_.node_header.size() && "Column index out of range");

    auto Compare = [column, order](const Node* lhs, const Node* rhs) -> bool {
        const NodeEnumT kColumn { column };
        switch (kColumn) {
        case NodeEnumT::kName:
            return (order == Qt::AscendingOrder) ? (lhs->name < rhs->name) : (lhs->name > rhs->name);
        case NodeEnumT::kCode:
            return (order == Qt::AscendingOrder) ? (lhs->code < rhs->code) : (lhs->code > rhs->code);
        case NodeEnumT::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case NodeEnumT::kNote:
            return (order == Qt::AscendingOrder) ? (lhs->note < rhs->note) : (lhs->note > rhs->note);
        case NodeEnumT::kRule:
            return (order == Qt::AscendingOrder) ? (lhs->direction_rule < rhs->direction_rule) : (lhs->direction_rule > rhs->direction_rule);
        case NodeEnumT::kType:
            return (order == Qt::AscendingOrder) ? (lhs->node_type < rhs->node_type) : (lhs->node_type > rhs->node_type);
        case NodeEnumT::kFinished:
            return (order == Qt::AscendingOrder) ? (lhs->finished < rhs->finished) : (lhs->finished > rhs->finished);
        case NodeEnumT::kUnit:
            return (order == Qt::AscendingOrder) ? (lhs->unit < rhs->unit) : (lhs->unit > rhs->unit);
        case NodeEnumT::kColor:
            return (order == Qt::AscendingOrder) ? (lhs->color < rhs->color) : (lhs->color > rhs->color);
        case NodeEnumT::kDocument:
            return (order == Qt::AscendingOrder) ? (lhs->document.size() < rhs->document.size()) : (lhs->document.size() > rhs->document.size());
        case NodeEnumT::kDateTime:
            return (order == Qt::AscendingOrder) ? (lhs->date_time < rhs->date_time) : (lhs->date_time > rhs->date_time);
        case NodeEnumT::kUnitCost:
            return (order == Qt::AscendingOrder) ? (lhs->first < rhs->first) : (lhs->first > rhs->first);
        case NodeEnumT::kQuantity:
            return (order == Qt::AscendingOrder) ? (lhs->initial_total < rhs->initial_total) : (lhs->initial_total > rhs->initial_total);
        case NodeEnumT::kAmount:
            return (order == Qt::AscendingOrder) ? (lhs->final_total < rhs->final_total) : (lhs->final_total > rhs->final_total);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    NodeModelUtils::SortIterative(root_, Compare);
    emit layoutChanged();
}

Qt::ItemFlags NodeModelT::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };

    const NodeEnumT kColumn { index.column() };
    switch (kColumn) {
    case NodeEnumT::kName:
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        break;
    case NodeEnumT::kDescription:
    case NodeEnumT::kCode:
    case NodeEnumT::kNote:
    case NodeEnumT::kType:
    case NodeEnumT::kRule:
    case NodeEnumT::kUnit:
    case NodeEnumT::kDateTime:
        flags |= Qt::ItemIsEditable;
        break;
    default:
        break;
    }

    const bool finished { index.siblingAtColumn(std::to_underlying(NodeEnumT::kFinished)).data().toBool() };
    if (finished)
        flags &= ~Qt::ItemIsEditable;

    return flags;
}

bool NodeModelT::UpdateUnit(Node* node, int value)
{
    if (node->unit == value)
        return false;

    const int node_id { node->id };
    QString message { tr("Cannot change %1 unit,").arg(Path(node_id)) };

    if (NodeModelUtils::HasChildren(node, message))
        return false;

    if (NodeModelUtils::IsInternalReferenced(sql_, node_id, message))
        return false;

    if (NodeModelUtils::IsSupportReferenced(sql_, node_id, message))
        return false;

    node->unit = value;
    sql_->WriteField(info_.node, kUnit, value, node_id);

    return true;
}

bool NodeModelT::UpdateAncestorValue(Node* node, double initial_delta, double final_delta, double /*first*/, double /*second*/, double /*discount*/)
{
    assert(node && node != root_ && node->parent && "Invalid node: node must be non-null, not the root, and must have a valid parent");

    if (node->parent == root_)
        return false;

    if (initial_delta == 0.0 && final_delta == 0.0)
        return false;

    const bool direction_rule { node->direction_rule };

    for (Node* current = node->parent; current && current != root_; current = current->parent) {
        bool equal { current->direction_rule == direction_rule };

        current->final_total += (equal ? 1 : -1) * final_delta;
        current->initial_total += (equal ? 1 : -1) * initial_delta;
    }

    return true;
}
