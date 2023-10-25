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

#ifndef NODEWIDGET_H
#define NODEWIDGET_H

#include <QPointer>
#include <QTreeView>
#include <QWidget>

#include "tree/model/nodemodel.h"

class NodeWidget : public QWidget {
    Q_OBJECT

public slots:
    virtual void RUpdateStatusValue() { };

public:
    virtual ~NodeWidget() = default;

    virtual void UpdateStatus() { };
    virtual QPointer<QTreeView> View() const = 0;
    virtual QPointer<NodeModel> Model() const = 0;
    virtual bool IsTreeWidget() const = 0;

protected:
    NodeWidget(QWidget* parent = nullptr)
        : QWidget { parent }
    {
    }
};

using PTreeView = QPointer<QTreeView>;

#endif // NODEWIDGET_H
