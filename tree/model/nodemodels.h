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

#ifndef NODEMODELS_H
#define NODEMODELS_H

#include "tree/model/nodemodel.h"

class NodeModelS final : public NodeModel {
    Q_OBJECT

public:
    NodeModelS(CNodeModelArg& arg, QObject* parent = nullptr);
    ~NodeModelS() override;

public slots:
    void RSyncStakeholder(int old_node_id, int new_node_id) override;
    void RSyncDouble(int node_id, int column, double value) override; // amount
    void RSyncMultiLeafValue(const QList<int>& node_list) override;

public:
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QList<int> PartyList(CString& text, int unit) const;

    QSortFilterProxyModel* IncludeUnitModel(int unit) override;

protected:
    const QSet<int>* UnitSet(int unit) const override;
    void RemoveUnitSet(int node_id, int unit) override;
    void InsertUnitSet(int node_id, int unit) override;

    bool UpdateAncestorValue(
        Node* node, double initial_delta, double final_delta, double first_delta = 0.0, double second_delta = 0.0, double discount_delta = 0.0) override;

private:
    QSet<int> cset_ {}; // Set of all nodes that are customer-type units
    QSet<int> vset_ {}; // Set of all nodes that are vendor-type units
    QSet<int> eset_ { 0 }; // Set of all nodes that are employee-type units
};

#endif // NODEMODELS_H
