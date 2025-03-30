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

#ifndef NODEMODELP_H
#define NODEMODELP_H

#include "tree/model/nodemodel.h"

class NodeModelP final : public NodeModel {
    Q_OBJECT

public:
    NodeModelP(CNodeModelArg& arg, QObject* parent = nullptr);
    ~NodeModelP() override;

public slots:
    void RUpdateLeafValue(int node_id, double initial_debit_delta, double initial_credit_delta, double final_debit_delta, double final_credit_delta,
        double delta5 = 0.0) override;

public:
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    const QString& Color(int node_id) const { return NodeModelUtils::Value(node_hash_, node_id, &Node::color); }
    double First(int node_id) const { return NodeModelUtils::Value(node_hash_, node_id, &Node::first); }

    QSortFilterProxyModel* ExcludeUnitModel(int unit) override;

protected:
    void RemoveUnitSet(int node_id, int unit) override;
    void InsertUnitSet(int node_id, int unit) override;

    bool UpdateAncestorValue(
        Node* node, double initial_delta, double final_delta, double first_delta = 0.0, double second_delta = 0.0, double discount_delta = 0.0) override;

    const QSet<int>* UnitSet(int unit) const override
    {
        Q_UNUSED(unit);
        return &pset_;
    }

private:
    QSet<int> pset_ {}; // Set of all nodes that are position-type units
};

#endif // NODEMODELP_H
