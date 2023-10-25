#include "treemodelstakeholder.h"

#include "global/resourcepool.h"

TreeModelStakeholder::TreeModelStakeholder(Sqlite* sql, CInfo& info, int default_unit, CTableHash& table_hash, CString& separator, QObject* parent)
    : TreeModel(sql, info, default_unit, table_hash, separator, parent)
{
    cmodel_ = new QStandardItemModel(this);
    vmodel_ = new QStandardItemModel(this);
    emodel_ = new QStandardItemModel(this);

    ConstructTree();
}

TreeModelStakeholder::~TreeModelStakeholder() { qDeleteAll(node_hash_); }

void TreeModelStakeholder::RUpdateStakeholder(int old_node_id, int new_node_id)
{
    const auto& const_node_hash { std::as_const(node_hash_) };

    for (auto* node : const_node_hash) {
        if (node->employee == old_node_id)
            node->employee = new_node_id;
    }
}

void TreeModelStakeholder::UpdateNodeFPTS(const Node* tmp_node)
{
    if (!tmp_node)
        return;

    auto* node { const_cast<Node*>(TreeModelUtils::GetNodeByID(node_hash_, tmp_node->id)) };
    if (*node == *tmp_node)
        return;

    UpdateTypeFPTS(node, tmp_node->type);

    if (node->name != tmp_node->name) {
        UpdateName(node, tmp_node->name);
        emit SUpdateName(node->id, node->name, node->type == kTypeBranch);
    }

    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->description, kDescription, &Node::description);
    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->code, kCode, &Node::code);
    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->note, kNote, &Node::note);
    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->first, kPaymentTerm, &Node::first);
    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->second, kTaxRate, &Node::second);
    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->date_time, kDeadline, &Node::date_time);
    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->rule, kRule, &Node::rule);
    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->employee, kEmployee, &Node::employee);
    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->unit, kUnit, &Node::unit);
}

bool TreeModelStakeholder::InsertNode(int row, const QModelIndex& parent, Node* node)
{
    if (row <= -1)
        return false;

    auto* parent_node { GetNodeByIndex(parent) };

    beginInsertRows(parent, row, row);
    parent_node->children.insert(row, node);
    endInsertRows();

    sql_->WriteNode(parent_node->id, node);
    node_hash_.insert(node->id, node);

    QString path { TreeModelUtils::ConstructPathFPTS(root_, node, separator_) };

    switch (node->type) {
    case kTypeBranch:
        branch_path_.insert(node->id, path);
        break;
    case kTypeLeaf: {
        leaf_path_.insert(node->id, path);

        switch (node->unit) {
        case kUnitCust:
            TreeModelUtils::AddItemToModel(cmodel_, path, node->id);
            break;
        case kUnitVend:
            TreeModelUtils::AddItemToModel(vmodel_, path, node->id);
            break;
        case kUnitEmp:
            TreeModelUtils::AddItemToModel(emodel_, path, node->id);
            break;
        default:
            break;
        }
    } break;
    case kTypeSupport:
        TreeModelUtils::AddItemToModel(support_model_, path, node->id);
        support_path_.insert(node->id, path);
        break;
    default:
        break;
    }

    emit SSearch();
    return true;
}

QList<int> TreeModelStakeholder::PartyList(CString& text, int unit) const
{
    QList<int> list {};

    for (auto* node : node_hash_)
        if (node->unit == unit && node->name.contains(text))
            list.emplaceBack(node->id);

    return list;
}

QStandardItemModel* TreeModelStakeholder::UnitModelPS(int unit) const
{
    switch (unit) {
    case kUnitCust:
        return cmodel_;
    case kUnitVend:
        return vmodel_;
    case kUnitEmp:
        return emodel_;
    default:
        return nullptr;
    }
}

void TreeModelStakeholder::UpdateSeparatorFPTS(CString& old_separator, CString& new_separator)
{
    if (old_separator == new_separator || new_separator.isEmpty())
        return;

    TreeModelUtils::UpdatePathSeparatorFPTS(old_separator, new_separator, leaf_path_);
    TreeModelUtils::UpdatePathSeparatorFPTS(old_separator, new_separator, branch_path_);
    TreeModelUtils::UpdatePathSeparatorFPTS(old_separator, new_separator, support_path_);

    TreeModelUtils::UpdateModelSeparatorFPTS(support_model_, support_path_);
    TreeModelUtils::UpdateModelSeparatorFPTS(cmodel_, leaf_path_);
    TreeModelUtils::UpdateModelSeparatorFPTS(vmodel_, leaf_path_);
    TreeModelUtils::UpdateModelSeparatorFPTS(emodel_, leaf_path_);
}

