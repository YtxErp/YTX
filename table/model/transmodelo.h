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

#ifndef TRANSMODELO_H
#define TRANSMODELO_H

#include "database/sqlite/sqliteo.h"
#include "database/sqlite/sqlites.h"
#include "transmodel.h"
#include "tree/model/nodemodel.h"
#include "tree/model/nodemodelp.h"

class TransModelO final : public TransModel {
    Q_OBJECT

public:
    TransModelO(CTransModelArg& arg, const Node* node, CNodeModel* product_tree, Sqlite* sqlite_stakeholder, QObject* parent = nullptr);
    ~TransModelO() override = default;

public slots:
    void RSyncBoolWD(int node_id, int column, bool value) override; // kFinished, kRule
    void RSyncInt(int node_id, int column, int value) override; // node_id, party_id

public:
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    bool removeRows(int row, int, const QModelIndex& parent = QModelIndex()) override;

    const QList<TransShadow*>& GetTransShadowList() { return trans_shadow_list_; }

private:
    bool UpdateInsideProduct(TransShadow* trans_shadow, int value);
    bool UpdateOutsideProduct(TransShadow* trans_shadow, int value);

    bool UpdateUnitPrice(TransShadow* trans_shadow, double value);
    bool UpdateDiscountPrice(TransShadow* trans_shadow, double value);
    bool UpdateSecond(TransShadow* trans_shadow, double value, int kCoefficient);
    bool UpdateFirst(TransShadow* trans_shadow, double value, int kCoefficient);
    void PurifyTransShadow(int lhs_node_id = 0);

    void CrossSearch(TransShadow* trans_shadow, int product_id, bool is_inside) const;

    void UpdateLhsNode(int node_id);
    void UpdateParty(int node_id, int party_id);
    void UpdateRule(int node_id, bool value);

private:
    const NodeModelP* product_tree_ {};
    SqliteS* sqlite_stakeholder_ {};
    SqliteO* sqlite_order_ {};
    const Node* node_ {};
    int party_id_ {};
};

#endif // TRANSMODELO_H
