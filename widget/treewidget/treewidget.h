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

#ifndef TREEWIDGET_H
#define TREEWIDGET_H

#include <QPointer>
#include <QTreeView>
#include <QWidget>

#include "tree/model/treemodel.h"

class TreeWidget : public QWidget {
    Q_OBJECT

public slots:
    virtual void RUpdateStatusValue() { };

public:
    virtual ~TreeWidget() = default;

    virtual void UpdateStatus() { };
    virtual QPointer<QTreeView> View() const = 0;
    virtual QPointer<TreeModel> Model() const = 0;

protected:
    TreeWidget(QWidget* parent = nullptr)
        : QWidget { parent }
    {
    }
};

using PQTreeView = QPointer<QTreeView>;

#endif // TREEWIDGET_H
