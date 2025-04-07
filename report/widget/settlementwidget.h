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

#ifndef SETTLEMENTWIDGET_H
#define SETTLEMENTWIDGET_H

#include <QDateTime>

#include "component/using.h"
#include "report/model/settlementmodel.h"
#include "report/model/settlementprimarymodel.h"
#include "reportwidget.h"

namespace Ui {
class SettlementWidget;
}

class SettlementWidget final : public ReportWidget {
    Q_OBJECT

signals:
    void SNodeLocation(int node_id);

public:
    SettlementWidget(
        SettlementModel* settlement_model, SettlementPrimaryModel* settlement_primary_model, CDateTime& start, CDateTime& end, QWidget* parent = nullptr);
    ~SettlementWidget() override;

    QPointer<QTableView> View() const override;
    QPointer<QTableView> PrimaryView() const;
    QPointer<QAbstractItemModel> Model() const override;

private slots:
    void on_pBtnRefresh_clicked();
    void on_start_dateChanged(const QDate& date);
    void on_end_dateChanged(const QDate& date);
    void on_pBtnAppend_clicked();
    void on_pBtnRemoveSettlement_clicked();
    void on_settlementView_doubleClicked(const QModelIndex& index);
    void on_settlementViewPrimary_doubleClicked(const QModelIndex& index);

private:
    void IniWidget(SettlementModel* settlement_model, SettlementPrimaryModel* settlement_primary_model);

private:
    Ui::SettlementWidget* ui;
    SettlementModel* settlement_model_ {};
    SettlementPrimaryModel* settlement_primary_model_ {};
    QDateTime start_ {};
    QDateTime end_ {};
};

#endif // SETTLEMENTWIDGET_H
