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

#ifndef TABLEMODELSTAKEHOLDER_H
#define TABLEMODELSTAKEHOLDER_H

#include "tablemodel.h"

class TableModelStakeholder final : public TableModel {
    Q_OBJECT

public:
    TableModelStakeholder(Sqlite* sql, bool rule, int node_id, CInfo& info, QObject* parent = nullptr);
    ~TableModelStakeholder() override = default;

public slots:
    void RAppendPrice(TransShadow* trans_shadow);

public:
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

protected:
    bool AppendMultiTrans(int node_id, const QList<int>& trans_id_list) override;
    bool RemoveMultiTrans(const QList<int>& trans_id_list) override;

private:
    bool UpdateInsideProduct(TransShadow* trans_shadow, int value) const;
};

#endif // TABLEMODELSTAKEHOLDER_H
