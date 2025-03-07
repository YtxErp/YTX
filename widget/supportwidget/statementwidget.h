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

#include "component/info.h"
#include "component/settings.h"
#include "supportwidget.h"
#include "tree/model/treemodelorder.h"

namespace Ui {
class StatementWidget;
}

class StatementWidget final : public SupportWidget {
    Q_OBJECT

public slots:
    void on_dateEditStart_dateChanged(const QDate& date);
    void on_dateEditEnd_dateChanged(const QDate& date);

public:
    StatementWidget(TreeModel* model, CInfo& info, const Settings& settings, QWidget* parent = nullptr);
    ~StatementWidget() override;

    QPointer<QTableView> View() const override;
    QPointer<QAbstractItemModel> Model() const override { return model_; };
    bool IsSupportWidget() const override { return true; }

private slots:
    void on_pBtnRefresh_clicked();
    void RUnitGroupClicked(int id);

private:
    void IniUnitGroup();

private:
    Ui::StatementWidget* ui;
    QDate start_ {};
    QDate end_ {};

    QButtonGroup* unit_group_ {};

    TreeModelOrder* model_ {};
    CInfo& info_;
    const Settings& settings_;
};

#endif // STATEMENTWIDGET_H
