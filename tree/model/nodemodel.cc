#include "nodemodel.h"

#include <QQueue>
#include <QtConcurrent>

#include "global/resourcepool.h"

NodeModel::NodeModel(CNodeModelArg& arg, QObject* parent)
    : QAbstractItemModel(parent)
    , sql_ { arg.sql }
    , info_ { arg.info }
    , leaf_wgt_hash_ { arg.leaf_wgt_hash }
    , separator_ { arg.separator }
{
    NodeModelUtils::InitializeRoot(root_, arg.default_unit);
}

NodeModel::~NodeModel() { delete root_; }

void NodeModel::RRemoveNode(int node_id)
{
    auto index { GetIndex(node_id) };
    int row { index.row() };
    auto parent_index { index.parent() };
    RemoveNode(row, parent_index);
}

QModelIndex NodeModel::parent(const QModelIndex& index) const
{
    // root_'s index is QModelIndex(), root_'s id == -1
    if (!index.isValid())
        return QModelIndex();

    auto* node { GetNodeByIndex(index) };
    if (node->id == -1)
        return QModelIndex();

    auto* parent_node { node->parent };
    if (parent_node->id == -1)
        return QModelIndex();

    return createIndex(parent_node->parent->children.indexOf(parent_node), 0, parent_node);
}

QModelIndex NodeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    auto* parent_node { GetNodeByIndex(parent) };
    auto* node { parent_node->children.at(row) };

    return node ? createIndex(row, column, node) : QModelIndex();
}

int NodeModel::rowCount(const QModelIndex& parent) const { return GetNodeByIndex(parent)->children.size(); }

QMimeData* NodeModel::mimeData(const QModelIndexList& indexes) const
{
    auto* mime_data { new QMimeData() };
    if (indexes.isEmpty())
        return mime_data;

    auto first_index { indexes.first() };

    if (first_index.isValid()) {
        int id { first_index.sibling(first_index.row(), std::to_underlying(NodeEnum::kID)).data().toInt() };
        mime_data->setData(kNodeID, QByteArray::number(id));
    }

    return mime_data;
}

QStringList* NodeModel::DocumentPointer(int node_id) const
{
    auto it { node_hash_.constFind(node_id) };
    if (it == node_hash_.constEnd())
        return nullptr;

    return &it.value()->document;
}

QStringList NodeModel::ChildrenName(int node_id) const
{
    auto it { node_hash_.constFind(node_id) };

    auto* node { it == node_hash_.constEnd() ? root_ : it.value() };

    QStringList list {};
    list.reserve(node->children.size());

    for (const auto* child : std::as_const(node->children)) {
        list.emplaceBack(child->name);
    }

    return list;
}

void NodeModel::LeafPathBranchPathModel(QStandardItemModel* model) const { NodeModelUtils::LeafPathBranchPathModel(leaf_path_, branch_path_, model); }

void NodeModel::UpdateSeparator(CString& old_separator, CString& new_separator)
{
    if (old_separator == new_separator || new_separator.isEmpty())
        return;

    NodeModelUtils::UpdatePathSeparator(old_separator, new_separator, leaf_path_);
    NodeModelUtils::UpdatePathSeparator(old_separator, new_separator, branch_path_);
    NodeModelUtils::UpdatePathSeparator(old_separator, new_separator, support_path_);

    NodeModelUtils::UpdateModelSeparator(leaf_model_, leaf_path_);
    NodeModelUtils::UpdateModelSeparator(support_model_, support_path_);
}

void NodeModel::SearchNode(QList<const Node*>& node_list, const QList<int>& node_id_list) const
{
    node_list.reserve(node_id_list.size());

    for (int node_id : node_id_list) {
        auto it { node_hash_.constFind(node_id) };
        if (it != node_hash_.constEnd() && it.value()) {
            node_list.emplaceBack(it.value());
        }
    }
}

void NodeModel::SetParent(Node* node, int parent_id) const
{
    if (!node)
        return;

    auto it { node_hash_.constFind(parent_id) };

    node->parent = it == node_hash_.constEnd() ? root_ : it.value();
}

QModelIndex NodeModel::GetIndex(int node_id) const
{
    if (node_id == -1)
        return QModelIndex();

    auto it = node_hash_.constFind(node_id);
    if (it == node_hash_.constEnd() || !it.value())
        return QModelIndex();

    const Node* node { it.value() };

    if (!node->parent)
        return QModelIndex();

    auto row { node->parent->children.indexOf(node) };
    if (row == -1)
        return QModelIndex();

    return createIndex(row, 0, node);
}

