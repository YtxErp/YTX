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

#ifndef SUPPORTWIDGETFPTS_H
#define SUPPORTWIDGETFPTS_H

#include <QPointer>
#include <QTableView>

#include "supportwidget.h"

namespace Ui {
class SupportWidgetFPTS;
}

class SupportWidgetFPTS final : public SupportWidget {
    Q_OBJECT

public:
    explicit SupportWidgetFPTS(QAbstractItemModel* model, QWidget* parent = nullptr);
    ~SupportWidgetFPTS();

    QPointer<QAbstractItemModel> Model() const override { return model_; }
    QPointer<QTableView> View() const override;
    bool IsSupportWidget() const override { return true; }

private:
    Ui::SupportWidgetFPTS* ui;
    QAbstractItemModel* model_ {};
};

#endif // SUPPORTWIDGETFPTS_H