bool TreeModelStakeholder::UpdateUnit(Node* node, int value)
{
    if (node->unit == value)
        return false;

    const int node_id { node->id };
    QString message { tr("Cannot change %1 unit,").arg(GetPath(node_id)) };

    if (TreeModelUtils::HasChildrenFPTS(node, message))
        return false;

    if (TreeModelUtils::IsInternalReferencedFPTS(sql_, node_id, message))
        return false;

    if (TreeModelUtils::IsExternalReferencedPS(sql_, node_id, message))
        return false;

    if (TreeModelUtils::IsSupportReferencedFPTS(sql_, node_id, message))
        return false;

    if (node->type == kTypeLeaf) {
        RemoveItem(node_id, node->unit);

        const auto& path { GetPath(node_id) };
        AddItem(node_id, path, value);
    }

    node->unit = value;
    sql_->UpdateField(info_.node, value, kUnit, node_id);

    return true;
}

bool TreeModelStakeholder::UpdateName(Node* node, CString& value)
{
    node->name = value;
    sql_->UpdateField(info_.node, value, kName, node->id);

    TreeModelUtils::UpdatePathFPTS(leaf_path_, branch_path_, support_path_, root_, node, separator_);
    TreeModelUtils::UpdateModel(leaf_path_, leaf_model_, support_path_, support_model_, node);
    TreeModelUtils::UpdateUnitModel(leaf_path_, cmodel_, node, kUnitCust, Filter::kIncludeSpecific);
    TreeModelUtils::UpdateUnitModel(leaf_path_, vmodel_, node, kUnitVend, Filter::kIncludeSpecific);
    TreeModelUtils::UpdateUnitModel(leaf_path_, emodel_, node, kUnitEmp, Filter::kIncludeSpecific);

    emit SResizeColumnToContents(std::to_underlying(TreeEnum::kName));
    emit SSearch();
    return true;
}

void TreeModelStakeholder::RemoveItem(int node_id, int unit)
{
    switch (unit) {
    case kUnitCust:
        TreeModelUtils::RemoveItemFromModel(cmodel_, node_id);
        break;
    case kUnitVend:
        TreeModelUtils::RemoveItemFromModel(vmodel_, node_id);
        break;
    case kUnitEmp:
        TreeModelUtils::RemoveItemFromModel(emodel_, node_id);
        break;
    default:
        break;
    }
}

void TreeModelStakeholder::AddItem(int node_id, CString& path, int unit)
{
    switch (unit) {
    case kUnitCust:
        TreeModelUtils::AddItemToModel(cmodel_, path, node_id);
        break;
    case kUnitVend:
        TreeModelUtils::AddItemToModel(vmodel_, path, node_id);
        break;
    case kUnitEmp:
        TreeModelUtils::AddItemToModel(emodel_, path, node_id);
        break;
    default:
        break;
    }
}

void TreeModelStakeholder::ConstructTree()
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

        switch (node->unit) {
        case kUnitCust:
            crange.insert(node->id);
            break;
        case kUnitVend:
            vrange.insert(node->id);
            break;
        case kUnitEmp:
            erange.insert(node->id);
            break;
        default:
            break;
        }
    }

    QString path {};
    for (auto* node : const_node_hash) {
        path = TreeModelUtils::ConstructPathFPTS(root_, node, separator_);

        switch (node->type) {
        case kTypeBranch:
            branch_path_.insert(node->id, path);
            break;
        case kTypeLeaf:
            leaf_path_.insert(node->id, path);
            break;
        case kTypeSupport:
            support_path_.insert(node->id, path);
            break;
        default:
            break;
        }
    }

    TreeModelUtils::SupportPathFilterModelFPTS(support_path_, support_model_, 0, Filter::kIncludeAllWithNone);
    TreeModelUtils::LeafPathRangeModelS(leaf_path_, crange, cmodel_, vrange, vmodel_, erange, emodel_);
}

