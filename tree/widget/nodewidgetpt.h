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

#ifndef NODEWIDGETPT_H
#define NODEWIDGETPT_H

#include <QDoubleSpinBox>

#include "component/settings.h"
#include "nodewidget.h"

namespace Ui {
class NodeWidgetPT;
}

class NodeWidgetPT final : public NodeWidget {
    Q_OBJECT

public slots:
    void RSyncStatusValue() override;

public:
    NodeWidgetPT(NodeModel* model, CSectionConfig& section_settings, QWidget* parent = nullptr);
    ~NodeWidgetPT() override;

    QPointer<QTreeView> View() const override;
    QPointer<NodeModel> Model() const override { return model_; };
    void UpdateStatus() override;

private:
    void UpdateStaticStatus();
    void UpdateDynamicStatus();

    void UpdateDynamicValue(int lhs_node_id, int rhs_node_id);
    void UpdateStaticValue(int node_id);
    double Operate(double lhs, double rhs, const QString& operation);

private:
    Ui::NodeWidgetPT* ui;
    NodeModel* model_ {};

    CSectionConfig& section_settings_ {};
};

#endif // NODEWIDGETPT_H
