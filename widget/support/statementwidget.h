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

#ifndef STATEMENTWIDGET_H
#define STATEMENTWIDGET_H

#include <QButtonGroup>

#include "supportwidget.h"
#include "table/statementmodel.h"

namespace Ui {
class StatementWidget;
}

class StatementWidget final : public SupportWidget {
    Q_OBJECT

signals:
    void SPrimaryStatement(int party_id, QDateTime start, QDateTime end, double pbalance, double cbalance);
    void SSecondaryStatement(int party_id, QDateTime start, QDateTime end, double pbalance, double cbalance);

public slots:
    void on_start_dateChanged(const QDate& date);
    void on_end_dateChanged(const QDate& date);

public:
    StatementWidget(StatementModel* model, QWidget* parent = nullptr);
    ~StatementWidget() override;

    QPointer<QTableView> View() const override;
    QPointer<QAbstractItemModel> Model() const override { return model_; };
    bool IsSupportWidget() const override { return true; }

private slots:
    void on_pBtnRefresh_clicked();
    void on_tableViewStatement_doubleClicked(const QModelIndex& index);
    void RUnitGroupClicked(int id);

private:
    void IniUnitGroup();
    void IniConnect();
    void IniWidget(StatementModel* model);
    void IniData();

private:
    Ui::StatementWidget* ui;
    QDateTime start_ {};
    QDateTime end_ {};
    UnitO unit_ {};

    QButtonGroup* unit_group_ {};

    StatementModel* model_ {};
};

#endif // STATEMENTWIDGET_H
