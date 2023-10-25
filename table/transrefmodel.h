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

#ifndef TRANSREFMODEL_H
#define TRANSREFMODEL_H

// Leaf node referenced by transactions
// such as a product node linked to order transactions or a stakeholder node associated with an order transaction.

#include <QAbstractItemModel>

#include "component/info.h"
#include "database/sqlite/sqlite.h"

class TransRefModel final : public QAbstractItemModel {
    Q_OBJECT
public:
    TransRefModel(int node_id, CInfo& info, Sqlite* sql, QObject* parent = nullptr);
    ~TransRefModel();

public:
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void sort(int column, Qt::SortOrder order) override;

private:
    void Query(int node_id);

private:
    Sqlite* sql_ {};

    TransList trans_list_ {};
    CInfo& info_;
    const int node_id_ {};
};

#endif // TRANSREFMODEL_H
