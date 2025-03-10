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

#ifndef REFWIDGET_H
#define REFWIDGET_H

#include <QDateTime>

#include "component/using.h"
#include "reportwidget.h"

namespace Ui {
class RefWidget;
}

class RefWidget final : public ReportWidget {
    Q_OBJECT

signals:
    void SRetrieveData(int node_id, const QDateTime& start, const QDateTime& end);

public:
    RefWidget(QAbstractItemModel* model, int node_id, CDateTime& start, CDateTime& end, QWidget* parent = nullptr);
    ~RefWidget() override;

    QPointer<QTableView> View() const override;
    QPointer<QAbstractItemModel> Model() const override;
    bool IsReportWidget() const override { return true; }

private slots:
    void on_pBtnRefresh_clicked();
    void on_start_dateChanged(const QDate& date);
    void on_end_dateChanged(const QDate& date);

private:
    void IniWidget(QAbstractItemModel* model);

private:
    Ui::RefWidget* ui;
    QDateTime start_ {};
    QDateTime end_ {};

    const int node_id_ {};
};

#endif // REFWIDGET_H
