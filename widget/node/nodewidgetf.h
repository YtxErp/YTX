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

#ifndef NODEWIDGETF_H
#define NODEWIDGETF_H

#include <QDoubleSpinBox>

#include "component/info.h"
#include "component/settings.h"
#include "nodewidget.h"

namespace Ui {
class NodeWidgetF;
}

class NodeWidgetF final : public NodeWidget {
    Q_OBJECT

public slots:
    void RUpdateStatusValue() override;

public:
    NodeWidgetF(NodeModel* model, CInfo& info, CSettings& settings, QWidget* parent = nullptr);
    ~NodeWidgetF() override;

    QPointer<QTreeView> View() const override;
    QPointer<NodeModel> Model() const override { return model_; };
    void UpdateStatus() override;
    bool IsTreeWidget() const override { return true; }

private:
    void UpdateStaticStatus();
    void UpdateDynamicStatus();

    void UpdateDynamicValue(int lhs_node_id, int rhs_node_id);
    void UpdateStaticValue(int node_id);
    double Operate(double lhs, double rhs, const QString& operation);
    void ResetStatus(QDoubleSpinBox* spin_box, bool& flags);

private:
    Ui::NodeWidgetF* ui;
    NodeModel* model_ {};

    CInfo& info_ {};
    CSettings& settings_ {};

    bool dynamic_unit_is_not_default_but_equal_ { false };
    bool static_unit_is_default_ { false };
};

#endif // NODEWIDGETF_H
