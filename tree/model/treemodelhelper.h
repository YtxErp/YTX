#ifndef TREEMODELHELPER_H
#define TREEMODELHELPER_H

#include <QModelIndex>
#include <QStandardItemModel>
#include <QVariant>

#include "component/info.h"
#include "component/using.h"
#include "tree/node.h"
#include "widget/tablewidget/tablewidget.h"

class TreeModelHelper {
public:
    // Qt's public
    static QVariant headerData(const Info& info, int section, Qt::Orientation orientation, int role)
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
            return info.tree_header.at(section);
        }
        return QVariant();
    }

    static int columnCount(const Info& info) { return info.tree_header.size(); }

    // Ytx's public
    static QString GetPath(CStringHash& leaf_path, CStringHash& branch_path, int node_id);

    // Ytx's protected
    static Node* GetNodeByIndex(Node* root, const QModelIndex& index)
    {
        if (index.isValid() && index.internalPointer())
            return static_cast<Node*>(index.internalPointer());

        return root;
    }

    template <typename T> static bool UpdateField(Sqlite* sql, Node* node, CString& table, const T& value, CString& field, T Node::* member)
    {
        if constexpr (std::is_floating_point_v<T>) {
            if (std::abs(node->*member - value) < TOLERANCE)
                return false;
        } else {
            if (node->*member == value)
                return false;
        }

        node->*member = value;
        sql->UpdateField(table, value, field, node->id);
        return true;
    }

    template <typename T> static const T& GetValue(const NodeHash& node_hash, int node_id, T Node::* member)
    {
        if (auto it = node_hash.constFind(node_id); it != node_hash.constEnd())
            return it.value()->*member;

        // If the node_id does not exist, return a static empty object to ensure a safe default value
        // Examples:
        // double InitialTotal(int node_id) const { return GetValue(node_id, &Node::initial_total); }
        // double FinalTotal(int node_id) const { return GetValue(node_id, &Node::final_total); }
        // Note: In the SetStatus() function of TreeWidget,
        // a node_id of 0 may be passed, so empty{} is needed to prevent illegal access
        static const T empty {};
        return empty;
    }

    static void UpdateBranchUnit(const Node* root, Node* node);
    static void UpdatePath(StringHash& leaf_path, StringHash& branch_path, const Node* root, const Node* node, CString& separator);
    static void InitializeRoot(Node*& root, int default_unit);
    static Node* GetNodeByID(const NodeHash& node_hash, int node_id);
    static bool IsDescendant(Node* lhs, Node* rhs);
    static void SortIterative(Node* node, std::function<bool(const Node*, const Node*)> Compare);
    static QString ConstructPath(const Node* root, const Node* node, CString& separator);
    static void ShowTemporaryTooltip(CString& message, int duration);
    static bool HasChildren(Node* node, CString& message);
    static bool IsOpened(CTableHash& table_hash, int node_id, CString& message);
    static void SearchNode(const NodeHash& node_hash, QList<const Node*>& node_list, const QList<int>& node_id_list);
    static bool ChildrenEmpty(const NodeHash& node_hash, int node_id);
    static bool Contains(const NodeHash& node_hash, int node_id);
    static void UpdateSeparator(StringHash& leaf_path, StringHash& branch_path, CString& old_separator, CString& new_separator);
    static void CopyNode(const NodeHash& node_hash, Node* tmp_node, int node_id);
    static void SetParent(const NodeHash& node_hash, Node* node, int parent_id);
    static QStringList ChildrenName(const NodeHash& node_hash, int node_id, int exclude_child);
    static void UpdateComboModel(QStandardItemModel* combo_model, const QVector<std::pair<QString, int>>& items);
};

#endif // TREEMODELHELPER_H
