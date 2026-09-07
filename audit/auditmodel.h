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

#pragma once

#include <QAbstractItemModel>
#include <QJsonArray>

#include "auditinfo.h"
#include "auditrow.h"
#include "component/using.h"
#include "enum/section.h"

namespace audit {

class Model final : public QAbstractItemModel {
    Q_OBJECT

public:
    explicit Model(const Info& info, const QStringList& header, CUuidString& leaf, CUuidString& branch, Section section, QObject* parent = nullptr);

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override
    {
        Q_UNUSED(index)
        return QModelIndex();
    }
    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        Q_UNUSED(parent)
        return list_.size();
    }
    int columnCount(const QModelIndex& parent = QModelIndex()) const override
    {
        Q_UNUSED(parent)
        return header_.size();
    }
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    void sort(int column, Qt::SortOrder order) override;

    void Rebuild(const QJsonArray& array);

private:
    const QString NodePath(const QUuid& node_id) const;
    static QString JsonValueToString(const QJsonValue& value);

private:
    QList<Row*> list_ {};
    const Section section_ {};

    // owned by MainWindow, outlives model
    const Info& info_;
    CUuidString& leaf_path_;
    CUuidString& branch_path_;

    const QStringList& header_;
};
}
