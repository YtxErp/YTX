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

#pragma once

#include <QAbstractItemModel>
#include <QMessageBox>
#include <QMimeData>
#include <QSortFilterProxyModel>
#include <QTimer>

#include "component/constant.h"
#include "component/sectioninfo.h"
#include "tree/itemmodel.h"
#include "tree/node.h"
#include "utils/delta.h"
#include "utils/templateutils.h"

using utils::DerivedPtr;

class TreeModel : public QAbstractItemModel {
    Q_OBJECT

public:
    ~TreeModel() override;

protected:
    explicit TreeModel(CSectionInfo& info, CString& separator, QObject* parent = nullptr);

signals:
    // send to LeafSStation
    void SDirectionRule(const QUuid& node_id, bool value);

    // send to Mainwindow
    void SUpdateName(const QUuid& node_id, const QString& name, bool branch);
    void SFreeWidget(Section section, const QUuid& node_id);
    void SMessage(QMessageBox::Icon icon, const QString& message);

    // send to NodeWidget
    void SSyncValue();
    void SInitStatus();

public:
    // Qt's
    QModelIndex parent(const QModelIndex& index) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
    bool moveRows(const QModelIndex& sourceParent, int sourceRow, int count, const QModelIndex& destinationParent, int destinationChild) override;

    Qt::DropActions supportedDropActions() const override { return Qt::CopyAction | Qt::MoveAction; }
    QStringList mimeTypes() const override { return QStringList { kYTX }; }

    bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override
    {
        if (!QAbstractItemModel::canDropMimeData(data, action, row, column, parent))
            return false;

        // dropping onto an item requires it to be a branch node
        auto* node { GetNodeByIndex(parent) };
        return node && node->kind == NodeKind::kBranch;
    }

    int columnCount(const QModelIndex& parent = QModelIndex()) const override
    {
        Q_UNUSED(parent);
        return header_.size();
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
            return header_.at(section);
        }

        return QVariant();
    }

    // WebSocket's
    virtual void ApplyTree(const QJsonObject& data);
    void ApplyNode(const QUuid& ancestor, const QJsonObject& data);

    void SyncTotalArray(const QJsonArray& total_array);
    void SyncVersion(const QUuid& node_id, int version);

    void ApplyReplace(const QUuid& old_node_id, const QUuid& new_node_id);
    void ApplyDrag(const QUuid& ancestor, const QUuid& descendant);
    void ApplyDirectionRule(const QUuid& node_id, bool direction_rule, int version);
    void ApplyStatus(const QUuid& node_id, int status, int version);
    void ApplyDelete(const QUuid& node_id);
    void ApplyUpdate(const QUuid& node_id, const QJsonObject& update, int version);

    virtual void ApplyName(const QUuid& node_id, const QString& name, int version);

    // Ytx's
    double InitialTotal(QUuid node_id) const { return Value(node_id, &Node::initial_total); }
    double FinalTotal(QUuid node_id) const { return Value(node_id, &Node::final_total); }
    NodeUnit Unit(QUuid node_id) const { return Value(node_id, &Node::unit); }
    bool Rule(QUuid node_id) const { return Value(node_id, &Node::direction_rule); }
    QString Name(QUuid node_id) const { return Value(node_id, &Node::name); }
    QString Color(QUuid node_id) const { return Value(node_id, &Node::color); }
    QString Path(const QUuid& node_id) const;

    inline ItemModel* LeafModel() const { return leaf_model_; }
    inline CUuidString& LeafPath() const { return leaf_path_; }
    inline CUuidString& BranchPath() const { return branch_path_; }

    ItemModel* PathModel(QWidget* parent) const;

    void UpdateSeparator(CString& old_separator, CString& new_separator);

    void AckNode(const QUuid& node_id) const;
    void SearchName(QList<Node*>& node_list, CString& name) const;
    void SearchTag(QList<Node*>& node_list, const QSet<QString>& tag_set) const;

    void Reset();
    void FlushTimers();

    inline bool Contains(const QUuid& node_id) const { return node_hash_.contains(node_id); }
    inline Node* GetNode(const QUuid& node_id) const
    {
        auto* node { node_hash_.value(node_id, nullptr) };
        if (!node) {
            qInfo() << "Node not found for section:" << std::to_underlying(section_) << "id:" << node_id;
        }
        return node;
    }

    QModelIndex GetIndex(const QUuid& node_id) const;
    Node* GetNodeByIndex(const QModelIndex& index) const;

    QSortFilterProxyModel* ExcludeId(const QUuid& node_id, QObject* parent) const;
    QSortFilterProxyModel* IncludeUnit(NodeUnit unit, QObject* parent);
    QSortFilterProxyModel* ReplaceSelf(const QUuid& node_id, NodeUnit unit, QObject* parent);

