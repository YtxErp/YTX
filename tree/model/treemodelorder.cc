#include "treemodelorder.h"

#include "global/resourcepool.h"

TreeModelOrder::TreeModelOrder(Sqlite* sql, CInfo& info, int default_unit, CTableHash& table_hash, CString& separator, QObject* parent)
    : TreeModel(sql, info, default_unit, table_hash, separator, parent)
    , sql_ { static_cast<SqliteOrder*>(sql) }
{
    ConstructTree();
}

void TreeModelOrder::RUpdateLeafValue(int node_id, double initial_delta, double final_delta, double first_delta, double second_delta, double discount_delta)
{
    auto* node { node_hash_.value(node_id) };
    if (!node || node == root_ || node->type != kTypeLeaf)
        return;

    if (first_delta == 0.0 && second_delta == 0.0 && initial_delta == 0.0 && discount_delta == 0.0 && final_delta == 0.0)
        return;

    sql_->WriteLeafValue(node);

    auto index { GetIndex(node->id) };
    emit dataChanged(index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kFirst)), index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kNetAmount)));

    if (node->finished) {
        UpdateAncestorValue(node, initial_delta, final_delta, first_delta, second_delta, discount_delta);
    }
}

void TreeModelOrder::RUpdateStakeholder(int old_node_id, int new_node_id)
{
    const auto& const_node_hash { std::as_const(node_hash_) };

    for (auto* node : const_node_hash) {
        if (node->party == old_node_id)
            node->party = new_node_id;

        if (node->employee == old_node_id)
            node->employee = new_node_id;
    }
}

void TreeModelOrder::RSyncBool(int node_id, int column, bool value)
{
    if (column != std::to_underlying(TreeEnumOrder::kFinished))
        return;

    auto it { node_hash_.constFind(node_id) };
    if (it == node_hash_.constEnd())
        return;

    auto* node { it.value() };

    int coefficient = value ? 1 : -1;
    UpdateAncestorValue(node, coefficient * node->initial_total, coefficient * node->final_total, coefficient * node->first, coefficient * node->second,
        coefficient * node->discount);

    if (node->unit == std::to_underlying(UnitOrder::kMS))
        emit SSyncDouble(node->party, std::to_underlying(TreeEnumStakeholder::kAmount), coefficient * (node->initial_total - node->discount));
}

void TreeModelOrder::UpdateTree(const QDate& start_date, const QDate& end_date)
{
    beginResetModel();
    root_->children.clear();
    sql_->ReadNode(node_hash_, start_date, end_date);

    const auto& const_node_hash { std::as_const(node_hash_) };

    for (auto* node : const_node_hash) {
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

    for (auto* node : const_node_hash) {
        if (node->type == kTypeLeaf && node->finished)
            UpdateAncestorValue(node, node->initial_total, node->final_total, node->first, node->second, node->discount);
    }
    endResetModel();
}

QString TreeModelOrder::GetPath(int node_id) const
{
    if (auto it = node_hash_.constFind(node_id); it != node_hash_.constEnd())
        return it.value()->name;

    return {};
}

void TreeModelOrder::RetriveNodeOrder(int node_id)
{
    if (node_hash_.contains(node_id))
        return;

    sql_->RetriveNode(node_hash_, node_id);
    auto* node { node_hash_.value(node_id) };
    node->parent = root_;

    auto row { root_->children.size() };

    beginInsertRows(QModelIndex(), row, row);
    root_->children.insert(row, node);
    endInsertRows();
}

Node* TreeModelOrder::GetNodeO(int node_id) const
{
    auto it = node_hash_.constFind(node_id);
    if (it != node_hash_.constEnd())
        return it.value();

    return nullptr;
}

bool TreeModelOrder::UpdateRuleFPTO(Node* node, bool value)
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
    emit dataChanged(index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kFirst)), index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kNetAmount)));

    sql_->WriteLeafValue(node);
    return true;
}

bool TreeModelOrder::UpdateUnit(Node* node, int value)
{
    // Cash = 0, Monthly = 1, Pending = 2

    if (node->unit == value || node->type != kTypeLeaf)
        return false;

    node->unit = value;
    const UnitOrder unit { value };

    switch (unit) {
    case UnitOrder::kIS:
        node->final_total = node->initial_total - node->discount;
        break;
    case UnitOrder::kPEND:
    case UnitOrder::kMS:
        node->final_total = 0.0;
        break;
    default:
        return false;
    }

    sql_->WriteField(info_.node, kUnit, value, node->id);
    sql_->WriteField(info_.node, kNetAmount, node->final_total, node->id);

    emit SResizeColumnToContents(std::to_underlying(TreeEnumOrder::kNetAmount));
    return true;
}

