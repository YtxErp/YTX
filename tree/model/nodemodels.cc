#include "nodemodels.h"

#include "global/resourcepool.h"

NodeModelS::NodeModelS(Sqlite* sql, CInfo& info, int default_unit, CTransWgtHash& leaf_wgt_hash, CString& separator, QObject* parent)
    : NodeModel(sql, info, default_unit, leaf_wgt_hash, separator, parent)
{
    cmodel_ = new QStandardItemModel(this);
    vmodel_ = new QStandardItemModel(this);
    emodel_ = new QStandardItemModel(this);

    ConstructTree();
}

NodeModelS::~NodeModelS() { qDeleteAll(node_hash_); }

void NodeModelS::RUpdateStakeholder(int old_node_id, int new_node_id)
{
    const auto& const_node_hash { std::as_const(node_hash_) };

    for (auto* node : const_node_hash) {
        if (node->employee == old_node_id)
            node->employee = new_node_id;
    }
}

void NodeModelS::RSyncDouble(int node_id, int column, double value)
{
    if (column != std::to_underlying(NodeEnumS::kAmount) || node_id <= 0 || value == 0.0)
        return;

    auto* node { node_hash_.value(node_id) };
    if (!node || node == root_ || node->type != kTypeLeaf)
        return;

    node->final_total += value;
    sql_->WriteField(info_.node, kAmount, node->final_total, node_id);

    UpdateAncestorValue(node, 0.0, value);
}

bool NodeModelS::InsertNode(int row, const QModelIndex& parent, Node* node)
{
    if (row <= -1)
        return false;

    auto* parent_node { GetNodeByIndex(parent) };

    beginInsertRows(parent, row, row);
    parent_node->children.insert(row, node);
    endInsertRows();

    sql_->WriteNode(parent_node->id, node);
    node_hash_.insert(node->id, node);

    QString path { NodeModelUtils::ConstructPathFPTS(root_, node, separator_) };

    switch (node->type) {
    case kTypeBranch:
        branch_path_.insert(node->id, path);
        break;
    case kTypeLeaf: {
        leaf_path_.insert(node->id, path);

        const UnitS kUnit { node->unit };

        switch (kUnit) {
        case UnitS::kCust:
            NodeModelUtils::AddItemToModel(cmodel_, path, node->id);
            break;
        case UnitS::kVend:
            NodeModelUtils::AddItemToModel(vmodel_, path, node->id);
            break;
        case UnitS::kEmp:
            NodeModelUtils::AddItemToModel(emodel_, path, node->id);
            break;
        default:
            break;
        }
    } break;
    case kTypeSupport:
        NodeModelUtils::AddItemToModel(support_model_, path, node->id);
        support_path_.insert(node->id, path);
        break;
    default:
        break;
    }

    emit SSearch();
    return true;
}

QList<int> NodeModelS::PartyList(CString& text, int unit) const
{
    QList<int> list {};

    for (auto* node : node_hash_)
        if (node->unit == unit && node->name.contains(text))
            list.emplaceBack(node->id);

    return list;
}

QStandardItemModel* NodeModelS::UnitModelPS(int unit) const
{
    const UnitS kUnit { unit };

    switch (kUnit) {
    case UnitS::kCust:
        return cmodel_;
    case UnitS::kVend:
        return vmodel_;
    case UnitS::kEmp:
        return emodel_;
    default:
        return nullptr;
    }
}

void NodeModelS::UpdateSeparatorFPTS(CString& old_separator, CString& new_separator)
{
    if (old_separator == new_separator || new_separator.isEmpty())
        return;

    NodeModelUtils::UpdatePathSeparatorFPTS(old_separator, new_separator, leaf_path_);
    NodeModelUtils::UpdatePathSeparatorFPTS(old_separator, new_separator, branch_path_);
    NodeModelUtils::UpdatePathSeparatorFPTS(old_separator, new_separator, support_path_);

    NodeModelUtils::UpdateModelSeparatorFPTS(support_model_, support_path_);
    NodeModelUtils::UpdateModelSeparatorFPTS(cmodel_, leaf_path_);
    NodeModelUtils::UpdateModelSeparatorFPTS(vmodel_, leaf_path_);
    NodeModelUtils::UpdateModelSeparatorFPTS(emodel_, leaf_path_);
}

bool NodeModelS::UpdateUnit(Node* node, int value)
{
    if (node->unit == value)
        return false;

    const int node_id { node->id };
    QString message { tr("Cannot change %1 unit,").arg(GetPath(node_id)) };

    if (NodeModelUtils::HasChildrenFPTS(node, message))
        return false;

    if (NodeModelUtils::IsInternalReferencedFPTS(sql_, node_id, message))
        return false;

    if (NodeModelUtils::IsExternalReferencedPS(sql_, node_id, message))
        return false;

    if (NodeModelUtils::IsSupportReferencedFPTS(sql_, node_id, message))
        return false;

    RemoveItem(node_id, node->unit);

    const auto& path { GetPath(node_id) };
    AddItem(node_id, path, value);

    node->unit = value;
    sql_->WriteField(info_.node, kUnit, value, node_id);

    return true;
}