bool TreeModelStakeholder::UpdateTypeFPTS(Node* node, int value)
{
    if (node->type == value)
        return false;

    const int node_id { node->id };
    QString message { tr("Cannot change %1 type,").arg(GetPath(node_id)) };

    if (TreeModelUtils::HasChildrenFPTS(node, message))
        return false;

    if (TreeModelUtils::IsOpenedFPTS(table_hash_, node_id, message))
        return false;

    if (TreeModelUtils::IsInternalReferencedFPTS(sql_, node_id, message))
        return false;

    if (TreeModelUtils::IsExternalReferencedPS(sql_, node_id, message))
        return false;

    QString path {};

    switch (node->type) {
    case kTypeBranch:
        path = branch_path_.take(node_id);
        break;
    case kTypeLeaf:
        path = leaf_path_.take(node_id);
        break;
    case kTypeSupport:
        TreeModelUtils::RemoveItemFromModel(support_model_, node->id);
        path = support_path_.take(node_id);
        break;
    default:
        break;
    }

    node->type = value;
    sql_->UpdateField(info_.node, value, kType, node_id);

    switch (value) {
    case kTypeBranch:
        branch_path_.insert(node_id, path);
        break;
    case kTypeLeaf:
        leaf_path_.insert(node_id, path);
        break;
    case kTypeSupport:
        TreeModelUtils::AddItemToModel(support_model_, path, node_id);
        support_path_.insert(node_id, path);
        break;
    default:
        break;
    }

    return true;
}

void TreeModelStakeholder::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.tree_header.size())
        return;

    auto Compare = [column, order](const Node* lhs, const Node* rhs) -> bool {
        const TreeEnumStakeholder kColumn { column };
        switch (kColumn) {
        case TreeEnumStakeholder::kName:
            return (order == Qt::AscendingOrder) ? (lhs->name < rhs->name) : (lhs->name > rhs->name);
        case TreeEnumStakeholder::kCode:
            return (order == Qt::AscendingOrder) ? (lhs->code < rhs->code) : (lhs->code > rhs->code);
        case TreeEnumStakeholder::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case TreeEnumStakeholder::kNote:
            return (order == Qt::AscendingOrder) ? (lhs->note < rhs->note) : (lhs->note > rhs->note);
        case TreeEnumStakeholder::kRule:
            return (order == Qt::AscendingOrder) ? (lhs->rule < rhs->rule) : (lhs->rule > rhs->rule);
        case TreeEnumStakeholder::kType:
            return (order == Qt::AscendingOrder) ? (lhs->type < rhs->type) : (lhs->type > rhs->type);
        case TreeEnumStakeholder::kUnit:
            return (order == Qt::AscendingOrder) ? (lhs->unit < rhs->unit) : (lhs->unit > rhs->unit);
        case TreeEnumStakeholder::kDeadline:
            return (order == Qt::AscendingOrder) ? (lhs->date_time < rhs->date_time) : (lhs->date_time > rhs->date_time);
        case TreeEnumStakeholder::kEmployee:
            return (order == Qt::AscendingOrder) ? (lhs->employee < rhs->employee) : (lhs->employee > rhs->employee);
        case TreeEnumStakeholder::kPaymentTerm:
            return (order == Qt::AscendingOrder) ? (lhs->first < rhs->first) : (lhs->first > rhs->first);
        case TreeEnumStakeholder::kTaxRate:
            return (order == Qt::AscendingOrder) ? (lhs->second < rhs->second) : (lhs->second > rhs->second);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    TreeModelUtils::SortIterative(root_, Compare);
    emit layoutChanged();
}

bool TreeModelStakeholder::RemoveNode(int row, const QModelIndex& parent)
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
        for (auto* child : node->children) {
            child->parent = parent_node;
            parent_node->children.emplace_back(child);
        }

        TreeModelUtils::UpdatePathFPTS(leaf_path_, branch_path_, support_path_, root_, node, separator_);
        TreeModelUtils::UpdateModel(leaf_path_, leaf_model_, support_path_, support_model_, node);

        branch_path_.remove(node_id);
        emit SUpdateName(node_id, node->name, true);

    } break;
    case kTypeLeaf: {
        leaf_path_.remove(node_id);
        RemoveItem(node_id, node->unit);
    } break;
    case kTypeSupport: {
        TreeModelUtils::RemoveItemFromModel(support_model_, node_id);
        support_path_.remove(node_id);
    } break;
    default:
        break;
    }

    emit SSearch();
    emit SResizeColumnToContents(std::to_underlying(TreeEnumStakeholder::kName));

    ResourcePool<Node>::Instance().Recycle(node);
    node_hash_.remove(node_id);

    return true;
}

