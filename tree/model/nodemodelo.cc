#include "nodemodelo.h"

NodeModelO::NodeModelO(CNodeModelArg& arg, QObject* parent)
    : NodeModel(arg, parent)
    , sql_ { static_cast<SqliteO*>(arg.sql) }
{
    ConstructTree();
}

void NodeModelO::RUpdateLeafValue(int node_id, double initial_delta, double final_delta, double first_delta, double second_delta, double discount_delta)
{
    auto* node { node_hash_.value(node_id) };
    assert(node && node->type == kTypeLeaf && "Node must be non-null and of type kTypeLeaf");

    if (first_delta == 0.0 && second_delta == 0.0 && initial_delta == 0.0 && discount_delta == 0.0 && final_delta == 0.0)
        return;

    sql_->SyncLeafValue(node);

    auto index { GetIndex(node->id) };
    emit dataChanged(index.siblingAtColumn(std::to_underlying(NodeEnumO::kFirst)), index.siblingAtColumn(std::to_underlying(NodeEnumO::kSettlement)));

    if (node->finished) {
        UpdateAncestorValue(node, initial_delta, final_delta, first_delta, second_delta, discount_delta);
    }
}

void NodeModelO::RSyncBoolWD(int node_id, int column, bool value)
{
    if (column != std::to_underlying(NodeEnumO::kFinished))
        return;

    auto* node { node_hash_.value(node_id) };
    assert(node && "Node must be non-null");

    int coefficient = value ? 1 : -1;
    UpdateAncestorValue(node, coefficient * node->initial_total, coefficient * node->final_total, coefficient * node->first, coefficient * node->second,
        coefficient * node->discount);

    if (node->unit == std::to_underlying(UnitO::kMS))
        emit SSyncDouble(node->party, std::to_underlying(NodeEnumS::kAmount), coefficient * (node->initial_total - node->discount));
}

void NodeModelO::UpdateTree(const QDateTime& start, const QDateTime& end)
{
    beginResetModel();
    root_->children.clear();
    sql_->ReadNode(node_hash_, start, end);

    for (auto* node : std::as_const(node_hash_)) {
        if (!node->parent) {
            node->parent = root_;
            root_->children.emplace_back(node);
        }

        if (node->type == kTypeBranch) {
            node->first = 0.0;
            node->second = 0.0;
            node->initial_total = 0.0;
            node->discount = 0.0;
            node->final_total = 0.0;
        }
    }

    for (auto* node : std::as_const(node_hash_)) {
        if (node->type == kTypeLeaf && node->finished)
            UpdateAncestorValue(node, node->initial_total, node->final_total, node->first, node->second, node->discount);
    }
    endResetModel();
}

QString NodeModelO::Path(int node_id) const
{
    if (auto it = node_hash_.constFind(node_id); it != node_hash_.constEnd())
        return it.value()->name;

    return {};
}

void NodeModelO::ReadNode(int node_id)
{
    assert(node_id >= 1 && "node_id must be positive");

    if (node_hash_.contains(node_id))
        return;

    auto* node { sql_->ReadNode(node_id) };
    assert(node && "Error: node must not be nullptr!");

    node->parent = root_;
    node_hash_.insert(node_id, node);

    auto row { root_->children.size() };

    beginInsertRows(QModelIndex(), row, row);
    root_->children.insert(row, node);
    endInsertRows();
}

Node* NodeModelO::GetNode(int node_id) const { return NodeModelUtils::GetNode(node_hash_, node_id); }

bool NodeModelO::UpdateRule(Node* node, bool value)
{
    if (node->rule == value || node->type != kTypeLeaf)
        return false;

    node->rule = value;
    sql_->WriteField(info_.node, kRule, value, node->id);

    node->first = -node->first;
    node->second = -node->second;
    node->discount = -node->discount;
    node->initial_total = -node->initial_total;
    node->final_total = -node->final_total;

    auto index { GetIndex(node->id) };
    emit dataChanged(index.siblingAtColumn(std::to_underlying(NodeEnumO::kFirst)), index.siblingAtColumn(std::to_underlying(NodeEnumO::kSettlement)));

    sql_->SyncLeafValue(node);
    sql_->InvertTransValue(node->id);

    return true;
}

