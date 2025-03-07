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

#ifndef TRANSWIDGETFPTS_H
#define TRANSWIDGETFPTS_H

#include <QTableView>

#include "table/model/transmodel.h"
#include "widget/trans/transwidget.h"

namespace Ui {
class TransWidgetFPTS;
}

class TransWidgetFPTS final : public TransWidget {
    Q_OBJECT

public:
    explicit TransWidgetFPTS(TransModel* model, QWidget* parent = nullptr);
    ~TransWidgetFPTS();

    QPointer<TransModel> Model() const override { return model_; }
    QPointer<QTableView> View() const override;
    bool IsLeafWidget() const override { return true; }

private:
    Ui::TransWidgetFPTS* ui;
    TransModel* model_ {};
};

#endif // TRANSWIDGETFPTS_H