void NodeModel::UpdateName(int node_id, CString& new_name)
{
    auto it { node_hash_.constFind(node_id) };
    if (it == node_hash_.constEnd())
        return;

    auto* node { it.value() };

    UpdateNameFunction(node, new_name);
    emit SUpdateName(node->id, node->name, node->type == kTypeBranch);
}

QString NodeModel::Path(int node_id) const
{
    if (auto it = leaf_path_.constFind(node_id); it != leaf_path_.constEnd())
        return it.value();

    if (auto it = branch_path_.constFind(node_id); it != branch_path_.constEnd())
        return it.value();

    if (auto it = support_path_.constFind(node_id); it != support_path_.constEnd())
        return it.value();

    return {};
}

bool NodeModel::InsertNode(int row, const QModelIndex& parent, Node* node)
{
    if (row <= -1)
        return false;

    auto* parent_node { GetNodeByIndex(parent) };

    beginInsertRows(parent, row, row);
    parent_node->children.insert(row, node);
    endInsertRows();

    sql_->WriteNode(parent_node->id, node);
    node_hash_.insert(node->id, node);

    InsertPath(node);
    SortModel(node->type);

    emit SSearch();
    return true;
}

bool NodeModel::RemoveNode(int row, const QModelIndex& parent)
{
    if (row <= -1 || row >= rowCount(parent))
        return false;

    auto* parent_node { GetNodeByIndex(parent) };
    auto* node { parent_node->children.at(row) };

    const int node_id { node->id };

    beginRemoveRows(parent, row, row);
    parent_node->children.removeOne(node);
    endRemoveRows();

    RemovePath(node, parent_node);

    ResourcePool<Node>::Instance().Recycle(node);
    node_hash_.remove(node_id);

    emit SSearch();
    emit SResizeColumnToContents(std::to_underlying(NodeEnum::kName));
    emit SUpdateStatusValue();

    return true;
}

void NodeModel::RemovePath(Node* node, Node* parent_node)
{
    const int node_id { node->id };

    switch (node->type) {
    case kTypeBranch: {
        for (auto* child : std::as_const(node->children)) {
            child->parent = parent_node;
            parent_node->children.emplace_back(child);
        }

        NodeModelUtils::UpdatePath(leaf_path_, branch_path_, support_path_, root_, node, separator_);
        NodeModelUtils::UpdateModel(leaf_path_, leaf_model_, support_path_, support_model_, node);

        branch_path_.remove(node_id);
        emit SUpdateName(node_id, node->name, true);

    } break;
    case kTypeLeaf: {
        leaf_path_.remove(node_id);
        UpdateAncestorValue(node, -node->initial_total, -node->final_total, -node->first, -node->second, -node->discount);
        RemovePathLeaf(node_id, node->unit);
    } break;
    case kTypeSupport: {
        support_path_.remove(node_id);
        NodeModelUtils::RemoveItem(support_model_, node_id);
    } break;
    default:
        break;
    }
}

Node* NodeModel::GetNodeByIndex(const QModelIndex& index) const
{
    if (index.isValid() && index.internalPointer())
        return static_cast<Node*>(index.internalPointer());

    return root_;
}

bool NodeModel::UpdateNameFunction(Node* node, CString& value)
{
    node->name = value;
    sql_->WriteField(info_.node, kName, value, node->id);

    NodeModelUtils::UpdatePath(leaf_path_, branch_path_, support_path_, root_, node, separator_);
    NodeModelUtils::UpdateModel(leaf_path_, leaf_model_, support_path_, support_model_, node);

    emit SResizeColumnToContents(std::to_underlying(NodeEnum::kName));
    emit SSearch();
    return true;
}

void NodeModel::ConstructTree()
{
    sql_->ReadNode(node_hash_);
    const auto& const_node_hash { std::as_const(node_hash_) };

    for (auto* node : const_node_hash) {
        if (!node->parent) {
            node->parent = root_;
            root_->children.emplace_back(node);
        }
    }

    auto* watcher { new QFutureWatcher<void>(this) };

    QFuture<void> future = QtConcurrent::run([this, &const_node_hash]() {
        for (auto* node : const_node_hash) {
            InsertPath(node);

            if (node->type == kTypeLeaf)
                UpdateAncestorValue(node, node->initial_total, node->final_total, node->first, node->second, node->discount);
        }
    });

    connect(watcher, &QFutureWatcher<void>::finished, this, [this, watcher]() {
        SortModel();
        watcher->deleteLater();
    });

    watcher->setFuture(future);
}

