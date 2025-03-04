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

#ifndef REFFETCHERWIDGETPS_H
#define REFFETCHERWIDGETPS_H

#include <QPointer>
#include <QTableView>

#include "reffetcherwidget.h"

namespace Ui {
class RefFetcherWidgetPS;
}

class RefFetcherWidgetPS final : public RefFetcherWidget {
    Q_OBJECT

public:
    explicit RefFetcherWidgetPS(QAbstractItemModel* model, QWidget* parent = nullptr);
    ~RefFetcherWidgetPS();

    QPointer<QAbstractItemModel> Model() const override { return model_; }
    QPointer<QTableView> View() const override;

private:
    Ui::RefFetcherWidgetPS* ui;
    QAbstractItemModel* model_ {};
};

#endif // REFFETCHERWIDGETPS_H