QVariant TreeModelStakeholder::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* node { GetNodeByIndex(index) };
    if (node == root_)
        return QVariant();

    const TreeEnumStakeholder kColumn { index.column() };
    bool skip { node->type == kTypeBranch || node->unit == kUnitProd || node->rule == kRuleIS };

    switch (kColumn) {
    case TreeEnumStakeholder::kName:
        return node->name;
    case TreeEnumStakeholder::kID:
        return node->id;
    case TreeEnumStakeholder::kCode:
        return node->code;
    case TreeEnumStakeholder::kDescription:
        return node->description;
    case TreeEnumStakeholder::kNote:
        return node->note;
    case TreeEnumStakeholder::kRule:
        return node->rule;
    case TreeEnumStakeholder::kType:
        return node->type;
    case TreeEnumStakeholder::kUnit:
        return node->unit;
    case TreeEnumStakeholder::kDeadline:
        return node->date_time.isEmpty() || skip ? QVariant() : node->date_time;
    case TreeEnumStakeholder::kEmployee:
        return node->employee == 0 ? QVariant() : node->employee;
    case TreeEnumStakeholder::kPaymentTerm:
        return node->first == 0 || skip ? QVariant() : node->first;
    case TreeEnumStakeholder::kTaxRate:
        return node->second == 0 ? QVariant() : node->second;
    default:
        return QVariant();
    }
}

bool TreeModelStakeholder::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    auto* node { GetNodeByIndex(index) };
    if (node == root_)
        return false;

    const TreeEnumStakeholder kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumStakeholder::kCode:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kCode, &Node::code);
        break;
    case TreeEnumStakeholder::kDescription:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kDescription, &Node::description);
        break;
    case TreeEnumStakeholder::kNote:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kNote, &Node::note);
        break;
    case TreeEnumStakeholder::kRule:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toBool(), kRule, &Node::rule);
        break;
    case TreeEnumStakeholder::kType:
        UpdateTypeFPTS(node, value.toInt());
        break;
    case TreeEnumStakeholder::kUnit:
        UpdateUnit(node, value.toInt());
        break;
    case TreeEnumStakeholder::kDeadline:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kDeadline, &Node::date_time);
        break;
    case TreeEnumStakeholder::kEmployee:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toInt(), kEmployee, &Node::employee);
        break;
    case TreeEnumStakeholder::kPaymentTerm:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toDouble(), kPaymentTerm, &Node::first);
        break;
    case TreeEnumStakeholder::kTaxRate:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toDouble(), kTaxRate, &Node::second);
        break;
    default:
        return false;
    }

    emit SResizeColumnToContents(index.column());
    return true;
}

Qt::ItemFlags TreeModelStakeholder::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const TreeEnumStakeholder kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumStakeholder::kName:
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}

bool TreeModelStakeholder::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
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

        destination_parent->children.insert(begin_row, node);
        node->parent = destination_parent;

        endMoveRows();
    }

    sql_->DragNode(destination_parent->id, node_id);
    TreeModelUtils::UpdatePathFPTS(leaf_path_, branch_path_, support_path_, root_, node, separator_);
    TreeModelUtils::UpdateModel(leaf_path_, leaf_model_, support_path_, support_model_, node);
    TreeModelUtils::UpdateUnitModel(leaf_path_, cmodel_, node, kUnitCust, Filter::kIncludeSpecific);
    TreeModelUtils::UpdateUnitModel(leaf_path_, vmodel_, node, kUnitVend, Filter::kIncludeSpecific);
    TreeModelUtils::UpdateUnitModel(leaf_path_, emodel_, node, kUnitEmp, Filter::kIncludeSpecific);

    emit SUpdateName(node_id, node->name, node->type == kTypeBranch);
    emit SResizeColumnToContents(std::to_underlying(TreeEnumStakeholder::kName));
    return true;
}