bool NodeModelS::UpdateNameFunction(Node* node, CString& value)
{
    node->name = value;
    sql_->WriteField(info_.node, kName, value, node->id);

    NodeModelUtils::UpdatePathFPTS(leaf_path_, branch_path_, support_path_, root_, node, separator_);
    NodeModelUtils::UpdateModel(leaf_path_, leaf_model_, support_path_, support_model_, node);
    NodeModelUtils::UpdateUnitModel(leaf_path_, cmodel_, node, std::to_underlying(UnitS::kCust), Filter::kIncludeSpecific);
    NodeModelUtils::UpdateUnitModel(leaf_path_, vmodel_, node, std::to_underlying(UnitS::kVend), Filter::kIncludeSpecific);
    NodeModelUtils::UpdateUnitModel(leaf_path_, emodel_, node, std::to_underlying(UnitS::kEmp), Filter::kIncludeSpecific);

    emit SResizeColumnToContents(std::to_underlying(NodeEnum::kName));
    emit SSearch();
    return true;
}

bool NodeModelS::UpdateAncestorValue(
    Node* node, double /*initial_delta*/, double final_delta, double /*first_delta*/, double /*second_delta*/, double /*discount_delta*/)
{
    if (!node || node == root_ || !node->parent || node->parent == root_)
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

void NodeModelS::RemoveItem(int node_id, int unit)
{
    const UnitS kUnit { unit };

    switch (kUnit) {
    case UnitS::kCust:
        NodeModelUtils::RemoveItemFromModel(cmodel_, node_id);
        break;
    case UnitS::kVend:
        NodeModelUtils::RemoveItemFromModel(vmodel_, node_id);
        break;
    case UnitS::kEmp:
        NodeModelUtils::RemoveItemFromModel(emodel_, node_id);
        break;
    default:
        break;
    }
}

void NodeModelS::AddItem(int node_id, CString& path, int unit)
{
    const UnitS kUnit { unit };

    switch (kUnit) {
    case UnitS::kCust:
        NodeModelUtils::AddItemToModel(cmodel_, path, node_id);
        break;
    case UnitS::kVend:
        NodeModelUtils::AddItemToModel(vmodel_, path, node_id);
        break;
    case UnitS::kEmp:
        NodeModelUtils::AddItemToModel(emodel_, path, node_id);
        break;
    default:
        break;
    }
}

void NodeModelS::ConstructTree()
{
    sql_->ReadNode(node_hash_);
    const auto& const_node_hash { std::as_const(node_hash_) };
    QSet<int> crange {};
    QSet<int> vrange {};
    QSet<int> erange {};

    for (auto* node : const_node_hash) {
        if (!node->parent) {
            node->parent = root_;
            root_->children.emplace_back(node);
        }

        const UnitS kUnit { node->unit };

        switch (kUnit) {
        case UnitS::kCust:
            crange.insert(node->id);
            break;
        case UnitS::kVend:
            vrange.insert(node->id);
            break;
        case UnitS::kEmp:
            erange.insert(node->id);
            break;
        default:
            break;
        }
    }

    QString path {};
    for (auto* node : const_node_hash) {
        path = NodeModelUtils::ConstructPathFPTS(root_, node, separator_);

        switch (node->type) {
        case kTypeBranch:
            branch_path_.insert(node->id, path);
            break;
        case kTypeLeaf:
            UpdateAncestorValue(node, 0.0, node->final_total);
            leaf_path_.insert(node->id, path);
            break;
        case kTypeSupport:
            support_path_.insert(node->id, path);
            break;
        default:
            break;
        }
    }

    NodeModelUtils::SupportPathFilterModelFPTS(support_path_, support_model_, 0, Filter::kIncludeAllWithNone);
    NodeModelUtils::LeafPathRangeModelS(leaf_path_, crange, cmodel_, vrange, vmodel_, erange, emodel_);
}

bool NodeModelS::UpdateTypeFPTS(Node* node, int value)
{
    if (node->type == value)
        return false;

    const int node_id { node->id };
    QString message { tr("Cannot change %1 type,").arg(GetPath(node_id)) };

    if (NodeModelUtils::HasChildrenFPTS(node, message))
        return false;

    if (NodeModelUtils::IsOpenedFPTS(leaf_wgt_hash_, node_id, message))
        return false;

    if (NodeModelUtils::IsInternalReferencedFPTS(sql_, node_id, message))
        return false;

    if (NodeModelUtils::IsExternalReferencedPS(sql_, node_id, message))
        return false;

    QString path {};

    switch (node->type) {
    case kTypeBranch:
        path = branch_path_.take(node_id);
        break;
    case kTypeLeaf:
        path = leaf_path_.take(node_id);
        RemoveItem(node_id, node->unit);
        break;
    case kTypeSupport:
        path = support_path_.take(node_id);
        NodeModelUtils::RemoveItemFromModel(support_model_, node->id);
        break;
    default:
        break;
    }

    node->type = value;
    sql_->WriteField(info_.node, kType, value, node_id);

    switch (value) {
    case kTypeBranch:
        branch_path_.insert(node_id, path);
        break;
    case kTypeLeaf:
        leaf_path_.insert(node_id, path);
        AddItem(node_id, path, node->unit);
        break;
    case kTypeSupport:
        support_path_.insert(node_id, path);
        NodeModelUtils::AddItemToModel(support_model_, path, node_id);
        break;
    default:
        break;
    }

    return true;
}

void NodeModelS::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.node_header.size())
        return;

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
        case NodeEnumS::kRule:
            return (order == Qt::AscendingOrder) ? (lhs->rule < rhs->rule) : (lhs->rule > rhs->rule);
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

