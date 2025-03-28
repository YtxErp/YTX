/*
 * Copyright (C) 2023 YTX
 *
 * This file is part of YTX.
 *
 * YTX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YTX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with YTX. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef NODEMODELUTILS_H
#define NODEMODELUTILS_H

#include <QStandardItemModel>

#include "component/using.h"
#include "tree/node.h"
#include "widget/trans/transwidget.h"

class NodeModelUtils {
public:
    template <typename T>
    static bool UpdateField(Sqlite* sql, Node* node, CString& table, CString& field, const T& value, T Node::* member, bool allow_leaf_only = false)
    {
        assert(sql && "Sqlite pointer is null");
        assert(node && "Node pointer is null");

        if (allow_leaf_only && node->type != kTypeLeaf) {
            return false;
        }

        T& current_value { std::invoke(member, node) };

        if constexpr (std::is_floating_point_v<T>) {
            if (std::abs(current_value - value) < kTolerance)
                return false;
        } else {
            if (current_value == value)
                return false;
        }

        current_value = value;
        sql->WriteField(table, field, value, node->id);

        return true;
    }

    template <typename T> static const T& Value(CNodeHash& hash, int node_id, T Node::* member)
    {
        if (auto it = hash.constFind(node_id); it != hash.constEnd())
            return std::invoke(member, *(it.value()));

        // If the node_id does not exist, return a static empty object to ensure a safe default value
        // Examples:
        // double InitialTotal(int node_id) const { return GetValue(node_id, &Node::initial_total); }
        // double FinalTotal(int node_id) const { return GetValue(node_id, &Node::final_total); }
        // Note: In the SetStatus() function of TreeWidget,
        // a node_id of 0 may be passed, so empty{} is needed to prevent illegal access
        static const T empty {};
        return empty;
    }

    static void InitializeRoot(Node*& root, int default_unit);

    static Node* GetNode(CNodeHash& hash, int node_id);
    static bool IsDescendant(const Node* lhs, const Node* rhs);

    static void SortIterative(Node* node, std::function<bool(const Node*, const Node*)> Compare);

    static QString ConstructPath(const Node* root, const Node* node, CString& separator);
    static void UpdatePath(StringHash& leaf, StringHash& branch, StringHash& support, const Node* root, const Node* node, CString& separator);

    static void LeafPathBranchPathModel(CStringHash& leaf, CStringHash& branch, QStandardItemModel* model);

    static void AppendItem(QStandardItemModel* model, int node_id, CString& path);
    static void RemoveItem(QStandardItemModel* model, int node_id);

    static void UpdateModel(CStringHash& leaf, QStandardItemModel* leaf_model, CStringHash& support, QStandardItemModel* support_model, const Node* node);
    static void UpdatePathSeparator(CString& old_separator, CString& new_separator, StringHash& source_path);
    static void UpdateModelSeparator(QStandardItemModel* model, CStringHash& source_path);

    static bool HasChildren(Node* node, CString& message);
    static bool IsOpened(CTransWgtHash& hash, int node_id, CString& message);

    static void UpdateBranchUnit(const Node* root, Node* node);

    static bool IsInternalReferenced(Sqlite* sql, int node_id, CString& message);
    static bool IsSupportReferenced(Sqlite* sql, int node_id, CString& message);
    static bool IsExternalReferenced(Sqlite* sql, int node_id, CString& message);

private:
    static void UpdateModelFunction(QStandardItemModel* model, CIntSet& update_range, CStringHash& source_path);
    static void UpdateComboModel(QStandardItemModel* model, const QVector<std::pair<int, QString>>& items);
};

#endif // NODEMODELUTILS_H