bool TreeModelOrder::UpdateFinished(Node* node, bool value)
{
    if (node->finished == value)
        return false;

    int coefficient = value ? 1 : -1;

    UpdateAncestorValue(node, coefficient * node->initial_total, coefficient * node->final_total, coefficient * node->first, coefficient * node->second,
        coefficient * node->discount);

    node->finished = value;
    emit SSyncBool(node->id, std::to_underlying(TreeEnumOrder::kFinished), value);
    if (node->unit == std::to_underlying(UnitOrder::kMS))
        emit SSyncDouble(node->party, std::to_underlying(TreeEnumStakeholder::kAmount), coefficient * (node->initial_total - node->discount));
    sql_->WriteField(info_.node, kFinished, value, node->id);
    return true;
}

bool TreeModelOrder::UpdateNameFunction(Node* node, CString& value)
{
    node->name = value;
    sql_->WriteField(info_.node, kName, value, node->id);

    emit SResizeColumnToContents(std::to_underlying(TreeEnumOrder::kName));
    emit SSearch();
    return true;
}

void TreeModelOrder::ConstructTree()
{
    sql_->ReadNode(node_hash_, QDate::currentDate(), QDate::currentDate());

    const auto& const_node_hash { std::as_const(node_hash_) };

    for (auto* node : const_node_hash) {
        if (!node->parent) {
            node->parent = root_;
            root_->children.emplace_back(node);
        }
    }

    for (auto* node : const_node_hash)
        if (node->type == kTypeLeaf && node->finished)
            UpdateAncestorValue(node, node->initial_total, node->final_total, node->first, node->second, node->discount);
}

bool TreeModelOrder::UpdateAncestorValue(Node* node, double initial_delta, double final_delta, double first_delta, double second_delta, double discount_delta)
{
    if (!node || node == root_ || !node->parent || node->parent == root_)
        return false;

    if (initial_delta == 0.0 && final_delta == 0.0 && first_delta == 0.0 && second_delta == 0.0 && discount_delta == 0.0)
        return false;

    const int kUnit { node->unit };
    const int kColumnBegin { std::to_underlying(TreeEnumOrder::kFirst) };
    int column_end { std::to_underlying(TreeEnumOrder::kNetAmount) };

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

void TreeModelOrder::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.node_header.size())
        return;

    auto Compare = [column, order](const Node* lhs, const Node* rhs) -> bool {
        const TreeEnumOrder kColumn { column };
        switch (kColumn) {
        case TreeEnumOrder::kName:
            return (order == Qt::AscendingOrder) ? (lhs->name < rhs->name) : (lhs->name > rhs->name);
        case TreeEnumOrder::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case TreeEnumOrder::kRule:
            return (order == Qt::AscendingOrder) ? (lhs->rule < rhs->rule) : (lhs->rule > rhs->rule);
        case TreeEnumOrder::kType:
            return (order == Qt::AscendingOrder) ? (lhs->type < rhs->type) : (lhs->type > rhs->type);
        case TreeEnumOrder::kUnit:
            return (order == Qt::AscendingOrder) ? (lhs->unit < rhs->unit) : (lhs->unit > rhs->unit);
        case TreeEnumOrder::kParty:
            return (order == Qt::AscendingOrder) ? (lhs->party < rhs->party) : (lhs->party > rhs->party);
        case TreeEnumOrder::kEmployee:
            return (order == Qt::AscendingOrder) ? (lhs->employee < rhs->employee) : (lhs->employee > rhs->employee);
        case TreeEnumOrder::kDateTime:
            return (order == Qt::AscendingOrder) ? (lhs->date_time < rhs->date_time) : (lhs->date_time > rhs->date_time);
        case TreeEnumOrder::kFirst:
            return (order == Qt::AscendingOrder) ? (lhs->first < rhs->first) : (lhs->first > rhs->first);
        case TreeEnumOrder::kSecond:
            return (order == Qt::AscendingOrder) ? (lhs->second < rhs->second) : (lhs->second > rhs->second);
        case TreeEnumOrder::kDiscount:
            return (order == Qt::AscendingOrder) ? (lhs->discount < rhs->discount) : (lhs->discount > rhs->discount);
        case TreeEnumOrder::kFinished:
            return (order == Qt::AscendingOrder) ? (lhs->finished < rhs->finished) : (lhs->finished > rhs->finished);
        case TreeEnumOrder::kGrossAmount:
            return (order == Qt::AscendingOrder) ? (lhs->initial_total < rhs->initial_total) : (lhs->initial_total > rhs->initial_total);
        case TreeEnumOrder::kNetAmount:
            return (order == Qt::AscendingOrder) ? (lhs->final_total < rhs->final_total) : (lhs->final_total > rhs->final_total);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    TreeModelUtils::SortIterative(root_, Compare);
    emit layoutChanged();
}

bool TreeModelOrder::InsertNode(int row, const QModelIndex& parent, Node* node)
{
    if (row <= -1)
        return false;

    auto* parent_node { GetNodeByIndex(parent) };

    beginInsertRows(parent, row, row);
    parent_node->children.insert(row, node);
    endInsertRows();

    sql_->WriteNode(parent_node->id, node);
    node_hash_.insert(node->id, node);

    emit SSearch();
    return true;
}

bool TreeModelOrder::RemoveNode(int row, const QModelIndex& parent)
{
    if (row <= -1 || row >= rowCount(parent))
        return false;

    auto* parent_node { GetNodeByIndex(parent) };
    auto* node { parent_node->children.at(row) };

    beginRemoveRows(parent, row, row);
    parent_node->children.removeOne(node);
    endRemoveRows();

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

            if (node->unit == std::to_underlying(UnitOrder::kMS))
                emit SSyncDouble(node->party, std::to_underlying(TreeEnumStakeholder::kAmount), node->discount - node->initial_total);
        }
        break;
    default:
        break;
    }

    emit SSearch();
    emit SResizeColumnToContents(std::to_underlying(TreeEnumOrder::kName));

    ResourcePool<Node>::Instance().Recycle(node);
    node_hash_.remove(node->id);

    return true;
}

