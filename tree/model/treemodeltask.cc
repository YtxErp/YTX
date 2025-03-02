#include "treemodeltask.h"

#include "global/resourcepool.h"

TreeModelTask::TreeModelTask(Sqlite* sql, CInfo& info, int default_unit, CTableHash& table_hash, CString& separator, QObject* parent)
    : TreeModel(sql, info, default_unit, table_hash, separator, parent)
{
    leaf_model_ = new QStandardItemModel(this);
    ConstructTree();
}

TreeModelTask::~TreeModelTask() { qDeleteAll(node_hash_); }

void TreeModelTask::RUpdateLeafValue(
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

void TreeModelTask::RUpdateMultiLeafTotal(const QList<int>& node_list)
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

void TreeModelTask::RSyncDouble(int node_id, int column, double value)
{
    if (column != std::to_underlying(TreeEnumTask::kUnitCost) || node_id <= 0 || value == 0.0)
        return;

    auto* node { node_hash_.value(node_id) };
    if (!node || node == root_ || node->type != kTypeLeaf || node->unit != std::to_underlying(UnitTask::kProd))
        return;

    node->first += value;
    sql_->WriteField(info_.node, kUnitCost, node->first, node_id);
}

QVariant TreeModelTask::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* node { GetNodeByIndex(index) };
    if (node == root_)
        return QVariant();

    const TreeEnumTask kColumn { index.column() };
    const bool kIsLeaf { node->type == kTypeLeaf };

    switch (kColumn) {
    case TreeEnumTask::kName:
        return node->name;
    case TreeEnumTask::kID:
        return node->id;
    case TreeEnumTask::kCode:
        return node->code;
    case TreeEnumTask::kDescription:
        return node->description;
    case TreeEnumTask::kNote:
        return node->note;
    case TreeEnumTask::kRule:
        return node->rule;
    case TreeEnumTask::kType:
        return node->type;
    case TreeEnumTask::kUnit:
        return node->unit;
    case TreeEnumTask::kColor:
        return node->color;
    case TreeEnumTask::kDateTime:
        return kIsLeaf ? node->date_time : QVariant();
    case TreeEnumTask::kFinished:
        return kIsLeaf && node->finished ? node->finished : QVariant();
    case TreeEnumTask::kUnitCost:
        return kIsLeaf && node->first != 0 ? node->first : QVariant();
    case TreeEnumTask::kDocument:
        return node->document.isEmpty() ? QVariant() : node->document.size();
    case TreeEnumTask::kQuantity:
        return node->initial_total;
    case TreeEnumTask::kAmount:
        return node->final_total;
    default:
        return QVariant();
    }
}

bool TreeModelTask::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    auto* node { GetNodeByIndex(index) };
    if (node == root_)
        return false;

    const TreeEnumTask kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumTask::kCode:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kCode, &Node::code);
        break;
    case TreeEnumTask::kDescription:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kDescription, &Node::description);
        break;
    case TreeEnumTask::kNote:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kNote, &Node::note);
        break;
    case TreeEnumTask::kRule:
        UpdateRuleFPTO(node, value.toBool());
        break;
    case TreeEnumTask::kType:
        UpdateTypeFPTS(node, value.toInt());
        break;
    case TreeEnumTask::kColor:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kColor, &Node::color, true);
        break;
    case TreeEnumTask::kDateTime:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toString(), kDateTime, &Node::date_time, true);
        break;
    case TreeEnumTask::kUnit:
        UpdateUnit(node, value.toInt());
        break;
    case TreeEnumTask::kFinished:
        TreeModelUtils::UpdateField(sql_, node, info_.node, value.toBool(), kFinished, &Node::finished, true);
        break;
    default:
        return false;
    }

    emit SResizeColumnToContents(index.column());
    return true;
}

