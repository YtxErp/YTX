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

#ifndef TREEMODELORDER_H
#define TREEMODELORDER_H

#include <QDate>

#include "database/sqlite/sqliteorder.h"
#include "tree/model/treemodel.h"

class TreeModelOrder final : public TreeModel {
    Q_OBJECT

public:
    TreeModelOrder(Sqlite* sql, CInfo& info, int default_unit, CTableHash& table_hash, CString& separator, QObject* parent = nullptr);
    ~TreeModelOrder() override = default;

signals:
    void SUpdateLeafValueOne(int node_id, double diff, const QString& node_field);

public slots:
    void RUpdateLeafValue(int node_id, double first_diff, double second_diff, double gross_amount_diff, double discount_diff, double net_amount_diff) override;
    void RUpdateStakeholder(int old_node_id, int new_node_id) override;
    void RUpdateFinished(int node_id, bool checked);

public:
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

    bool InsertNode(int row, const QModelIndex& parent, Node* node) override;
    bool RemoveNode(int row, const QModelIndex& parent = QModelIndex()) override;

    void UpdateTree(const QDate& start_date, const QDate& end_date);
    QString GetPath(int node_id) const override;
    void RetriveNodeOrder(int node_id) override;
    int Party(int node_id) const { return TreeModelUtils::GetValue(node_hash_, node_id, &Node::party); };

protected:
    bool UpdateRuleFPTO(Node* node, bool value) override; // charge = 0, refund = 1
    bool UpdateUnit(Node* node, int value) override; // Cash = 0, Monthly = 1, Pending = 2
    bool UpdateName(Node* node, CString& value) override;
    void ConstructTree() override;

private:
    bool UpdateFinished(Node* node, bool value);
    void UpdateAncestorValueOrder(
        Node* node, double first_diff, double second_diff = 0.0, double amount_diff = 0.0, double discount_diff = 0.0, double settled_diff = 0.0);

private:
    SqliteOrder* sql_ {};
};

#endif // TREEMODELORDER_H
