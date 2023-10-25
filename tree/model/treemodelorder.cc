#include "treemodelorder.h"

#include "global/resourcepool.h"

TreeModelOrder::TreeModelOrder(Sqlite* sql, CInfo& info, int default_unit, CTableHash& table_hash, CString& separator, QObject* parent)
    : TreeModel(sql, info, default_unit, table_hash, separator, parent)
    , sql_ { static_cast<SqliteOrder*>(sql) }
{
    ConstructTree();
}

void TreeModelOrder::RUpdateLeafValueOne(int node_id, double diff, CString& node_field)
{
    auto* node { node_hash_.value(node_id) };
    if (!node || node == root_ || node->type != kTypeLeaf || diff == 0.0)
        return;

    node->first += diff;

    sql_->UpdateField(info_.node, node->first, node_field, node_id);

    const int column { std::to_underlying(TreeEnumOrder::kFirst) };

    auto index { GetIndex(node_id) };
    emit dataChanged(index.siblingAtColumn(column), index.siblingAtColumn(column));

    if (node->finished)
        UpdateAncestorValueOrder(node, diff);
}

void TreeModelOrder::RUpdateLeafValue(int node_id, double first_diff, double second_diff, double amount_diff, double discount_diff, double settled_diff)
{
    auto* node { node_hash_.value(node_id) };
    if (!node || node == root_ || node->type != kTypeLeaf)
        return;

    if (first_diff == 0 && second_diff == 0 && amount_diff == 0 && discount_diff == 0 && settled_diff == 0)
        return;

    double settled { node->unit == kUnitIM ? settled_diff : 0.0 };

    node->first += first_diff;
    node->second += second_diff;
    node->discount += discount_diff;
    node->initial_total += amount_diff;
    node->final_total += settled;

    sql_->UpdateNodeValue(node);

    auto index { GetIndex(node->id) };
    emit dataChanged(index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kFirst)), index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kSettled)));

    if (node->finished)
        UpdateAncestorValueOrder(node, first_diff, second_diff, amount_diff, discount_diff, settled);
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

void TreeModelOrder::RUpdateFinished(int node_id, bool checked)
{
    auto it { node_hash_.constFind(node_id) };
    if (it == node_hash_.constEnd())
        return;

    auto* node { it.value() };

    int coefficient = checked ? 1 : -1;
    UpdateAncestorValueOrder(node, coefficient * node->first, coefficient * node->second, coefficient * node->initial_total, coefficient * node->discount,
        coefficient * node->final_total);
}

void TreeModelOrder::UpdateAncestorValueOrder(Node* node, double first_diff, double second_diff, double amount_diff, double discount_diff, double settled_diff)
{
    if (!node || node == root_ || node->parent == root_ || !node->parent)
        return;

    if (first_diff == 0 && second_diff == 0 && amount_diff == 0 && discount_diff == 0 && settled_diff == 0)
        return;

    const int unit { node->unit };
    const int column_begin { std::to_underlying(TreeEnumOrder::kFirst) };
    int column_end { std::to_underlying(TreeEnumOrder::kSettled) };

    // 确定需要更新的列范围
    if (second_diff == 0.0 && amount_diff == 0.0 && discount_diff == 0.0 && settled_diff == 0.0)
        column_end = column_begin;

    QModelIndexList ancestor {};
    for (node = node->parent; node && node != root_; node = node->parent) {
        if (node->unit != unit)
            continue;

        node->first += first_diff;
        node->second += second_diff;
        node->discount += discount_diff;
        node->initial_total += amount_diff;
        node->final_total += settled_diff;

        ancestor.emplaceBack(GetIndex(node->id));
    }

    if (!ancestor.isEmpty())
        emit dataChanged(index(ancestor.first().row(), column_begin), index(ancestor.last().row(), column_end), { Qt::DisplayRole });
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
            UpdateAncestorValueOrder(node, node->first, node->second, node->initial_total, node->discount, node->final_total);
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

bool TreeModelOrder::UpdateRuleFPTO(Node* node, bool value)
{
    if (node->rule == value || node->type != kTypeLeaf)
        return false;

    node->rule = value;
    sql_->UpdateField(info_.node, value, kRule, node->id);

    node->first = -node->first;
    node->second = -node->second;
    node->discount = -node->discount;
    node->initial_total = -node->initial_total;
    node->final_total = -node->final_total;

    auto index { GetIndex(node->id) };
    emit dataChanged(index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kFirst)), index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kSettled)));

    sql_->UpdateField(info_.node, node->first, kFirst, node->id);
    sql_->UpdateField(info_.node, node->second, kSecond, node->id);
    sql_->UpdateField(info_.node, node->discount, kDiscount, node->id);
    sql_->UpdateField(info_.node, node->initial_total, kAmount, node->id);
    sql_->UpdateField(info_.node, node->final_total, kSettled, node->id);

    return true;
}

bool TreeModelOrder::UpdateUnit(Node* node, int value)
{
    // Cash = 0, Monthly = 1, Pending = 2

    if (node->unit == value || node->type != kTypeLeaf)
        return false;

    node->unit = value;

    switch (value) {
    case kUnitIM:
        node->final_total = node->initial_total - node->discount;
        break;
    case kUnitPEND:
    case kUnitMS:
        node->final_total = 0.0;
        break;
    default:
        return false;
    }

    sql_->UpdateField(info_.node, value, kUnit, node->id);
    sql_->UpdateField(info_.node, node->final_total, kAmount, node->id);

    emit SResizeColumnToContents(std::to_underlying(TreeEnumOrder::kSettled));
    return true;
}