void TreeModelTask::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.node_header.size())
        return;

    auto Compare = [column, order](const Node* lhs, const Node* rhs) -> bool {
        const TreeEnumTask kColumn { column };
        switch (kColumn) {
        case TreeEnumTask::kName:
            return (order == Qt::AscendingOrder) ? (lhs->name < rhs->name) : (lhs->name > rhs->name);
        case TreeEnumTask::kCode:
            return (order == Qt::AscendingOrder) ? (lhs->code < rhs->code) : (lhs->code > rhs->code);
        case TreeEnumTask::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case TreeEnumTask::kNote:
            return (order == Qt::AscendingOrder) ? (lhs->note < rhs->note) : (lhs->note > rhs->note);
        case TreeEnumTask::kRule:
            return (order == Qt::AscendingOrder) ? (lhs->rule < rhs->rule) : (lhs->rule > rhs->rule);
        case TreeEnumTask::kType:
            return (order == Qt::AscendingOrder) ? (lhs->type < rhs->type) : (lhs->type > rhs->type);
        case TreeEnumTask::kFinished:
            return (order == Qt::AscendingOrder) ? (lhs->finished < rhs->finished) : (lhs->finished > rhs->finished);
        case TreeEnumTask::kUnit:
            return (order == Qt::AscendingOrder) ? (lhs->unit < rhs->unit) : (lhs->unit > rhs->unit);
        case TreeEnumTask::kColor:
            return (order == Qt::AscendingOrder) ? (lhs->color < rhs->color) : (lhs->color > rhs->color);
        case TreeEnumTask::kDocument:
            return (order == Qt::AscendingOrder) ? (lhs->document.size() < rhs->document.size()) : (lhs->document.size() > rhs->document.size());
        case TreeEnumTask::kDateTime:
            return (order == Qt::AscendingOrder) ? (lhs->date_time < rhs->date_time) : (lhs->date_time > rhs->date_time);
        case TreeEnumTask::kUnitCost:
            return (order == Qt::AscendingOrder) ? (lhs->first < rhs->first) : (lhs->first > rhs->first);
        case TreeEnumTask::kQuantity:
            return (order == Qt::AscendingOrder) ? (lhs->initial_total < rhs->initial_total) : (lhs->initial_total > rhs->initial_total);
        case TreeEnumTask::kAmount:
            return (order == Qt::AscendingOrder) ? (lhs->final_total < rhs->final_total) : (lhs->final_total > rhs->final_total);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    TreeModelUtils::SortIterative(root_, Compare);
    emit layoutChanged();
}

Qt::ItemFlags TreeModelTask::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };

    const TreeEnumTask kColumn { index.column() };
    switch (kColumn) {
    case TreeEnumTask::kName:
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        break;
    case TreeEnumTask::kDescription:
    case TreeEnumTask::kCode:
    case TreeEnumTask::kNote:
    case TreeEnumTask::kType:
    case TreeEnumTask::kRule:
    case TreeEnumTask::kUnit:
    case TreeEnumTask::kDateTime:
        flags |= Qt::ItemIsEditable;
        break;
    default:
        break;
    }

    const bool finished { index.siblingAtColumn(std::to_underlying(TreeEnumTask::kFinished)).data().toBool() };
    if (finished)
        flags &= ~Qt::ItemIsEditable;

    return flags;
}

bool TreeModelTask::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
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

    emit SUpdateName(node_id, node->name, node->type == kTypeBranch);
    emit SResizeColumnToContents(std::to_underlying(TreeEnum::kName));

    return true;
}

bool TreeModelTask::RemoveNode(int row, const QModelIndex& parent)
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

        TreeModelUtils::UpdatePathFPTS(leaf_path_, branch_path_, support_path_, root_, node, separator_);
        TreeModelUtils::UpdateModel(leaf_path_, leaf_model_, support_path_, support_model_, node);

        branch_path_.remove(node_id);
        emit SUpdateName(node_id, node->name, true);

    } break;
    case kTypeLeaf: {
        UpdateAncestorValue(node, -node->initial_total, -node->final_total);
        TreeModelUtils::RemoveItemFromModel(leaf_model_, node_id);
        leaf_path_.remove(node_id);
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

bool TreeModelTask::InsertNode(int row, const QModelIndex& parent, Node* node)
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
    case kTypeLeaf:
        TreeModelUtils::AddItemToModel(leaf_model_, path, node->id);
        leaf_path_.insert(node->id, path);
        break;
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

bool TreeModelTask::UpdateUnit(Node* node, int value)
{
    if (node->unit == value)
        return false;

    const int node_id { node->id };
    QString message { tr("Cannot change %1 unit,").arg(GetPath(node_id)) };

    if (TreeModelUtils::HasChildrenFPTS(node, message))
        return false;

    if (TreeModelUtils::IsInternalReferencedFPTS(sql_, node_id, message))
        return false;

    if (TreeModelUtils::IsSupportReferencedFPTS(sql_, node_id, message))
        return false;

    node->unit = value;
    sql_->WriteField(info_.node, kUnit, value, node_id);

    return true;
}

bool TreeModelTask::UpdateAncestorValue(
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

void TreeModelTask::ConstructTree()
{
    sql_->ReadNode(node_hash_);
    const auto& const_node_hash { std::as_const(node_hash_) };

    for (auto* node : const_node_hash) {
        if (!node->parent) {
            node->parent = root_;
            root_->children.emplace_back(node);
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
}