void NodeModel::SortModel()
{
    support_model_->sort(0);
    leaf_model_->sort(0);
}

void NodeModel::IniModel()
{
    leaf_model_ = new QStandardItemModel(this);
    support_model_ = new QStandardItemModel(this);

    NodeModelUtils::AppendItem(support_model_, 0, {});
}

bool NodeModel::UpdateRule(Node* node, bool value)
{
    if (node->rule == value)
        return false;

    node->rule = value;
    sql_->WriteField(info_.node, kRule, value, node->id);

    node->final_total = -node->final_total;
    node->initial_total = -node->initial_total;
    node->first = -node->first;
    if (node->type == kTypeLeaf) {
        emit SRule(info_.section, node->id, value);
        sql_->SyncLeafValue(node);
    }

    emit SUpdateStatusValue();
    return true;
}

bool NodeModel::UpdateType(Node* node, int value)
{
    if (node->type == value)
        return false;

    const int node_id { node->id };
    QString message { tr("Cannot change %1 type,").arg(Path(node_id)) };

    if (NodeModelUtils::HasChildren(node, message))
        return false;

    if (NodeModelUtils::IsOpened(leaf_wgt_hash_, node_id, message))
        return false;

    if (NodeModelUtils::IsInternalReferenced(sql_, node_id, message))
        return false;

    if (NodeModelUtils::IsExternalReferenced(sql_, node_id, message))
        return false;

    switch (node->type) {
    case kTypeBranch:
        branch_path_.remove(node_id);
        break;
    case kTypeLeaf:
        leaf_path_.remove(node_id);
        RemovePathLeaf(node_id, node->unit);
        break;
    case kTypeSupport:
        support_path_.remove(node_id);
        NodeModelUtils::RemoveItem(support_model_, node_id);
        break;
    default:
        break;
    }

    node->type = value;
    sql_->WriteField(info_.node, kType, value, node_id);

    InsertPath(node);
    SortModel(node->type);
    return true;
}

void NodeModel::SortModel(int type)
{
    switch (type) {
    case kTypeLeaf:
        if (leaf_model_ != nullptr) {
            leaf_model_->sort(0);
        }
        break;
    case kTypeSupport:
        if (support_model_ != nullptr) {
            support_model_->sort(0);
        }
        break;
    default:
        break;
    }
}

void NodeModel::RemovePathLeaf(int node_id, int /*unit*/) { NodeModelUtils::RemoveItem(leaf_model_, node_id); }

void NodeModel::InsertPath(Node* node)
{
    CString path { NodeModelUtils::ConstructPath(root_, node, separator_) };

    switch (node->type) {
    case kTypeBranch:
        branch_path_.insert(node->id, path);
        break;
    case kTypeLeaf:
        leaf_path_.insert(node->id, path);
        InsertPathLeaf(node->id, path, node->unit);
        break;
    case kTypeSupport:
        support_path_.insert(node->id, path);
        NodeModelUtils::AppendItem(support_model_, node->id, path);
        break;
    default:
        break;
    }
}

void NodeModel::InsertPathLeaf(int node_id, CString& path, int /*unit*/) { NodeModelUtils::AppendItem(leaf_model_, node_id, path); }

bool NodeModel::ChildrenEmpty(int node_id) const
{
    auto it { node_hash_.constFind(node_id) };
    return (it == node_hash_.constEnd()) ? true : it.value()->children.isEmpty();
}

QSet<int> NodeModel::ChildrenID(int node_id) const
{
    if (node_id <= 0)
        return {};

    auto it { node_hash_.constFind(node_id) };
    if (it == node_hash_.constEnd() || !it.value())
        return {};

    auto* node { it.value() };
    if (node->type != kTypeBranch || node->children.isEmpty())
        return {};

    QQueue<const Node*> queue {};
    queue.enqueue(node);

    QSet<int> set {};
    while (!queue.isEmpty()) {
        auto* queue_node = queue.dequeue();

        switch (queue_node->type) {
        case kTypeBranch: {
            for (const auto* child : queue_node->children)
                queue.enqueue(child);
        } break;
        case kTypeLeaf:
        case kTypeSupport:
            set.insert(queue_node->id);
            break;
        default:
            break;
        }
    }

    return set;
}
