#include "treemodelproduct.h"

#include "global/resourcepool.h"

TreeModelProduct::TreeModelProduct(Sqlite* sql, CInfo& info, int default_unit, CTableHash& table_hash, CString& separator, QObject* parent)
    : TreeModel(sql, info, default_unit, table_hash, separator, parent)
{
    leaf_model_ = new QStandardItemModel(this);
    product_model_ = new QStandardItemModel(this);

    ConstructTree();
}

TreeModelProduct::~TreeModelProduct() { qDeleteAll(node_hash_); }

void TreeModelProduct::RUpdateLeafValue(
    int node_id, double initial_debit_diff, double initial_credit_diff, double final_debit_diff, double final_credit_diff, double /*settled_diff*/)
{
    auto* node { TreeModelUtils::GetNodeByID(node_hash_, node_id) };
    if (!node || node == root_ || node->type != kTypeLeaf)
        return;

    if (initial_credit_diff == 0 && initial_debit_diff == 0 && final_debit_diff == 0 && final_credit_diff == 0)
        return;

    bool rule { node->rule };

    double initial_diff { (rule ? 1 : -1) * (initial_credit_diff - initial_debit_diff) };
    double final_diff { (rule ? 1 : -1) * (final_credit_diff - final_debit_diff) };

    node->initial_total += initial_diff;
    node->final_total += final_diff;

    sql_->UpdateNodeValue(node);
    TreeModelUtils::UpdateAncestorValueFPT(root_, node, initial_diff, final_diff);
    emit SUpdateDSpinBox();
}

void TreeModelProduct::RUpdateMultiLeafTotal(const QList<int>& node_list)
{
    double old_final_total {};
    double old_initial_total {};
    double final_diff {};
    double initial_diff {};
    Node* node {};

    for (int node_id : node_list) {
        node = TreeModelUtils::GetNodeByID(node_hash_, node_id);

        if (!node || node->type != kTypeLeaf)
            continue;

        old_final_total = node->final_total;
        old_initial_total = node->initial_total;

        sql_->LeafTotal(node);
        sql_->UpdateNodeValue(node);

        final_diff = node->final_total - old_final_total;
        initial_diff = node->initial_total - old_initial_total;

        TreeModelUtils::UpdateAncestorValueFPT(root_, node, initial_diff, final_diff);
    }

    emit SUpdateDSpinBox();
}

void TreeModelProduct::UpdateNodeFPTS(const Node* tmp_node)
{
    if (!tmp_node)
        return;

    auto* node { TreeModelUtils::GetNodeByID(node_hash_, tmp_node->id) };
    if (*node == *tmp_node)
        return;

    UpdateRuleFPTO(node, tmp_node->rule);
    UpdateUnit(node, tmp_node->unit);
    UpdateTypeFPTS(node, tmp_node->type);

    if (node->name != tmp_node->name) {
        UpdateName(node, tmp_node->name);
        emit SUpdateName(node->id, node->name, node->type == kTypeBranch);
    }

    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->description, kDescription, &Node::description);
    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->code, kCode, &Node::code);
    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->note, kNote, &Node::note);
    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->first, kUnitPrice, &Node::first);
    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->second, kCommission, &Node::second);
    TreeModelUtils::UpdateField(sql_, node, info_.node, tmp_node->color, kColor, &Node::color);
}

bool TreeModelProduct::RemoveNode(int row, const QModelIndex& parent)
{
    if (row <= -1 || row >= rowCount(parent))
        return false;

    auto* parent_node { GetNodeByIndex(parent) };
    auto* node { parent_node->children.at(row) };

    beginRemoveRows(parent, row, row);
    parent_node->children.removeOne(node);
    endRemoveRows();

    int node_id { node->id };

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
        TreeModelUtils::UpdateAncestorValueFPT(root_, node, -node->initial_total, -node->final_total);
        TreeModelUtils::RemoveItemFromModel(leaf_model_, node_id);
        leaf_path_.remove(node_id);

        if (node->unit != kUnitPos) {
            TreeModelUtils::RemoveItemFromModel(product_model_, node_id);
        }
    } break;
    case kTypeSupport: {
        TreeModelUtils::RemoveItemFromModel(support_model_, node_id);
        support_path_.remove(node_id);
    } break;
    default:
        break;
    }

    emit SSearch();
    emit SResizeColumnToContents(std::to_underlying(TreeEnum::kName));

    ResourcePool<Node>::Instance().Recycle(node);
    node_hash_.remove(node_id);

    return true;
}

