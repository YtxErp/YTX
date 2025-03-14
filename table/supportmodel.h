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

#ifndef SUPPORTMODEL_H
#define SUPPORTMODEL_H

#include <QAbstractItemModel>

#include "database/sqlite/sqlite.h"
#include "table/trans.h"

class SupportModel final : public QAbstractItemModel {
    Q_OBJECT

public:
    SupportModel(Sqlite* sql, bool rule, int node_id, CInfo& info, QObject* parent = nullptr);
    ~SupportModel();

public slots:
    // receive from TableModel
    void RAppendSupportTrans(const TransShadow* trans_shadow);
    void RRemoveSupportTrans(int support_id, int trans_id);

    // receive from SupportStation
    void RAppendMultiSupportTrans(int new_support_id, const QList<int>& trans_id_list);

    // receive from sql
    bool RemoveMultiSupportTrans(const QMultiHash<int, int>& node_trans);

public:
    inline int rowCount(const QModelIndex& /*parent*/ = QModelIndex()) const override { return trans_shadow_list_.size(); }
    inline int columnCount(const QModelIndex& /*parent*/ = QModelIndex()) const override { return info_.search_trans_header.size(); }
    inline QModelIndex parent(const QModelIndex& /*index*/) const override { return QModelIndex(); }

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QModelIndex GetIndex(int trans_id) const;

private:
    Sqlite* sql_ {};
    bool rule_ {};

    CInfo& info_;
    int node_id_ {};

    QList<TransShadow*> trans_shadow_list_ {};
};

#endif // SUPPORTMODEL_H
