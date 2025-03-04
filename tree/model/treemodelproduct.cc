#include "treemodelproduct.h"

#include "global/resourcepool.h"

TreeModelProduct::TreeModelProduct(Sqlite* sql, CInfo& info, int default_unit, CLeafWgtHash& leaf_wgt_hash, CString& separator, QObject* parent)
    : TreeModel(sql, info, default_unit, leaf_wgt_hash, separator, parent)
{
    leaf_model_ = new QStandardItemModel(this);
    product_model_ = new QStandardItemModel(this);

    ConstructTree();
}

TreeModelProduct::~TreeModelProduct() { qDeleteAll(node_hash_); }

void TreeModelProduct::RUpdateLeafValue(
    int node_id, double initial_debit_delta, double initial_credit_delta, double final_debit_delta, double final_credit_delta, double /*settled_delta*/)
{
    auto* node { TreeModelUtils::GetNodeByID(node_hash_, node_id) };
    if (!node || node == root_ || node->type != kTypeLeaf)
        return;

    if (initial_credit_delta == 0.0 && initial_debit_delta == 0.0 && final_debit_delta == 0.0 && final_credit_delta == 0.0)
        return;

    bool rule { node->rule };

    double initial_delta { (rule ? 1 : -1) * (initial_credit_delta - initial_debit_delta) };
    double final_delta { (rule ? 1 : -1) * (final_credit_delta - final_debit_delta) };

    node->initial_total += initial_delta;
    node->final_total += final_delta;

    sql_->WriteLeafValue(node);
    UpdateAncestorValue(node, initial_delta, final_delta);
    emit SUpdateStatusValue();
}

