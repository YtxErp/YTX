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

#ifndef NODEWIDGETO_H
#define NODEWIDGETO_H

#include "component/info.h"
#include "component/settings.h"
#include "tree/model/nodemodelo.h"
#include "nodewidget.h"

namespace Ui {
class NodeWidgetO;
}

class NodeWidgetO final : public NodeWidget {
    Q_OBJECT

public slots:
    void on_dateEditStart_dateChanged(const QDate& date);
    void on_dateEditEnd_dateChanged(const QDate& date);

public:
    NodeWidgetO(NodeModel* model, CInfo& info, const Settings& settings, QWidget* parent = nullptr);
    ~NodeWidgetO() override;

    QPointer<QTreeView> View() const override;
    QPointer<NodeModel> Model() const override { return model_; };
    bool IsTreeWidget() const override { return true; }

private slots:
    void on_pBtnRefresh_clicked();

private:
    Ui::NodeWidgetO* ui;
    QDate start_ {};
    QDate end_ {};

    NodeModelO* model_ {};
    CInfo& info_;
    const Settings& settings_;
};

#endif // NODEWIDGETO_H
