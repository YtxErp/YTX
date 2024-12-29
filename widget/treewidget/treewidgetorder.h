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

#ifndef TREEWIDGETORDER_H
#define TREEWIDGETORDER_H

#include "component/info.h"
#include "component/settings.h"
#include "tree/model/treemodelorder.h"
#include "treewidget.h"

namespace Ui {
class TreeWidgetOrder;
}

class TreeWidgetOrder final : public TreeWidget {
    Q_OBJECT

public slots:
    void on_dateEditStart_dateChanged(const QDate& date);
    void on_dateEditEnd_dateChanged(const QDate& date);

public:
    TreeWidgetOrder(TreeModel* model, CInfo& info, const Settings& settings, QWidget* parent = nullptr);
    ~TreeWidgetOrder() override;

    QPointer<QTreeView> View() const override;
    QPointer<TreeModel> Model() const override { return model_; };

private slots:
    void on_pBtnRefresh_clicked();

private:
    Ui::TreeWidgetOrder* ui;
    QDate start_ {};
    QDate end_ {};

    TreeModelOrder* model_ {};
    CInfo& info_;
    const Settings& settings_;
};

#endif // TREEWIDGETORDER_H
