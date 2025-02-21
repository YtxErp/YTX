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

#ifndef TREEWIDGETPT_H
#define TREEWIDGETPT_H

#include <QDoubleSpinBox>

#include "component/settings.h"
#include "treewidget.h"

namespace Ui {
class TreeWidgetPT;
}

class TreeWidgetPT final : public TreeWidget {
    Q_OBJECT

public slots:
    void RUpdateStatusValue() override;

public:
    TreeWidgetPT(TreeModel* model, CSettings& settings, QWidget* parent = nullptr);
    ~TreeWidgetPT() override;

    QPointer<QTreeView> View() const override;
    QPointer<TreeModel> Model() const override { return model_; };
    void UpdateStatus() override;

private:
    void UpdateStaticStatus();
    void UpdateDynamicStatus();

    void UpdateDynamicValue(int lhs_node_id, int rhs_node_id);
    void UpdateStaticValue(int node_id);
    double Operate(double lhs, double rhs, const QString& operation);

private:
    Ui::TreeWidgetPT* ui;

    TreeModel* model_ {};
    CSettings& settings_ {};
};

#endif // TREEWIDGETPT_H