bool NodeModelS::RemoveNode(int row, const QModelIndex& parent)
{
    if (row <= -1 || row >= rowCount(parent))
        return false;

    auto* parent_node { GetNodeByIndex(parent) };
    auto* node { parent_node->children.at(row) };

    int node_id { node->id };

    beginRemoveRows(parent, row, row);
    parent_node->children.removeOne(node);
    endRemoveRows();

    switch (node->type) {
    case kTypeBranch: {
        for (auto* child : std::as_const(node->children)) {
            child->parent = parent_node;
            parent_node->children.emplace_back(child);
        }

        NodeModelUtils::UpdatePathFPTS(leaf_path_, branch_path_, support_path_, root_, node, separator_);
        NodeModelUtils::UpdateModel(leaf_path_, leaf_model_, support_path_, support_model_, node);

        branch_path_.remove(node_id);
        emit SUpdateName(node_id, node->name, true);

    } break;
    case kTypeLeaf: {
        leaf_path_.remove(node_id);
        RemoveItem(node_id, node->unit);
    } break;
    case kTypeSupport: {
        NodeModelUtils::RemoveItemFromModel(support_model_, node_id);
        support_path_.remove(node_id);
    } break;
    default:
        break;
    }

    ResourcePool<Node>::Instance().Recycle(node);
    node_hash_.remove(node_id);

    emit SSearch();
    emit SResizeColumnToContents(std::to_underlying(NodeEnumS::kName));

    return true;
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
    case NodeEnumS::kRule:
        return node->rule;
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
        NodeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kCode, &Node::code);
        break;
    case NodeEnumS::kDescription:
        NodeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kDescription, &Node::description);
        break;
    case NodeEnumS::kNote:
        NodeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kNote, &Node::note);
        break;
    case NodeEnumS::kRule:
        NodeModelUtils::UpdateField(sql_, node, info_.node, value.toBool(), kRule, &Node::rule, true);
        break;
    case NodeEnumS::kType:
        UpdateTypeFPTS(node, value.toInt());
        break;
    case NodeEnumS::kUnit:
        UpdateUnit(node, value.toInt());
        break;
    case NodeEnumS::kDeadline:
        NodeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kDeadline, &Node::date_time, true);
        break;
    case NodeEnumS::kEmployee:
        NodeModelUtils::UpdateField(sql_, node, info_.node, value.toInt(), kEmployee, &Node::employee, true);
        break;
    case NodeEnumS::kPaymentTerm:
        NodeModelUtils::UpdateField(sql_, node, info_.node, value.toDouble(), kPaymentTerm, &Node::first, true);
        break;
    case NodeEnumS::kTaxRate:
        NodeModelUtils::UpdateField(sql_, node, info_.node, value.toDouble(), kTaxRate, &Node::second, true);
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

    auto* node { NodeModelUtils::GetNodeByID(node_hash_, node_id) };
    if (!node || node->parent == destination_parent || NodeModelUtils::IsDescendant(destination_parent, node))
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
    NodeModelUtils::UpdatePathFPTS(leaf_path_, branch_path_, support_path_, root_, node, separator_);
    NodeModelUtils::UpdateModel(leaf_path_, leaf_model_, support_path_, support_model_, node);
    NodeModelUtils::UpdateUnitModel(leaf_path_, cmodel_, node, std::to_underlying(UnitS::kCust), Filter::kIncludeSpecific);
    NodeModelUtils::UpdateUnitModel(leaf_path_, vmodel_, node, std::to_underlying(UnitS::kVend), Filter::kIncludeSpecific);
    NodeModelUtils::UpdateUnitModel(leaf_path_, emodel_, node, std::to_underlying(UnitS::kEmp), Filter::kIncludeSpecific);

    emit SUpdateName(node_id, node->name, node->type == kTypeBranch);
    emit SResizeColumnToContents(std::to_underlying(NodeEnumS::kName));
    return true;
}
