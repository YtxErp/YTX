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

#ifndef EXCLUDEINTFILTERMODEL_H
#define EXCLUDEINTFILTERMODEL_H

#include <QSortFilterProxyModel>
#include <QStandardItemModel>

class ExcludeIntFilterModel : public QSortFilterProxyModel {
public:
    explicit ExcludeIntFilterModel(int leaf_id, QObject* parent = nullptr)
        : QSortFilterProxyModel { parent }
        , leaf_id_ { leaf_id }
    {
    }

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& /*source_parent*/) const override
    {
        assert(dynamic_cast<QStandardItemModel*>(sourceModel()) && "sourceModel() is not QStandardItemModel");
        auto* item { static_cast<QStandardItemModel*>(sourceModel())->item(source_row) };
        assert(item && "item is nullptr");

        return item->data(Qt::UserRole).toInt() != leaf_id_;
    }

private:
    const int leaf_id_ {};
};

#endif // EXCLUDEINTFILTERMODEL_H
