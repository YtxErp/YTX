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

#ifndef NODEWIDGETO_H
#define NODEWIDGETO_H

#include "nodewidget.h"
#include "tree/model/nodemodelo.h"

namespace Ui {
class NodeWidgetO;
}

class NodeWidgetO final : public NodeWidget {
    Q_OBJECT

public slots:
    void on_start_dateChanged(const QDate& date);
    void on_end_dateChanged(const QDate& date);

public:
    NodeWidgetO(NodeModel* model, QWidget* parent = nullptr);
    ~NodeWidgetO() override;

    QPointer<QTreeView> View() const override;
    QPointer<NodeModel> Model() const override { return model_; };
    bool IsNodeWidget() const override { return true; }

private slots:
    void on_pBtnRefresh_clicked();

private:
    Ui::NodeWidgetO* ui;
    NodeModelO* model_ {};

    QDateTime start_ {};
    QDateTime end_ {};
};

#endif // NODEWIDGETO_H
