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
    SupportModel(Sqlite* sql, int support_id, CInfo& info, QObject* parent = nullptr);
    ~SupportModel() = default;

public slots:
    // receive from SupportSStation
    void RAppendOneTransS(int support_id, int trans_id);
    void RRemoveOneTransS(int support_id, int trans_id);
    void RRemoveMultiTransS(int support_id, const QSet<int>& trans_id_set);
    void RAppendMultiTransS(int support_id, const QSet<int>& trans_id_set);

public:
    inline int rowCount(const QModelIndex& /*parent*/ = QModelIndex()) const override { return trans_list_.size(); }
    inline int columnCount(const QModelIndex& /*parent*/ = QModelIndex()) const override { return info_.search_trans_header.size(); }
    inline QModelIndex parent(const QModelIndex& /*index*/) const override { return QModelIndex(); }

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QModelIndex GetIndex(int trans_id) const;

private:
    Sqlite* sql_ {};
    CInfo& info_;
    int support_id_ {};

    TransList trans_list_ {};
};

#endif // SUPPORTMODEL_H