protected:
    void RestartTimer(const QUuid& id);

    void EmitNumericChanged(const QSet<QUuid>& ids);
    void EmitColumnChanged(int column, const QSet<QUuid>& ids);
    void EmitDataChanged(int start_row, int end_row, int start_column, int end_column, const QModelIndex& parent);

    void RequestDirectionRule(Node* node, bool value);
    void RequestStatus(Node* node, int value);
    void BuildTreeData(const QJsonObject& data, QHash<QUuid, Node*>& node_hash, QHash<QUuid, QString>& leaf_path, QHash<QUuid, QString>& branch_path);

    virtual void ClearTree();
    virtual void RegisterNode(Node* node);
    virtual void UnregisterNode(Node* node, Node* parent_node);
    virtual void UpdateSubtreePath(const Node* node);

    virtual void InitTreeData(const QHash<QUuid, Node*>& node_hash, QHash<QUuid, QString>& leaf_path, QHash<QUuid, QString>& branch_path);

    virtual QSet<QUuid> UpdateAncestorTotal(Node* node, const node::Delta& delta) const;
    virtual void InitAncestorTotal(Node* node, const node::Delta& delta) const;

    virtual QSet<QUuid>* UnitSet(NodeUnit unit)
    {
        Q_UNUSED(unit)
        return nullptr;
    }
    virtual void ClearUnitSet() { }
    virtual void InitUnitSet() { }

    template <typename Field, typename T> const Field& Value(const QUuid& node_id, Field T::* member) const
    {
        if (auto it = node_hash_.constFind(node_id); it != node_hash_.constEnd()) {
            auto* derived { static_cast<T*>(it.value()) };
            return derived->*member;
        }

        // If the node_id does not exist, return a static empty object to ensure a safe default value
        // Examples:
        // double InitialTotal(QUuid node_id) const { return GetValue(node_id, &Node::initial_total); }
        // double FinalTotal(QUuid node_id) const { return GetValue(node_id, &Node::final_total); }
        // Note: In the SetStatus() function of TreeWidget,
        // a node_id of 0 may be passed, so empty{} is needed to prevent illegal access

        static const Field empty {};
        return empty;
    }

private:
    void InitRoot();
    void InitLeafData();
    void FlushTimer(const QUuid& id);

    QSet<QUuid> ExtractLeafIds(const Node* node) const;
    void SyncLeafModel(const QSet<QUuid>& leaf_ids) const;

    QSet<QUuid> SyncTotal(const QUuid& node_id, double initial_total, double final_total);

    void UnitSetRemove(const QUuid& node_id, NodeUnit unit)
    {
        if (auto* set = UnitSet(unit)) {
            set->remove(node_id);
        }
    }

    void UnitSetInsert(const QUuid& node_id, NodeUnit unit)
    {
        if (auto* set = UnitSet(unit)) {
            set->insert(node_id);
        }
    }

protected:
    Node* root_ {};
    NodeHash node_hash_ {};

    QHash<QUuid, QString> leaf_path_ {};
    QHash<QUuid, QString> branch_path_ {};

    ItemModel* leaf_model_ {};

    const Section section_ {};
    const QString& separator_;
    const QStringList& header_;

    QHash<QUuid, PendingNodeUpdate> pending_updates_ {};
};

using CTreeModel = const TreeModel;
