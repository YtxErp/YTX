#include "treemodel.h"

#include <QQueue>

TreeModel::TreeModel(Sqlite* sql, CInfo& info, int default_unit, CTableHash& table_hash, CString& separator, QObject* parent)
    : QAbstractItemModel(parent)
    , sql_ { sql }
    , info_ { info }
    , table_hash_ { table_hash }
    , separator_ { separator }
{
    TreeModelUtils::InitializeRoot(root_, default_unit);
    support_model_ = new QStandardItemModel(this);
}

TreeModel::~TreeModel() { delete root_; }

void TreeModel::RRemoveNode(int node_id)
{
    auto index { GetIndex(node_id) };
    int row { index.row() };
    auto parent_index { index.parent() };
    RemoveNode(row, parent_index);
}

QModelIndex TreeModel::parent(const QModelIndex& index) const
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

QModelIndex TreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    auto* parent_node { GetNodeByIndex(parent) };
    auto* node { parent_node->children.at(row) };

    return node ? createIndex(row, column, node) : QModelIndex();
}

int TreeModel::rowCount(const QModelIndex& parent) const { return GetNodeByIndex(parent)->children.size(); }

QMimeData* TreeModel::mimeData(const QModelIndexList& indexes) const
{
    auto* mime_data { new QMimeData() };
    if (indexes.isEmpty())
        return mime_data;

    auto first_index { indexes.first() };

    if (first_index.isValid()) {
        int id { first_index.sibling(first_index.row(), std::to_underlying(TreeEnum::kID)).data().toInt() };
        mime_data->setData(kNodeID, QByteArray::number(id));
    }

    return mime_data;
}

QStringList* TreeModel::GetDocumentPointer(const QModelIndex& index) const { return &GetNodeByIndex(index)->document; }

QStringList TreeModel::ChildrenNameFPTS(int node_id, int exclude_child) const
{
    auto it { node_hash_.constFind(node_id) };

    auto* node { it == node_hash_.constEnd() ? root_ : it.value() };
    QStringList list {};
    list.reserve(node->children.size());

    for (const auto* child : node->children) {
        if (child->id != exclude_child)
            list.emplaceBack(child->name);
    }

    return list;
}

void TreeModel::CopyNodeFPTS(Node* tmp_node, int node_id) const
{
    if (!tmp_node)
        return;

    auto it = node_hash_.constFind(node_id);
    if (it == node_hash_.constEnd() || !it.value())
        return;

    *tmp_node = *(it.value());
}

void TreeModel::LeafPathBranchPathModelFPT(QStandardItemModel* model) const { TreeModelUtils::LeafPathBranchPathModelFPT(leaf_path_, branch_path_, model); }

void TreeModel::LeafPathFilterModelFPTS(QStandardItemModel* model, int specific_unit, int exclude_node) const
{
    TreeModelUtils::LeafPathFilterModelFPTS(node_hash_, leaf_path_, model, specific_unit, exclude_node);
}

void TreeModel::SupportPathFilterModelFPTS(QStandardItemModel* model, int specific_node, Filter filter) const
{
    TreeModelUtils::SupportPathFilterModelFPTS(support_path_, model, specific_node, filter);
}

void TreeModel::SetNodeShadowOrder(NodeShadow* node_shadow, int node_id) const
{
    if (!node_shadow || node_id <= 0)
        return;

    auto it { node_hash_.constFind(node_id) };
    if (it != node_hash_.constEnd() && it.value())
        node_shadow->Set(it.value());
}

void TreeModel::SetNodeShadowOrder(NodeShadow* node_shadow, Node* node) const
{
    if (!node_shadow || !node)
        return;

    node_shadow->Set(node);
}

void TreeModel::UpdateSeparatorFPTS(CString& old_separator, CString& new_separator)
{
    if (old_separator == new_separator || new_separator.isEmpty())
        return;

    TreeModelUtils::UpdatePathSeparatorFPTS(old_separator, new_separator, leaf_path_);
    TreeModelUtils::UpdatePathSeparatorFPTS(old_separator, new_separator, branch_path_);
    TreeModelUtils::UpdatePathSeparatorFPTS(old_separator, new_separator, support_path_);

    TreeModelUtils::UpdateModelSeparatorFPTS(leaf_model_, leaf_path_);
    TreeModelUtils::UpdateModelSeparatorFPTS(support_model_, support_path_);
}

void TreeModel::SearchNodeFPTS(QList<const Node*>& node_list, const QList<int>& node_id_list) const
{
    node_list.reserve(node_id_list.size());

    for (int node_id : node_id_list) {
        auto it { node_hash_.constFind(node_id) };
        if (it != node_hash_.constEnd() && it.value()) {
            node_list.emplaceBack(it.value());
        }
    }
}

void TreeModel::SetParent(Node* node, int parent_id) const
{
    if (!node)
        return;

    auto it { node_hash_.constFind(parent_id) };

    node->parent = it == node_hash_.constEnd() ? root_ : it.value();
}

QModelIndex TreeModel::GetIndex(int node_id) const
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

QString TreeModel::GetPath(int node_id) const
{
    if (auto it = leaf_path_.constFind(node_id); it != leaf_path_.constEnd())
        return it.value();

    if (auto it = branch_path_.constFind(node_id); it != branch_path_.constEnd())
        return it.value();

    if (auto it = support_path_.constFind(node_id); it != support_path_.constEnd())
        return it.value();

    return {};
}

Node* TreeModel::GetNodeByIndex(const QModelIndex& index) const
{
    if (index.isValid() && index.internalPointer())
        return static_cast<Node*>(index.internalPointer());

    return root_;
}

bool TreeModel::UpdateName(Node* node, CString& value)
{
    node->name = value;
    sql_->UpdateField(info_.node, value, kName, node->id);

    TreeModelUtils::UpdatePathFPTS(leaf_path_, branch_path_, support_path_, root_, node, separator_);
    TreeModelUtils::UpdateModel(leaf_path_, leaf_model_, support_path_, support_model_, node);

    emit SResizeColumnToContents(std::to_underlying(TreeEnum::kName));
    emit SSearch();
    return true;
}

bool TreeModel::UpdateRuleFPTO(Node* node, bool value)
{
    if (node->rule == value)
        return false;

    node->rule = value;
    sql_->UpdateField(info_.node, value, kRule, node->id);

    node->final_total = -node->final_total;
    node->initial_total = -node->initial_total;
    node->first = -node->first;
    if (node->type == kTypeLeaf) {
        emit SRule(info_.section, node->id, value);
        sql_->UpdateNodeValue(node);
    }

    return true;
}

bool TreeModel::UpdateTypeFPTS(Node* node, int value)
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
        TreeModelUtils::RemoveItemFromModel(leaf_model_, node_id);
        path = leaf_path_.take(node_id);
        break;
    case kTypeSupport:
        TreeModelUtils::RemoveItemFromModel(support_model_, node_id);
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
        TreeModelUtils::AddItemToModel(leaf_model_, path, node_id);
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

bool TreeModel::ChildrenEmpty(int node_id) const
{
    auto it { node_hash_.constFind(node_id) };
    return (it == node_hash_.constEnd()) ? true : it.value()->children.isEmpty();
}

QSet<int> TreeModel::ChildrenIDFPTS(int node_id) const
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
            set.insert(queue_node->id);
            break;
        default:
            break;
        }
    }

    return set;
}
