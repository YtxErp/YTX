/*
 * Copyright (C) 2023 YtxErp
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

#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QAbstractItemModel>
#include <QMimeData>

#include "component/constvalue.h"
#include "component/enumclass.h"
#include "treemodelutils.h"

class TreeModel : public QAbstractItemModel {
    Q_OBJECT

public:
    virtual ~TreeModel();
    TreeModel() = delete;

protected:
    explicit TreeModel(Sqlite* sql, CInfo& info, int default_unit, CTableHash& table_hash, CString& separator, QObject* parent = nullptr);

signals:
    // send to SignalStation
    void SRule(Section seciton, int node_id, bool rule);

    // send to its view
    void SResizeColumnToContents(int column);

    // send to Search dialog
    void SSearch();

    // send to Mainwindow
    void SUpdateName(int node_id, const QString& name, bool branch);
    void SUpdateStatusValue();

public slots:
    // receive from Sqlite
    void RRemoveNode(int node_id);
    virtual void RUpdateMultiLeafTotal(const QList<int>& /*node_list*/) { }

    // receive from  TableModel
    void RSearch() { emit SSearch(); }
    virtual void RUpdateLeafValueOne(int node_id, double diff, const QString& node_field)
    {
        Q_UNUSED(node_id);
        Q_UNUSED(diff);
        Q_UNUSED(node_field);
    }
    virtual void RUpdateLeafValue(
        int node_id, double initial_debit_diff, double initial_credit_diff, double final_debit_diff, double final_credit_diff, double settled_diff)
    {
        Q_UNUSED(node_id);
        Q_UNUSED(initial_debit_diff);
        Q_UNUSED(initial_credit_diff);
        Q_UNUSED(final_debit_diff);
        Q_UNUSED(final_credit_diff);
        Q_UNUSED(settled_diff);
    }

    virtual void RUpdateStakeholder(int old_node_id, int new_node_id)
    {
        Q_UNUSED(old_node_id);
        Q_UNUSED(new_node_id);
    };

public:
    // Qt's
    // Default implementations
    QModelIndex parent(const QModelIndex& index) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    Qt::DropActions supportedDropActions() const override { return Qt::CopyAction | Qt::MoveAction; }
    QStringList mimeTypes() const override { return QStringList { kNodeID }; }

    bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int /*row*/, int /*column*/, const QModelIndex& /*parent*/) const override
    {
        return data && data->hasFormat(kNodeID) && action != Qt::IgnoreAction;
    }
    int columnCount(const QModelIndex& parent = QModelIndex()) const override
    {
        Q_UNUSED(parent);
        return info_.node_header.size();
    }
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
            return info_.node_header.at(section);
        }

        return QVariant();
    }

    // Ytx's
    // Default implementations
    double InitialTotalFPT(int node_id) const { return TreeModelUtils::GetValue(node_hash_, node_id, &Node::initial_total); }
    double FinalTotalFPT(int node_id) const { return TreeModelUtils::GetValue(node_hash_, node_id, &Node::final_total); }
    int TypeFPTS(int node_id) { return TreeModelUtils::GetValue(node_hash_, node_id, &Node::type); }
    int Unit(int node_id) const { return TreeModelUtils::GetValue(node_hash_, node_id, &Node::unit); }
    QString Name(int node_id) const { return TreeModelUtils::GetValue(node_hash_, node_id, &Node::name); }
    bool Rule(int node_id) const { return TreeModelUtils::GetValue(node_hash_, node_id, &Node::rule); }
    QStringList* GetDocumentPointer(const QModelIndex& index) const;

    bool ChildrenEmpty(int node_id) const;
    bool Contains(int node_id) const { return node_hash_.contains(node_id); }
    QStandardItemModel* SupportModel() const { return support_model_; }
    QStandardItemModel* LeafModel() const { return leaf_model_; }

    void CopyNodeFPTS(Node* tmp_node, int node_id) const;
    QStringList ChildrenNameFPTS(int node_id, int exclude_child) const;
    QSet<int> ChildrenIDFPTS(int node_id) const;

    void LeafPathBranchPathModelFPT(QStandardItemModel* model) const;
    void LeafPathFilterModelFPTS(QStandardItemModel* model, int specific_unit, int exclude_node) const;
    void SupportPathFilterModelFPTS(QStandardItemModel* model, int specific_node, Filter filter) const;

    void SetNodeShadowOrder(NodeShadow* node_shadow, int node_id) const;
    void SetNodeShadowOrder(NodeShadow* node_shadow, Node* node) const;

    void SearchNodeFPTS(QList<const Node*>& node_list, const QList<int>& node_id_list) const;

    void SetParent(Node* node, int parent_id) const;
    QModelIndex GetIndex(int node_id) const;

    // virtual functions
    virtual void UpdateNodeFPTS(const Node* tmp_node) { Q_UNUSED(tmp_node); }
    virtual void RetriveNodeOrder(int node_id) { Q_UNUSED(node_id); }

    virtual void UpdateSeparatorFPTS(CString& old_separator, CString& new_separator);
    virtual QStandardItemModel* UnitModelPS(int unit = 0) const
    {
        Q_UNUSED(unit);
        return nullptr;
    }

    virtual void UpdateDefaultUnit(int default_unit) { root_->unit = default_unit; }
    virtual QString GetPath(int node_id) const;

    // Core pure virtual functions
    virtual bool InsertNode(int row, const QModelIndex& parent, Node* node) = 0;
    virtual bool RemoveNode(int row, const QModelIndex& parent = QModelIndex()) = 0;

protected:
    Node* GetNodeByIndex(const QModelIndex& index) const;

    virtual bool UpdateTypeFPTS(Node* node, int value);
    virtual bool UpdateName(Node* node, CString& value);
    virtual bool UpdateRuleFPTO(Node* node, bool value);

    virtual void ConstructTree() = 0;
    virtual bool UpdateUnit(Node* node, int value) = 0;

protected:
    Node* root_ {};
    Sqlite* sql_ {};

    NodeHash node_hash_ {};
    StringHash leaf_path_ {};
    StringHash branch_path_ {};
    StringHash support_path_ {};

    QStandardItemModel* support_model_ {};
    QStandardItemModel* leaf_model_ {};

    CInfo& info_;
    CTableHash& table_hash_;
    CString& separator_;
};

using PTreeModel = QPointer<TreeModel>;
using CTreeModel = const TreeModel;

#endif // TREEMODEL_H
