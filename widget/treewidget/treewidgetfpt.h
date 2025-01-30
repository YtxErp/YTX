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

#ifndef TREEWIDGETFPT_H
#define TREEWIDGETFPT_H

#include "component/info.h"
#include "component/settings.h"
#include "treewidget.h"

namespace Ui {
class TreeWidgetFPT;
}

class TreeWidgetFPT final : public TreeWidget {
    Q_OBJECT

public slots:
    void RUpdateDSpinBox() override;

public:
    TreeWidgetFPT(TreeModel* model, CInfo& info, CSettings& settings, QWidget* parent = nullptr);
    ~TreeWidgetFPT() override;

    QPointer<QTreeView> View() const override;
    QPointer<TreeModel> Model() const override { return model_; };
    void SetStatus() override;

private:
    void DynamicStatus(int lhs_node_id, int rhs_node_id);
    void StaticStatus(int node_id);
    double Operate(double lhs, double rhs, const QString& operation);

private:
    Ui::TreeWidgetFPT* ui;

    TreeModel* model_ {};
    CInfo& info_ {};
    CSettings& settings_ {};

    bool dynamic_unit_is_not_default_but_equal_ { false };
    bool static_unit_is_default_ { false };
};

#endif // TREEWIDGETFPT_H