bool NodeModelO::UpdateUnit(Node* node, int value)
{
    // Cash = 0, Monthly = 1, Pending = 2

    if (node->unit == value || node->type != kTypeLeaf)
        return false;

    node->unit = value;
    const UnitO unit { value };

    switch (unit) {
    case UnitO::kIS:
        node->final_total = node->initial_total - node->discount;
        break;
    case UnitO::kPEND:
    case UnitO::kMS:
        node->final_total = 0.0;
        break;
    default:
        return false;
    }

    sql_->WriteField(info_.node, kUnit, value, node->id);
    sql_->WriteField(info_.node, kSettlement, node->final_total, node->id);

    emit SResizeColumnToContents(std::to_underlying(NodeEnumO::kSettlement));
    return true;
}

bool NodeModelO::UpdateFinished(Node* node, bool value)
{
    if (node->finished == value || node->unit == std::to_underlying(UnitO::kPEND) || node->type != kTypeLeaf)
        return false;

    int coefficient = value ? 1 : -1;

    UpdateAncestorValue(node, coefficient * node->initial_total, coefficient * node->final_total, coefficient * node->first, coefficient * node->second,
        coefficient * node->discount);

    node->finished = value;
    emit SSyncBoolWD(node->id, std::to_underlying(NodeEnumO::kFinished), value);
    if (node->unit == std::to_underlying(UnitO::kMS))
        emit SSyncDouble(node->party, std::to_underlying(NodeEnumS::kAmount), coefficient * (node->initial_total - node->discount));

    sql_->WriteField(info_.node, kFinished, value, node->id);
    if (value)
        sql_->SyncPriceS(node->id);
    return true;
}

bool NodeModelO::UpdateNameFunction(Node* node, CString& value)
{
    node->name = value;
    sql_->WriteField(info_.node, kName, value, node->id);

    emit SResizeColumnToContents(std::to_underlying(NodeEnumO::kName));
    emit SSearch();
    return true;
}

void NodeModelO::ConstructTree()
{
    const QDate kCurrentDate { QDate::currentDate() };
    sql_->ReadNode(node_hash_, QDateTime(kCurrentDate, kStartTime), QDateTime(kCurrentDate, kEndTime));

    for (auto* node : std::as_const(node_hash_)) {
        if (!node->parent) {
            node->parent = root_;
            root_->children.emplace_back(node);
        }
    }

    for (auto* node : std::as_const(node_hash_))
        if (node->type == kTypeLeaf && node->finished)
            UpdateAncestorValue(node, node->initial_total, node->final_total, node->first, node->second, node->discount);
}

void NodeModelO::RemovePath(Node* node, Node* parent_node)
{
    switch (node->type) {
    case kTypeBranch:
        for (auto* child : std::as_const(node->children)) {
            child->parent = parent_node;
            parent_node->children.emplace_back(child);
        }
        break;
    case kTypeLeaf:
        if (node->finished) {
            UpdateAncestorValue(node, -node->initial_total, -node->final_total, -node->first, -node->second, -node->discount);

            if (node->unit == std::to_underlying(UnitO::kMS))
                emit SSyncDouble(node->party, std::to_underlying(NodeEnumS::kAmount), node->discount - node->initial_total);
        }
        break;
    default:
        break;
    }
}