void TreeModelProduct::RUpdateMultiLeafTotal(const QList<int>& node_list)
{
    double old_final_total {};
    double old_initial_total {};
    double final_delta {};
    double initial_delta {};
    Node* node {};

    for (int node_id : node_list) {
        node = TreeModelUtils::GetNodeByID(node_hash_, node_id);

        if (!node || node->type != kTypeLeaf)
            continue;

        old_final_total = node->final_total;
        old_initial_total = node->initial_total;

        sql_->ReadLeafTotal(node);
        sql_->WriteLeafValue(node);

        final_delta = node->final_total - old_final_total;
        initial_delta = node->initial_total - old_initial_total;

        UpdateAncestorValue(node, initial_delta, final_delta);
    }

    emit SUpdateStatusValue();
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
        for (auto* child : std::as_const(node->children)) {
            child->parent = parent_node;
            parent_node->children.emplace_back(child);
        }

        TreeModelUtils::UpdatePathFPTS(leaf_path_, branch_path_, support_path_, root_, node, separator_);
        TreeModelUtils::UpdateModel(leaf_path_, leaf_model_, support_path_, support_model_, node);

        branch_path_.remove(node_id);
        emit SUpdateName(node_id, node->name, true);

    } break;
    case kTypeLeaf: {
        UpdateAncestorValue(node, -node->initial_total, -node->final_total);
        TreeModelUtils::RemoveItemFromModel(leaf_model_, node_id);
        leaf_path_.remove(node_id);

        if (node->unit != std::to_underlying(UnitP::kPos)) {
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

    ResourcePool<Node>::Instance().Recycle(node);
    node_hash_.remove(node_id);

    emit SSearch();
    emit SResizeColumnToContents(std::to_underlying(TreeEnum::kName));
    emit SUpdateStatusValue();

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

        if (node->unit != std::to_underlying(UnitP::kPos)) {
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
    sql_->WriteField(info_.node, kUnit, value, node_id);

    if (value == std::to_underlying(UnitP::kPos))
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

        if (node->unit != std::to_underlying(UnitP::kPos))
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
            UpdateAncestorValue(node, node->initial_total, node->final_total);
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

bool TreeModelProduct::UpdateNameFunction(Node* node, CString& value)
{
    node->name = value;
    sql_->WriteField(info_.node, kName, value, node->id);

    TreeModelUtils::UpdatePathFPTS(leaf_path_, branch_path_, support_path_, root_, node, separator_);
    TreeModelUtils::UpdateModel(leaf_path_, leaf_model_, support_path_, support_model_, node);
    TreeModelUtils::UpdateUnitModel(leaf_path_, product_model_, node, std::to_underlying(UnitP::kPos), Filter::kExcludeSpecific);

    emit SResizeColumnToContents(std::to_underlying(TreeEnum::kName));
    emit SSearch();
    return true;
}

bool TreeModelProduct::UpdateAncestorValue(
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

void TreeModelProduct::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.node_header.size())
        return;

    auto Compare = [column, order](const Node* lhs, const Node* rhs) -> bool {
        const TreeEnumP kColumn { column };
        switch (kColumn) {
        case TreeEnumP::kName:
            return (order == Qt::AscendingOrder) ? (lhs->name < rhs->name) : (lhs->name > rhs->name);
        case TreeEnumP::kCode:
            return (order == Qt::AscendingOrder) ? (lhs->code < rhs->code) : (lhs->code > rhs->code);
        case TreeEnumP::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case TreeEnumP::kNote:
            return (order == Qt::AscendingOrder) ? (lhs->note < rhs->note) : (lhs->note > rhs->note);
        case TreeEnumP::kRule:
            return (order == Qt::AscendingOrder) ? (lhs->rule < rhs->rule) : (lhs->rule > rhs->rule);
        case TreeEnumP::kType:
            return (order == Qt::AscendingOrder) ? (lhs->type < rhs->type) : (lhs->type > rhs->type);
        case TreeEnumP::kUnit:
            return (order == Qt::AscendingOrder) ? (lhs->unit < rhs->unit) : (lhs->unit > rhs->unit);
        case TreeEnumP::kColor:
            return (order == Qt::AscendingOrder) ? (lhs->color < rhs->color) : (lhs->color > rhs->color);
        case TreeEnumP::kCommission:
            return (order == Qt::AscendingOrder) ? (lhs->second < rhs->second) : (lhs->second > rhs->second);
        case TreeEnumP::kUnitPrice:
            return (order == Qt::AscendingOrder) ? (lhs->first < rhs->first) : (lhs->first > rhs->first);
        case TreeEnumP::kQuantity:
            return (order == Qt::AscendingOrder) ? (lhs->initial_total < rhs->initial_total) : (lhs->initial_total > rhs->initial_total);
        case TreeEnumP::kAmount:
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

    const TreeEnumP kColumn { index.column() };
    const bool kIsLeaf { node->type == kTypeLeaf };

    switch (kColumn) {
    case TreeEnumP::kName:
        return node->name;
    case TreeEnumP::kID:
        return node->id;
    case TreeEnumP::kCode:
        return node->code;
    case TreeEnumP::kDescription:
        return node->description;
    case TreeEnumP::kNote:
        return node->note;
    case TreeEnumP::kRule:
        return node->rule;
    case TreeEnumP::kType:
        return node->type;
    case TreeEnumP::kUnit:
        return node->unit;
    case TreeEnumP::kColor:
        return node->color;
    case TreeEnumP::kCommission:
        return kIsLeaf && node->second != 0 ? node->second : QVariant();
    case TreeEnumP::kUnitPrice:
        return kIsLeaf && node->first != 0 ? node->first : QVariant();
    case TreeEnumP::kQuantity:
        return node->initial_total;
    case TreeEnumP::kAmount:
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

    const TreeEnumP kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumP::kCode:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kCode, &Node::code);
        break;
    case TreeEnumP::kDescription:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kDescription, &Node::description);
        break;
    case TreeEnumP::kNote:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kNote, &Node::note);
        break;
    case TreeEnumP::kRule:
        UpdateRuleFPTO(node, value.toBool());
        break;
    case TreeEnumP::kType:
        UpdateTypeFPTS(node, value.toInt());
        break;
    case TreeEnumP::kColor:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kColor, &Node::color, true);
        break;
    case TreeEnumP::kUnit:
        UpdateUnit(node, value.toInt());
        break;
    case TreeEnumP::kCommission:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toDouble(), kCommission, &Node::second, true);
        break;
    case TreeEnumP::kUnitPrice:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toDouble(), kUnitPrice, &Node::first, true);
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
    const TreeEnumP kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumP::kName:
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        flags &= ~Qt::ItemIsEditable;
        break;
    case TreeEnumP::kQuantity:
    case TreeEnumP::kAmount:
    case TreeEnumP::kColor:
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
        UpdateAncestorValue(node, -node->initial_total, -node->final_total);

        destination_parent->children.insert(begin_row, node);
        node->parent = destination_parent;
        UpdateAncestorValue(node, node->initial_total, node->final_total);

        endMoveRows();
    }

    sql_->DragNode(destination_parent->id, node_id);
    TreeModelUtils::UpdatePathFPTS(leaf_path_, branch_path_, support_path_, root_, node, separator_);
    TreeModelUtils::UpdateModel(leaf_path_, leaf_model_, support_path_, support_model_, node);
    TreeModelUtils::UpdateUnitModel(leaf_path_, product_model_, node, std::to_underlying(UnitP::kPos), Filter::kExcludeSpecific);

    emit SUpdateName(node_id, node->name, node->type == kTypeBranch);
    emit SResizeColumnToContents(std::to_underlying(TreeEnum::kName));

    return true;
}
