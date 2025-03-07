#include "nodemodel.h"

#include <QQueue>

NodeModel::NodeModel(Sqlite* sql, CInfo& info, int default_unit, CTransWgtHash& leaf_wgt_hash, CString& separator, QObject* parent)
    : QAbstractItemModel(parent)
    , sql_ { sql }
    , info_ { info }
    , leaf_wgt_hash_ { leaf_wgt_hash }
    , separator_ { separator }
{
    NodeModelUtils::InitializeRoot(root_, default_unit);
    support_model_ = new QStandardItemModel(this);
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

QStringList* NodeModel::GetDocumentPointer(int node_id) const
{
    auto it { node_hash_.constFind(node_id) };
    if (it == node_hash_.constEnd())
        return nullptr;

    return &it.value()->document;
}

QStringList NodeModel::ChildrenNameFPTS(int node_id) const
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

void NodeModel::CopyNodeFPTS(Node* tmp_node, int node_id) const
{
    if (!tmp_node)
        return;

    auto it = node_hash_.constFind(node_id);
    if (it == node_hash_.constEnd() || !it.value())
        return;

    *tmp_node = *(it.value());
}

void NodeModel::LeafPathBranchPathModelFPT(QStandardItemModel* model) const { NodeModelUtils::LeafPathBranchPathModelFPT(leaf_path_, branch_path_, model); }

void NodeModel::LeafPathFilterModelFPTS(QStandardItemModel* model, int specific_unit, int exclude_node) const
{
    NodeModelUtils::LeafPathFilterModelFPTS(node_hash_, leaf_path_, model, specific_unit, exclude_node);
}

void NodeModel::SupportPathFilterModelFPTS(QStandardItemModel* model, int specific_node, Filter filter) const
{
    NodeModelUtils::SupportPathFilterModelFPTS(support_path_, model, specific_node, filter);
}

void NodeModel::UpdateSeparatorFPTS(CString& old_separator, CString& new_separator)
{
    if (old_separator == new_separator || new_separator.isEmpty())
        return;

    NodeModelUtils::UpdatePathSeparatorFPTS(old_separator, new_separator, leaf_path_);
    NodeModelUtils::UpdatePathSeparatorFPTS(old_separator, new_separator, branch_path_);
    NodeModelUtils::UpdatePathSeparatorFPTS(old_separator, new_separator, support_path_);

    NodeModelUtils::UpdateModelSeparatorFPTS(leaf_model_, leaf_path_);
    NodeModelUtils::UpdateModelSeparatorFPTS(support_model_, support_path_);
}

void NodeModel::SearchNodeFPTS(QList<const Node*>& node_list, const QList<int>& node_id_list) const
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

QString NodeModel::GetPath(int node_id) const
{
    if (auto it = leaf_path_.constFind(node_id); it != leaf_path_.constEnd())
        return it.value();

    if (auto it = branch_path_.constFind(node_id); it != branch_path_.constEnd())
        return it.value();

    if (auto it = support_path_.constFind(node_id); it != support_path_.constEnd())
        return it.value();

    return {};
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

    NodeModelUtils::UpdatePathFPTS(leaf_path_, branch_path_, support_path_, root_, node, separator_);
    NodeModelUtils::UpdateModel(leaf_path_, leaf_model_, support_path_, support_model_, node);

    emit SResizeColumnToContents(std::to_underlying(NodeEnum::kName));
    emit SSearch();
    return true;
}

bool NodeModel::UpdateRuleFPTO(Node* node, bool value)
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
        sql_->WriteLeafValue(node);
    }

    emit SUpdateStatusValue();
    return true;
}

bool NodeModel::UpdateTypeFPTS(Node* node, int value)
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
        NodeModelUtils::RemoveItemFromModel(leaf_model_, node_id);
        path = leaf_path_.take(node_id);
        break;
    case kTypeSupport:
        NodeModelUtils::RemoveItemFromModel(support_model_, node_id);
        path = support_path_.take(node_id);
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
        NodeModelUtils::AddItemToModel(leaf_model_, path, node_id);
        leaf_path_.insert(node_id, path);
        break;
    case kTypeSupport:
        NodeModelUtils::AddItemToModel(support_model_, path, node_id);
        support_path_.insert(node_id, path);
        break;
    default:
        break;
    }

    return true;
}

bool NodeModel::ChildrenEmpty(int node_id) const
{
    auto it { node_hash_.constFind(node_id) };
    return (it == node_hash_.constEnd()) ? true : it.value()->children.isEmpty();
}

QSet<int> NodeModel::ChildrenIDFPTS(int node_id) const
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