bool NodeModelO::UpdateAncestorValue(Node* node, double initial_delta, double final_delta, double first_delta, double second_delta, double discount_delta)
{
    assert(node && node != root_ && node->parent && "Invalid node: node must be non-null, not the root, and must have a valid parent");

    if (node->parent == root_)
        return false;

    if (initial_delta == 0.0 && final_delta == 0.0 && first_delta == 0.0 && second_delta == 0.0 && discount_delta == 0.0)
        return false;

    const int kUnit { node->unit };
    const int kColumnBegin { std::to_underlying(NodeEnumO::kFirst) };
    int column_end { std::to_underlying(NodeEnumO::kSettlement) };

    // 确定需要更新的列范围
    if (initial_delta == 0.0 && final_delta == 0.0 && second_delta == 0.0 && discount_delta == 0.0)
        column_end = kColumnBegin;

    QModelIndexList ancestor {};
    for (node = node->parent; node && node != root_; node = node->parent) {
        if (node->unit != kUnit)
            continue;

        node->first += first_delta;
        node->second += second_delta;
        node->discount += discount_delta;
        node->initial_total += initial_delta;
        node->final_total += final_delta;

        ancestor.emplaceBack(GetIndex(node->id));
    }

    if (!ancestor.isEmpty())
        emit dataChanged(index(ancestor.first().row(), kColumnBegin), index(ancestor.last().row(), column_end), { Qt::DisplayRole });

    return true;
}

void NodeModelO::sort(int column, Qt::SortOrder order)
{
    assert(column >= 0 && column < info_.node_header.size() && "Column index out of range");

    auto Compare = [column, order](const Node* lhs, const Node* rhs) -> bool {
        const NodeEnumO kColumn { column };
        switch (kColumn) {
        case NodeEnumO::kName:
            return (order == Qt::AscendingOrder) ? (lhs->name < rhs->name) : (lhs->name > rhs->name);
        case NodeEnumO::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case NodeEnumO::kRule:
            return (order == Qt::AscendingOrder) ? (lhs->rule < rhs->rule) : (lhs->rule > rhs->rule);
        case NodeEnumO::kType:
            return (order == Qt::AscendingOrder) ? (lhs->type < rhs->type) : (lhs->type > rhs->type);
        case NodeEnumO::kUnit:
            return (order == Qt::AscendingOrder) ? (lhs->unit < rhs->unit) : (lhs->unit > rhs->unit);
        case NodeEnumO::kParty:
            return (order == Qt::AscendingOrder) ? (lhs->party < rhs->party) : (lhs->party > rhs->party);
        case NodeEnumO::kEmployee:
            return (order == Qt::AscendingOrder) ? (lhs->employee < rhs->employee) : (lhs->employee > rhs->employee);
        case NodeEnumO::kDateTime:
            return (order == Qt::AscendingOrder) ? (lhs->date_time < rhs->date_time) : (lhs->date_time > rhs->date_time);
        case NodeEnumO::kFirst:
            return (order == Qt::AscendingOrder) ? (lhs->first < rhs->first) : (lhs->first > rhs->first);
        case NodeEnumO::kSecond:
            return (order == Qt::AscendingOrder) ? (lhs->second < rhs->second) : (lhs->second > rhs->second);
        case NodeEnumO::kDiscount:
            return (order == Qt::AscendingOrder) ? (lhs->discount < rhs->discount) : (lhs->discount > rhs->discount);
        case NodeEnumO::kFinished:
            return (order == Qt::AscendingOrder) ? (lhs->finished < rhs->finished) : (lhs->finished > rhs->finished);
        case NodeEnumO::kGrossAmount:
            return (order == Qt::AscendingOrder) ? (lhs->initial_total < rhs->initial_total) : (lhs->initial_total > rhs->initial_total);
        case NodeEnumO::kSettlement:
            return (order == Qt::AscendingOrder) ? (lhs->final_total < rhs->final_total) : (lhs->final_total > rhs->final_total);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    NodeModelUtils::SortIterative(root_, Compare);
    emit layoutChanged();
}

QVariant NodeModelO::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* node { GetNodeByIndex(index) };
    if (node == root_)
        return QVariant();

    const NodeEnumO kColumn { index.column() };
    bool branch { node->type == kTypeBranch };

    switch (kColumn) {
    case NodeEnumO::kName:
        return node->name;
    case NodeEnumO::kID:
        return node->id;
    case NodeEnumO::kDescription:
        return node->description;
    case NodeEnumO::kRule:
        return branch ? -1 : node->rule;
    case NodeEnumO::kType:
        return branch ? node->type : QVariant();
    case NodeEnumO::kUnit:
        return node->unit;
    case NodeEnumO::kParty:
        return node->party == 0 ? QVariant() : node->party;
    case NodeEnumO::kEmployee:
        return node->employee == 0 ? QVariant() : node->employee;
    case NodeEnumO::kDateTime:
        return branch || node->date_time.isEmpty() ? QVariant() : node->date_time;
    case NodeEnumO::kFirst:
        return node->first == 0 ? QVariant() : node->first;
    case NodeEnumO::kSecond:
        return node->second == 0 ? QVariant() : node->second;
    case NodeEnumO::kDiscount:
        return node->discount == 0 ? QVariant() : node->discount;
    case NodeEnumO::kFinished:
        return !branch && node->finished ? node->finished : QVariant();
    case NodeEnumO::kGrossAmount:
        return node->initial_total;
    case NodeEnumO::kSettlement:
        return node->final_total;
    default:
        return QVariant();
    }
}

bool NodeModelO::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    auto* node { GetNodeByIndex(index) };
    if (node == root_)
        return false;

    const NodeEnumO kColumn { index.column() };

    switch (kColumn) {
    case NodeEnumO::kDescription:
        NodeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kDescription, &Node::description);
        emit SSyncString(node->id, index.column(), value.toString());
        break;
    case NodeEnumO::kRule:
        UpdateRule(node, value.toBool());
        emit SSyncBoolWD(node->id, index.column(), value.toBool());
        break;
    case NodeEnumO::kUnit:
        UpdateUnit(node, value.toInt());
        emit SSyncInt(node->id, index.column(), value.toInt());
        break;
    case NodeEnumO::kParty:
        NodeModelUtils::UpdateField(sql_, node, info_.node, value.toInt(), kParty, &Node::party);
        break;
    case NodeEnumO::kEmployee:
        NodeModelUtils::UpdateField(sql_, node, info_.node, value.toInt(), kEmployee, &Node::employee);
        emit SSyncInt(node->id, index.column(), value.toInt());
        break;
    case NodeEnumO::kDateTime:
        NodeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kDateTime, &Node::date_time);
        emit SSyncString(node->id, index.column(), value.toString());
        break;
    case NodeEnumO::kFinished:
        UpdateFinished(node, value.toBool());
        break;
    default:
        return false;
    }

    emit SResizeColumnToContents(index.column());
    return true;
}

