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

#ifndef TABLEWIDGETFPTS_H
#define TABLEWIDGETFPTS_H

#include <QTableView>

#include "table/model/tablemodel.h"
#include "widget/tablewidget/tablewidget.h"

namespace Ui {
class TableWidgetFPTS;
}

class TableWidgetFPTS final : public TableWidget {
    Q_OBJECT

public:
    explicit TableWidgetFPTS(TableModel* model, QWidget* parent = nullptr);
    ~TableWidgetFPTS();

    QPointer<TableModel> Model() const override { return model_; }
    QPointer<QTableView> View() const override;

private:
    Ui::TableWidgetFPTS* ui;
    TableModel* model_ {};
};

#endif // TABLEWIDGETFPTS_H