bool TreeModelProduct::InsertNode(int row, const QModelIndex& parent, Node* node)
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
        TreeModelUtils::AddItemToModel(leaf_model_, path, node->id);
        leaf_path_.insert(node->id, path);

        if (node->unit != kUnitPos) {
            TreeModelUtils::AddItemToModel(product_model_, path, node->id);
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

void TreeModelProduct::UpdateSeparatorFPTS(CString& old_separator, CString& new_separator)
{
    if (old_separator == new_separator || new_separator.isEmpty())
        return;

    TreeModelUtils::UpdatePathSeparatorFPTS(old_separator, new_separator, leaf_path_);
    TreeModelUtils::UpdatePathSeparatorFPTS(old_separator, new_separator, branch_path_);
    TreeModelUtils::UpdatePathSeparatorFPTS(old_separator, new_separator, support_path_);

    TreeModelUtils::UpdateModelSeparatorFPTS(leaf_model_, leaf_path_);
    TreeModelUtils::UpdateModelSeparatorFPTS(support_model_, support_path_);
    TreeModelUtils::UpdateModelSeparatorFPTS(product_model_, leaf_path_);
}

bool TreeModelProduct::UpdateUnit(Node* node, int value)
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

    node->unit = value;
    sql_->UpdateField(info_.node, value, kUnit, node_id);

    if (value == kUnitPos)
        TreeModelUtils::RemoveItemFromModel(product_model_, node_id);
    else
        TreeModelUtils::AddItemToModel(product_model_, leaf_path_.value(node_id), node_id);

    return true;
}

void TreeModelProduct::ConstructTree()
{
    sql_->ReadNode(node_hash_);
    if (node_hash_.isEmpty())
        return;

    const auto& const_node_hash { std::as_const(node_hash_) };
    QSet<int> range {};

    for (auto* node : const_node_hash) {
        if (!node->parent) {
            node->parent = root_;
            root_->children.emplace_back(node);
        }

        if (node->unit != kUnitPos)
            range.insert(node->id);
    }

    QString path {};
    for (auto* node : const_node_hash) {
        path = TreeModelUtils::ConstructPathFPTS(root_, node, separator_);

        switch (node->type) {
        case kTypeBranch:
            branch_path_.insert(node->id, path);
            break;
        case kTypeLeaf:
            TreeModelUtils::UpdateAncestorValueFPT(root_, node, node->initial_total, node->final_total);
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
    TreeModelUtils::LeafPathModelFPT(leaf_path_, leaf_model_);
    TreeModelUtils::LeafPathRangeModelP(leaf_path_, range, product_model_);
}

bool TreeModelProduct::UpdateName(Node* node, CString& value)
{
    node->name = value;
    sql_->UpdateField(info_.node, value, kName, node->id);

    TreeModelUtils::UpdatePathFPTS(leaf_path_, branch_path_, support_path_, root_, node, separator_);
    TreeModelUtils::UpdateModel(leaf_path_, leaf_model_, support_path_, support_model_, node);
    TreeModelUtils::UpdateUnitModel(leaf_path_, product_model_, node, kUnitPos, Filter::kExcludeSpecific);

    emit SResizeColumnToContents(std::to_underlying(TreeEnum::kName));
    emit SSearch();
    return true;
}

void TreeModelProduct::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.tree_header.size())
        return;

    auto Compare = [column, order](const Node* lhs, const Node* rhs) -> bool {
        const TreeEnumProduct kColumn { column };
        switch (kColumn) {
        case TreeEnumProduct::kName:
            return (order == Qt::AscendingOrder) ? (lhs->name < rhs->name) : (lhs->name > rhs->name);
        case TreeEnumProduct::kCode:
            return (order == Qt::AscendingOrder) ? (lhs->code < rhs->code) : (lhs->code > rhs->code);
        case TreeEnumProduct::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case TreeEnumProduct::kNote:
            return (order == Qt::AscendingOrder) ? (lhs->note < rhs->note) : (lhs->note > rhs->note);
        case TreeEnumProduct::kRule:
            return (order == Qt::AscendingOrder) ? (lhs->rule < rhs->rule) : (lhs->rule > rhs->rule);
        case TreeEnumProduct::kType:
            return (order == Qt::AscendingOrder) ? (lhs->type < rhs->type) : (lhs->type > rhs->type);
        case TreeEnumProduct::kUnit:
            return (order == Qt::AscendingOrder) ? (lhs->unit < rhs->unit) : (lhs->unit > rhs->unit);
        case TreeEnumProduct::kColor:
            return (order == Qt::AscendingOrder) ? (lhs->color < rhs->color) : (lhs->color > rhs->color);
        case TreeEnumProduct::kCommission:
            return (order == Qt::AscendingOrder) ? (lhs->second < rhs->second) : (lhs->second > rhs->second);
        case TreeEnumProduct::kUnitPrice:
            return (order == Qt::AscendingOrder) ? (lhs->first < rhs->first) : (lhs->first > rhs->first);
        case TreeEnumProduct::kQuantity:
            return (order == Qt::AscendingOrder) ? (lhs->initial_total < rhs->initial_total) : (lhs->initial_total > rhs->initial_total);
        case TreeEnumProduct::kAmount:
            return (order == Qt::AscendingOrder) ? (lhs->final_total < rhs->final_total) : (lhs->final_total > rhs->final_total);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    TreeModelUtils::SortIterative(root_, Compare);
    emit layoutChanged();
}

QVariant TreeModelProduct::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* node { GetNodeByIndex(index) };
    if (node == root_)
        return QVariant();

    const TreeEnumProduct kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumProduct::kName:
        return node->name;
    case TreeEnumProduct::kID:
        return node->id;
    case TreeEnumProduct::kCode:
        return node->code;
    case TreeEnumProduct::kDescription:
        return node->description;
    case TreeEnumProduct::kNote:
        return node->note;
    case TreeEnumProduct::kRule:
        return node->rule;
    case TreeEnumProduct::kType:
        return node->type;
    case TreeEnumProduct::kUnit:
        return node->unit;
    case TreeEnumProduct::kColor:
        return node->color;
    case TreeEnumProduct::kCommission:
        return node->second == 0 ? QVariant() : node->second;
    case TreeEnumProduct::kUnitPrice:
        return node->first == 0 ? QVariant() : node->first;
    case TreeEnumProduct::kQuantity:
        return node->initial_total == 0 ? QVariant() : node->initial_total;
    case TreeEnumProduct::kAmount:
        return node->final_total;
    default:
        return QVariant();
    }
}

