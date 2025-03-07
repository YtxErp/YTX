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

#ifndef NODEMODEL_H
#define NODEMODEL_H

#include <QAbstractItemModel>
#include <QMimeData>

#include "component/constvalue.h"
#include "component/enumclass.h"
#include "nodemodelutils.h"

class NodeModel : public QAbstractItemModel {
    Q_OBJECT

public:
    virtual ~NodeModel();
    NodeModel() = delete;

protected:
    explicit NodeModel(Sqlite* sql, CInfo& info, int default_unit, CTransWgtHash& leaf_wgt_hash, CString& separator, QObject* parent = nullptr);

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

    // send to TreeModelStakeholder
    void SSyncDouble(int node_id, int column, double value);

    // send to TableWidgetOrder and EditNodeOrder
    void SSyncBool(int node_id, int column, bool value);
    void SSyncInt(int node_id, int column, int value);
    void SSyncString(int node_id, int column, const QString& value);

public slots:
    // receive from Sqlite
    void RRemoveNode(int node_id);
    virtual void RUpdateMultiLeafTotal(const QList<int>& /*node_list*/) { }

    // receive from  TableModel
    void RSearch() { emit SSearch(); }

    // receive from TableWidgetOrder and EditNodeOrder
    virtual void RSyncBool(int node_id, int column, bool value)
    {
        Q_UNUSED(node_id);
        Q_UNUSED(column);
        Q_UNUSED(value);
    }

    virtual void RUpdateLeafValue(int node_id, double delta1, double delta2, double delta3, double delta4, double delta5)
    {
        Q_UNUSED(node_id);
        Q_UNUSED(delta1);
        Q_UNUSED(delta2);
        Q_UNUSED(delta3);
        Q_UNUSED(delta4);
        Q_UNUSED(delta5);
    }

    // receive from TreeModelOrder
    virtual void RSyncDouble(int node_id, int column, double value)
    {
        Q_UNUSED(node_id);
        Q_UNUSED(column);
        Q_UNUSED(value);
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
    double InitialTotalFPT(int node_id) const { return NodeModelUtils::GetValue(node_hash_, node_id, &Node::initial_total); }
    double FinalTotalFPT(int node_id) const { return NodeModelUtils::GetValue(node_hash_, node_id, &Node::final_total); }
    int Type(int node_id) { return NodeModelUtils::GetValue(node_hash_, node_id, &Node::type); }
    int Unit(int node_id) const { return NodeModelUtils::GetValue(node_hash_, node_id, &Node::unit); }
    QString Name(int node_id) const { return NodeModelUtils::GetValue(node_hash_, node_id, &Node::name); }
    bool Rule(int node_id) const { return NodeModelUtils::GetValue(node_hash_, node_id, &Node::rule); }
    QStringList* GetDocumentPointer(int node_id) const;

    bool ChildrenEmpty(int node_id) const;
    bool Contains(int node_id) const { return node_hash_.contains(node_id); }
    QStandardItemModel* SupportModel() const { return support_model_; }
    QStandardItemModel* LeafModel() const { return leaf_model_; }

    void CopyNodeFPTS(Node* tmp_node, int node_id) const;
    QStringList ChildrenNameFPTS(int node_id) const;
    QSet<int> ChildrenIDFPTS(int node_id) const;

    void LeafPathBranchPathModelFPT(QStandardItemModel* model) const;
    void LeafPathFilterModelFPTS(QStandardItemModel* model, int specific_unit, int exclude_node) const;
    void SupportPathFilterModelFPTS(QStandardItemModel* model, int specific_node, Filter filter) const;

    void SearchNodeFPTS(QList<const Node*>& node_list, const QList<int>& node_id_list) const;

    void SetParent(Node* node, int parent_id) const;
    QModelIndex GetIndex(int node_id) const;

    void UpdateName(int node_id, CString& new_name);

    // virtual functions
    virtual void RetrieveNode(int node_id) { Q_UNUSED(node_id); }

    virtual void UpdateSeparatorFPTS(CString& old_separator, CString& new_separator);
    virtual QStandardItemModel* UnitModelPS(int unit = 0) const
    {
        Q_UNUSED(unit);
        return nullptr;
    }

    virtual Node* GetNodeO(int node_id) const
    {
        Q_UNUSED(node_id);
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
    virtual bool UpdateNameFunction(Node* node, CString& value);
    virtual bool UpdateRuleFPTO(Node* node, bool value);

    virtual void ConstructTree() = 0;
    virtual bool UpdateUnit(Node* node, int value) = 0;
    virtual bool UpdateAncestorValue(Node* node, double initial_delta, double final_delta, double first_delta, double second_delta, double discount_delta) = 0;

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
    CTransWgtHash& leaf_wgt_hash_;
    CString& separator_;
};

using PTreeModel = QPointer<NodeModel>;
using CTreeModel = const NodeModel;

#endif // NODEMODEL_H
