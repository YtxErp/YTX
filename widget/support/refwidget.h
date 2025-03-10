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

#include "supportwidget.h"
#include "table/transrefmodel.h"

namespace Ui {
class RefWidget;
}

class RefWidget final : public SupportWidget {
    Q_OBJECT

public slots:
    void on_start_dateChanged(const QDate& date);
    void on_end_dateChanged(const QDate& date);

public:
    RefWidget(TransRefModel* model, int node_id, QWidget* parent = nullptr);
    ~RefWidget() override;

    QPointer<QTableView> View() const override;
    QPointer<QAbstractItemModel> Model() const override { return model_; };
    bool IsSupportWidget() const override { return true; }

private slots:
    void on_pBtnRefresh_clicked();

private:
    void IniWidget(TransRefModel* model);
    void IniData();

private:
    Ui::RefWidget* ui;
    QDateTime start_ {};
    QDateTime end_ {};

    TransRefModel* model_ {};
    const int node_id_ {};
};

#endif // REFWIDGET_H
