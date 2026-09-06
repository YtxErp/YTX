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

#include "tree/model/treemodel.h"
#include "treewidget.h"
#include "utils/daterange.h"

namespace Ui {
class TreeWidgetO;
}

class TreeWidgetO final : public TreeWidget {
    Q_OBJECT

public:
    TreeWidgetO(Section section, TreeModel* model, QWidget* parent = nullptr);
    ~TreeWidgetO() override;

    QTreeView* View() const override;
    void Reset() override;

private slots:
    void on_start_dateChanged(const QDate& date);
    void on_end_dateChanged(const QDate& date);
    void on_pBtnFetch_clicked();

private:
    void InitWidget();
    void InitTimer();
    static utils::DateRange DefaultRange()
    {
        const auto today { QDate::currentDate() };
        return { today, today };
    }

private:
    Ui::TreeWidgetO* ui;
    Section section_ {};
    TreeModel* model_ {};

    utils::DateRange range_ {};

    QTimer* cooldown_timer_ { nullptr };
};

inline const char* kTreeWidgetO = "TreeWidgetO";
