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

#ifndef NODEMODEL_H
#define NODEMODEL_H

#include <QAbstractItemModel>
#include <QMimeData>
#include <QSortFilterProxyModel>

#include "component/arg/nodemodelarg.h"
#include "component/constvalue.h"
#include "component/enumclass.h"
#include "nodemodelutils.h"

class NodeModel : public QAbstractItemModel {
    Q_OBJECT

public:
    virtual ~NodeModel();
    NodeModel() = delete;

protected:
    explicit NodeModel(CNodeModelArg& arg, QObject* parent = nullptr);

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

    // send to TableWidgetOrder and InsertNodeOrder
    void SSyncBoolWD(int node_id, int column, bool value);
    void SSyncInt(int node_id, int column, int value);
    void SSyncString(int node_id, int column, const QString& value);

    // send to FilterModel
    // Inserting or removing a node will trigger it automatically; manually trigger it only when changing the unit.
    void SSyncFilterModel();

public slots:
    // receive from Sqlite
    void RRemoveNode(int node_id);
    virtual void RUpdateMultiLeafTotal(const QList<int>& node_list);

    // receive from  TableModel
    void RSearch() { emit SSearch(); }

    // receive from TableWidgetOrder and InsertNodeOrder
    virtual void RSyncBoolWD(int node_id, int column, bool value)
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
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

    inline Qt::DropActions supportedDropActions() const override { return Qt::CopyAction | Qt::MoveAction; }
    inline QStringList mimeTypes() const override { return QStringList { kNodeID }; }

    inline bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int /*row*/, int /*column*/, const QModelIndex& /*parent*/) const override
    {
        return data && data->hasFormat(kNodeID) && action != Qt::IgnoreAction;
    }

    inline int columnCount(const QModelIndex& parent = QModelIndex()) const override
    {
        Q_UNUSED(parent);
        return info_.node_header.size();
    }

    inline QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
            return info_.node_header.at(section);
        }

        return QVariant();
    }

    // Ytx's
    // Default implementations
    double InitialTotal(int node_id) const { return NodeModelUtils::Value(node_hash_, node_id, &Node::initial_total); }
    double FinalTotal(int node_id) const { return NodeModelUtils::Value(node_hash_, node_id, &Node::final_total); }
    int Type(int node_id) { return NodeModelUtils::Value(node_hash_, node_id, &Node::type); }
    int Unit(int node_id) const { return NodeModelUtils::Value(node_hash_, node_id, &Node::unit); }
    bool Rule(int node_id) const { return NodeModelUtils::Value(node_hash_, node_id, &Node::rule); }
    bool Finished(int node_id) const { return NodeModelUtils::Value(node_hash_, node_id, &Node::finished); }
    int Party(int node_id) const { return NodeModelUtils::Value(node_hash_, node_id, &Node::party); };
    int Employee(int node_id) const { return NodeModelUtils::Value(node_hash_, node_id, &Node::employee); }
    QString Name(int node_id) const { return NodeModelUtils::Value(node_hash_, node_id, &Node::name); }

    QStringList* DocumentPointer(int node_id) const;
    QStringList ChildrenName(int node_id) const;
    QSet<int> ChildrenID(int node_id) const;

    inline QStandardItemModel* SupportModel() const { return support_model_; }
    inline QStandardItemModel* LeafModel() const { return leaf_model_; }

    inline CStringHash& LeafPath() const { return leaf_path_; }

    void LeafPathBranchPathModel(QStandardItemModel* model) const;

    void SearchNode(QList<const Node*>& node_list, const QSet<int>& node_id_set) const;

    void UpdateName(int node_id, CString& new_name);
    void UpdateSeparator(CString& old_separator, CString& new_separator);

    bool InsertNode(int row, const QModelIndex& parent, Node* node);

    inline bool Contains(int node_id) const { return node_hash_.contains(node_id); }
    inline void SetParent(Node* node, int parent_id) const
    {
        assert(node && "Node must be non-null");
        auto it { node_hash_.constFind(parent_id) };
        node->parent = it == node_hash_.constEnd() ? root_ : it.value();
    }

    QModelIndex GetIndex(int node_id) const;

    // virtual functions
    virtual void ReadNode(int node_id) { Q_UNUSED(node_id); }
    virtual void UpdateDefaultUnit(int default_unit) { root_->unit = default_unit; }

    virtual QString Path(int node_id) const;
    virtual Node* GetNode(int node_id) const
    {
        Q_UNUSED(node_id)
        return nullptr;
    }

    virtual QSortFilterProxyModel* ExcludeLeafModel(int node_id);

    virtual QSortFilterProxyModel* IncludeUnitModel(int unit)
    {
        Q_UNUSED(unit)
        return nullptr;
    }

    virtual QSortFilterProxyModel* ExcludeUnitModel(int unit)
    {
        Q_UNUSED(unit)
        return nullptr;
    }

protected:
    Node* GetNodeByIndex(const QModelIndex& index) const;
    bool UpdateType(Node* node, int value);
    void SortModel(int type);
    void SortModel();
    void IniModel();

    virtual void ConstructTree();
    virtual void InsertPath(Node* node);
    virtual void RemovePath(Node* node, Node* parent_node);

    virtual const QSet<int>* UnitSet(int unit) const
    {
        Q_UNUSED(unit)
        return nullptr;
    }

    virtual void RemoveUnitSet(int node_id, int unit)
    {
        Q_UNUSED(node_id)
        Q_UNUSED(unit)
    }

    virtual void InsertUnitSet(int node_id, int unit)
    {
        Q_UNUSED(node_id)
        Q_UNUSED(unit)
    }

    virtual bool UpdateRule(Node* node, bool value);
    virtual bool UpdateUnit(Node* node, int value);
    virtual bool UpdateNameFunction(Node* node, CString& value);
    virtual bool UpdateAncestorValue(
        Node* node, double initial_delta, double final_delta, double first_delta = 0.0, double second_delta = 0.0, double discount_delta = 0.0)
        = 0;

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

using PNodeModel = QPointer<NodeModel>;
using CNodeModel = const NodeModel;

#endif // NODEMODEL_H