bool TreeModelOrder::UpdateFinished(Node* node, bool value)
{
    if (node->finished == value)
        return false;

    int coefficient = value ? 1 : -1;

    UpdateAncestorValueOrder(node, coefficient * node->first, coefficient * node->second, coefficient * node->initial_total, coefficient * node->discount,
        coefficient * node->final_total);

    node->finished = value;
    emit SUpdateData(node->id, TreeEnumOrder::kFinished, value);
    sql_->UpdateField(info_.node, value, kFinished, node->id);
    return true;
}

bool TreeModelOrder::UpdateName(Node* node, CString& value)
{
    node->name = value;
    sql_->UpdateField(info_.node, value, kName, node->id);

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
            UpdateAncestorValueOrder(node, node->first, node->second, node->initial_total, node->discount, node->final_total);
}

void TreeModelOrder::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.tree_header.size())
        return;

    auto Compare = [column, order](const Node* lhs, const Node* rhs) -> bool {
        const TreeEnumOrder kColumn { column };
        switch (kColumn) {
        case TreeEnumOrder::kName:
            return (order == Qt::AscendingOrder) ? (lhs->name < rhs->name) : (lhs->name > rhs->name);
        case TreeEnumOrder::kCode:
            return (order == Qt::AscendingOrder) ? (lhs->code < rhs->code) : (lhs->code > rhs->code);
        case TreeEnumOrder::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case TreeEnumOrder::kNote:
            return (order == Qt::AscendingOrder) ? (lhs->note < rhs->note) : (lhs->note > rhs->note);
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
        case TreeEnumOrder::kAmount:
            return (order == Qt::AscendingOrder) ? (lhs->initial_total < rhs->initial_total) : (lhs->initial_total > rhs->initial_total);
        case TreeEnumOrder::kSettled:
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

    if (node->type == kTypeLeaf && node->finished)
        UpdateAncestorValueOrder(node, node->first, node->second, node->initial_total, node->discount, node->final_total);

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
        for (auto* child : node->children) {
            child->parent = parent_node;
            parent_node->children.emplace_back(child);
        }
        break;
    case kTypeLeaf:
        if (node->finished) {
            UpdateAncestorValueOrder(node, -node->first, -node->second, -node->initial_total, -node->discount, -node->final_total);
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
    case TreeEnumOrder::kCode:
        return node->code;
    case TreeEnumOrder::kDescription:
        return node->description;
    case TreeEnumOrder::kNote:
        return node->note;
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
    case TreeEnumOrder::kAmount:
        return node->initial_total;
    case TreeEnumOrder::kSettled:
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
    case TreeEnumOrder::kCode:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kCode, &Node::code);
        break;
    case TreeEnumOrder::kDescription:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kDescription, &Node::description);
        break;
    case TreeEnumOrder::kNote:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kNote, &Node::note);
        break;
    case TreeEnumOrder::kRule:
        UpdateRuleFPTO(node, value.toBool());
        break;
    case TreeEnumOrder::kUnit:
        UpdateUnit(node, value.toInt());
        break;
    case TreeEnumOrder::kParty:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toInt(), kParty, &Node::party);
        break;
    case TreeEnumOrder::kEmployee:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toInt(), kEmployee, &Node::employee);
        break;
    case TreeEnumOrder::kDateTime:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kDateTime, &Node::date_time);
        break;
    case TreeEnumOrder::kFinished:
        UpdateFinished(node, value.toBool());
        break;
    default:
        return false;
    }

    emit SResizeColumnToContents(index.column());
    emit SUpdateData(node->id, kColumn, value);
    return true;
}

Qt::ItemFlags TreeModelOrder::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const TreeEnumOrder kColumn { index.column() };

    const bool finished { index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kFinished)).data().toBool() };
    if (!finished)
        flags |= Qt::ItemIsEditable;

    switch (kColumn) {
    case TreeEnumOrder::kName:
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        flags &= ~Qt::ItemIsEditable;
        break;
    case TreeEnumOrder::kType:
    case TreeEnumOrder::kFinished:
    case TreeEnumOrder::kFirst:
    case TreeEnumOrder::kSecond:
    case TreeEnumOrder::kDiscount:
    case TreeEnumOrder::kAmount:
    case TreeEnumOrder::kSettled:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        break;
    }

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

    if (beginMoveRows(source_index.parent(), source_row, source_row, parent, begin_row)) {
        node->parent->children.removeAt(source_row);
        if (node->type == kTypeBranch) {
            UpdateAncestorValueOrder(node, -node->first, -node->second, -node->initial_total, -node->discount, -node->final_total);
        } else {
            if (node->finished) {
                UpdateAncestorValueOrder(node, -node->first, -node->second, -node->initial_total, -node->discount, -node->final_total);
            }
        }

        destination_parent->children.insert(begin_row, node);
        node->unit = destination_parent->unit;
        node->parent = destination_parent;
        node->final_total = node->unit == kUnitIM ? node->initial_total - node->discount : 0.0;

        if (node->finished)
            UpdateAncestorValueOrder(node, node->first, node->second, node->initial_total, node->discount, node->final_total);

        endMoveRows();
    }

    sql_->DragNode(destination_parent->id, node_id);
    emit SResizeColumnToContents(std::to_underlying(TreeEnumOrder::kName));

    return true;
}