QVariant TreeModelOrder::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* node { GetNodeByIndex(index) };
    if (node == root_)
        return QVariant();

    const TreeEnumOrder kColumn { index.column() };
    bool branch { node->type == kTypeBranch };

    switch (kColumn) {
    case TreeEnumOrder::kName:
        return node->name;
    case TreeEnumOrder::kID:
        return node->id;
    case TreeEnumOrder::kDescription:
        return node->description;
    case TreeEnumOrder::kRule:
        return branch ? -1 : node->rule;
    case TreeEnumOrder::kType:
        return branch ? node->type : QVariant();
    case TreeEnumOrder::kUnit:
        return node->unit;
    case TreeEnumOrder::kParty:
        return node->party == 0 ? QVariant() : node->party;
    case TreeEnumOrder::kEmployee:
        return node->employee == 0 ? QVariant() : node->employee;
    case TreeEnumOrder::kDateTime:
        return branch || node->date_time.isEmpty() ? QVariant() : node->date_time;
    case TreeEnumOrder::kFirst:
        return node->first == 0 ? QVariant() : node->first;
    case TreeEnumOrder::kSecond:
        return node->second == 0 ? QVariant() : node->second;
    case TreeEnumOrder::kDiscount:
        return node->discount == 0 ? QVariant() : node->discount;
    case TreeEnumOrder::kFinished:
        return !branch && node->finished ? node->finished : QVariant();
    case TreeEnumOrder::kGrossAmount:
        return node->initial_total;
    case TreeEnumOrder::kNetAmount:
        return node->final_total;
    default:
        return QVariant();
    }
}

bool TreeModelOrder::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    auto* node { GetNodeByIndex(index) };
    if (node == root_)
        return false;

    const TreeEnumOrder kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumOrder::kDescription:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kDescription, &Node::description);
        emit SSyncString(node->id, index.column(), value.toString());
        break;
    case TreeEnumOrder::kRule:
        UpdateRuleFPTO(node, value.toBool());
        emit SSyncBool(node->id, index.column(), value.toBool());
        break;
    case TreeEnumOrder::kUnit:
        UpdateUnit(node, value.toInt());
        emit SSyncInt(node->id, index.column(), value.toInt());
        break;
    case TreeEnumOrder::kParty:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toInt(), kParty, &Node::party);
        break;
    case TreeEnumOrder::kEmployee:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toInt(), kEmployee, &Node::employee);
        emit SSyncInt(node->id, index.column(), value.toInt());
        break;
    case TreeEnumOrder::kDateTime:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kDateTime, &Node::date_time);
        emit SSyncString(node->id, index.column(), value.toString());
        break;
    case TreeEnumOrder::kFinished:
        UpdateFinished(node, value.toBool());
        break;
    default:
        return false;
    }

    emit SResizeColumnToContents(index.column());
    return true;
}

Qt::ItemFlags TreeModelOrder::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };

    const TreeEnumOrder kColumn { index.column() };
    switch (kColumn) {
    case TreeEnumOrder::kName:
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        break;
    case TreeEnumOrder::kDescription:
    case TreeEnumOrder::kUnit:
    case TreeEnumOrder::kDateTime:
    case TreeEnumOrder::kRule:
    case TreeEnumOrder::kEmployee:
        flags |= Qt::ItemIsEditable;
        break;
    default:
        break;
    }

    const bool non_editable { index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kType)).data().toBool()
        || index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kFinished)).data().toBool() };

    if (non_editable)
        flags &= ~Qt::ItemIsEditable;

    return flags;
}

bool TreeModelOrder::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
        return false;

    auto* destination_parent { GetNodeByIndex(parent) };
    if (destination_parent->type != kTypeBranch)
        return false;

    int node_id {};

    if (auto mime { data->data(kNodeID) }; !mime.isEmpty())
        node_id = QVariant(mime).toInt();

    auto* node { TreeModelUtils::GetNodeByID(node_hash_, node_id) };
    if (!node || node->parent == destination_parent || TreeModelUtils::IsDescendant(destination_parent, node))
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
    emit SResizeColumnToContents(std::to_underlying(TreeEnumOrder::kName));

    return true;
}