bool TreeModelProduct::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    auto* node { GetNodeByIndex(index) };
    if (node == root_)
        return false;

    const TreeEnumProduct kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumProduct::kCode:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kCode, &Node::code);
        break;
    case TreeEnumProduct::kDescription:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kDescription, &Node::description);
        break;
    case TreeEnumProduct::kNote:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kNote, &Node::note);
        break;
    case TreeEnumProduct::kRule:
        UpdateRuleFPTO(node, value.toBool());
        break;
    case TreeEnumProduct::kType:
        UpdateTypeFPTS(node, value.toInt());
        break;
    case TreeEnumProduct::kColor:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kColor, &Node::color);
        break;
    case TreeEnumProduct::kUnit:
        UpdateUnit(node, value.toInt());
        break;
    case TreeEnumProduct::kCommission:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toDouble(), kCommission, &Node::second);
        break;
    case TreeEnumProduct::kUnitPrice:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toDouble(), kUnitPrice, &Node::first);
        break;
    default:
        return false;
    }

    emit SResizeColumnToContents(index.column());
    return true;
}

Qt::ItemFlags TreeModelProduct::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const TreeEnumProduct kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumProduct::kName:
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        flags &= ~Qt::ItemIsEditable;
        break;
    case TreeEnumProduct::kQuantity:
    case TreeEnumProduct::kAmount:
    case TreeEnumProduct::kColor:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}

bool TreeModelProduct::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
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
        TreeModelUtils::UpdateAncestorValueFPT(root_, node, -node->initial_total, -node->final_total);

        destination_parent->children.insert(begin_row, node);
        node->parent = destination_parent;
        TreeModelUtils::UpdateAncestorValueFPT(root_, node, node->initial_total, node->final_total);

        endMoveRows();
    }

    sql_->DragNode(destination_parent->id, node_id);
    TreeModelUtils::UpdatePathFPTS(leaf_path_, branch_path_, support_path_, root_, node, separator_);
    TreeModelUtils::UpdateModel(leaf_path_, leaf_model_, support_path_, support_model_, node);
    TreeModelUtils::UpdateUnitModel(leaf_path_, product_model_, node, kUnitPos, Filter::kExcludeSpecific);

    emit SUpdateName(node_id, node->name, node->type == kTypeBranch);
    emit SResizeColumnToContents(std::to_underlying(TreeEnum::kName));

    return true;
}
