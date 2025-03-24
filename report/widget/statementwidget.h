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

#ifndef STATEMENTWIDGET_H
#define STATEMENTWIDGET_H

#include <QButtonGroup>
#include <QDateTime>

#include "component/using.h"
#include "reportwidget.h"

namespace Ui {
class StatementWidget;
}

class StatementWidget final : public ReportWidget {
    Q_OBJECT

signals:
    void SStatementPrimary(int party_id, int unit, const QDateTime& start, const QDateTime& end);
    void SStatementSecondary(int party_id, int unit, const QDateTime& start, const QDateTime& end);
    void SRetrieveData(int unit, const QDateTime& start, const QDateTime& end);
    void SExport(int unit, const QDateTime& start, const QDateTime& end);

public:
    StatementWidget(QAbstractItemModel* model, int unit, bool enable_excel, CDateTime& start, CDateTime& end, QWidget* parent = nullptr);
    ~StatementWidget() override;

    QPointer<QTableView> View() const override;
    QPointer<QAbstractItemModel> Model() const override;

private slots:
    void on_pBtnRefresh_clicked();
    void on_tableView_doubleClicked(const QModelIndex& index);
    void on_start_dateChanged(const QDate& date);
    void on_end_dateChanged(const QDate& date);
    void on_pBtnExport_clicked();

    void RUnitGroupClicked(int id);

private:
    void IniUnitGroup();
    void IniConnect();
    void IniUnit(int unit);
    void IniWidget(QAbstractItemModel* model, bool enable_excel);

private:
    Ui::StatementWidget* ui;
    int unit_ {};
    QDateTime start_ {};
    QDateTime end_ {};

    QButtonGroup* unit_group_ {};
};

#endif // STATEMENTWIDGET_H
