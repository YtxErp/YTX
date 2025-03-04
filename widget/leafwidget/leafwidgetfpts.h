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

#ifndef LEAFWIDGETFPTS_H
#define LEAFWIDGETFPTS_H

#include <QTableView>

#include "table/model/tablemodel.h"
#include "widget/leafwidget/leafwidget.h"

namespace Ui {
class LeafWidgetFPTS;
}

class LeafWidgetFPTS final : public LeafWidget {
    Q_OBJECT

public:
    explicit LeafWidgetFPTS(TableModel* model, QWidget* parent = nullptr);
    ~LeafWidgetFPTS();

    QPointer<TableModel> Model() const override { return model_; }
    QPointer<QTableView> View() const override;
    bool IsTableWidget() const override { return true; }

private:
    Ui::LeafWidgetFPTS* ui;
    TableModel* model_ {};
};

#endif // LEAFWIDGETFPTS_H
