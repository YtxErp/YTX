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

#ifndef STATEMENTSECONDARYMODEL_H
#define STATEMENTSECONDARYMODEL_H

#include <QAbstractItemModel>

#include "component/info.h"
#include "database/sql/sqlo.h"
#include "tree/model/nodemodel.h"

class StatementSecondaryModel final : public QAbstractItemModel {
    Q_OBJECT
public:
    StatementSecondaryModel(
        Sql* sql, CInfo& info, int party_id, CStringHash& product_leaf, PNodeModel stakeholder, CString& company_name, QObject* parent = nullptr);
    ~StatementSecondaryModel();

public slots:
    void RResetModel(int unit, const QDateTime& start, const QDateTime& end);
    void RExport(int unit, const QDateTime& start, const QDateTime& end);

public:
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void sort(int column, Qt::SortOrder order) override;

private:
    SqlO* sql_ {};
    CInfo& info_;
    const int party_id_ {};
    CStringHash& product_leaf_ {};
    CStringHash& stakeholder_leaf_ {};
    PNodeModel stakeholder_ {};
    CString& company_name_ {};
    TransList trans_list_ {};
};

#endif // STATEMENTSECONDARYMODEL_H
