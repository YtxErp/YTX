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

#ifndef NODEMODELO_H
#define NODEMODELO_H

#include <QDate>

#include "database/sqlite/sqliteo.h"
#include "tree/model/nodemodel.h"

class NodeModelO final : public NodeModel {
    Q_OBJECT

public:
    NodeModelO(CNodeModelArg& arg, QObject* parent = nullptr);
    ~NodeModelO() override = default;

public slots:
    void RUpdateLeafValue(int node_id, double initial_delta, double final_delta, double first_delta, double second_delta, double discount_delta) override;
    void RSyncBoolWD(int node_id, int column, bool value) override; // kFinished

public:
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool moveRows(const QModelIndex& sourceParent, int sourceRow, int count, const QModelIndex& destinationParent, int destinationChild) override;

    void UpdateTree(const QDateTime& start, const QDateTime& end);
    QString Path(int node_id) const override;
    void ReadNode(int node_id) override;

    Node* GetNode(int node_id) const override;

protected:
    bool UpdateRule(Node* node, bool value) override; // charge = 0, refund = 1
    bool UpdateUnit(Node* node, int value) override; // Cash = 0, Monthly = 1, Pending = 2
    bool UpdateNameFunction(Node* node, CString& value) override;
    void ConstructTree() override;
    void InsertPath(Node* /*node*/) override { };
    void RemovePath(Node* node, Node* parent_node) override;
    bool UpdateAncestorValue(Node* node, double initial_delta, double final_delta, double first_delta, double second_delta, double discount_delta) override;

private:
    bool UpdateFinished(Node* node, bool value);

private:
    SqliteO* sql_ {};
};

#endif // NODEMODELO_H
