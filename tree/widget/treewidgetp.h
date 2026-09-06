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

#pragma once

#include "component/config.h"
#include "tree/model/treemodel.h"
#include "treewidget.h"

namespace Ui {
class TreeWidgetP;
}

class TreeWidgetP final : public TreeWidget {
    Q_OBJECT

public slots:
    void RSyncValue() override;
    void RInitStatus() override;

public:
    explicit TreeWidgetP(TreeModel* model, CSectionConfig& config, QWidget* parent = nullptr);
    ~TreeWidgetP() override;

    QTreeView* View() const override;
    void Reset() override;

private:
    void InitStaticStatus() override;
    void InitDynamicStatus() override;

    void SyncDynamicValue(const QUuid& lhs_node_id, const QUuid& rhs_node_id) override;
    void SyncStaticValue(const QUuid& node_id) override;

private:
    Ui::TreeWidgetP* ui;
    TreeModel* model_ {};

    CSectionConfig& config_ {};
};
