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

#ifndef NODEWIDGETS_H
#define NODEWIDGETS_H

#include "nodewidget.h"

namespace Ui {
class NodeWidgetS;
}

class NodeWidgetS final : public NodeWidget {
    Q_OBJECT

public:
    NodeWidgetS(NodeModel* model, QWidget* parent = nullptr);
    ~NodeWidgetS() override;

    QPointer<QTreeView> View() const override;
    QPointer<NodeModel> Model() const override { return model_; };
    bool IsNodeWidget() const override { return true; }

private:
    Ui::NodeWidgetS* ui;
    NodeModel* model_ {};
};

#endif // NODEWIDGETS_H
