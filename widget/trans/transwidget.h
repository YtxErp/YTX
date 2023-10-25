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

#ifndef TRANSWIDGET_H
#define TRANSWIDGET_H

#include <QPointer>
#include <QTableView>
#include <QWidget>

#include "table/model/transmodel.h"

class TransWidget : public QWidget {
    Q_OBJECT

public:
    virtual ~TransWidget() = default;

    virtual QPointer<TransModel> Model() const = 0;
    virtual QPointer<QTableView> View() const = 0;
    virtual bool IsLeafWidget() const = 0;

protected:
    explicit TransWidget(QWidget* parent = nullptr)
        : QWidget { parent }
    {
    }
};

using TransWgtHash = QHash<int, QPointer<TransWidget>>;
using CTransWgtHash = const QHash<int, QPointer<TransWidget>>;
using PTableView = QPointer<QTableView>;

#endif // TRANSWIDGET_H