Qt::ItemFlags NodeModelO::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };

    const NodeEnumO kColumn { index.column() };
    switch (kColumn) {
    case NodeEnumO::kName:
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        break;
    case NodeEnumO::kDescription:
    case NodeEnumO::kUnit:
    case NodeEnumO::kDateTime:
    case NodeEnumO::kRule:
    case NodeEnumO::kEmployee:
        flags |= Qt::ItemIsEditable;
        break;
    default:
        break;
    }

    const bool non_editable { index.siblingAtColumn(std::to_underlying(NodeEnumO::kType)).data().toBool()
        || index.siblingAtColumn(std::to_underlying(NodeEnumO::kFinished)).data().toBool() };

    if (non_editable)
        flags &= ~Qt::ItemIsEditable;

    return flags;
}

bool NodeModelO::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
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
    bool update_ancestor { node->type == kTypeBranch || node->finished };

    if (beginMoveRows(source_index.parent(), source_row, source_row, parent, begin_row)) {
        node->parent->children.removeAt(source_row);
        if (update_ancestor) {
            UpdateAncestorValue(node, -node->initial_total, -node->final_total, -node->first, -node->second, -node->discount);
        }

        destination_parent->children.insert(begin_row, node);
        node->parent = destination_parent;

        if (update_ancestor) {
            UpdateAncestorValue(node, node->initial_total, node->final_total, node->first, node->second, node->discount);
        }

        endMoveRows();
    }

    sql_->DragNode(destination_parent->id, node_id);
    emit SResizeColumnToContents(std::to_underlying(NodeEnumO::kName));

    return true;
}
