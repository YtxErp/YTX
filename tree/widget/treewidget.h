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

#include <QDoubleSpinBox>
#include <QTreeView>
#include <QWidget>

class TreeWidget : public QWidget {
    Q_OBJECT

public slots:
    virtual void RSyncValue() { };
    virtual void RInitStatus() { };

public:
    ~TreeWidget() override { };

    virtual QTreeView* View() const = 0;
    virtual void Reset() { }

protected:
    TreeWidget(QWidget* parent = nullptr)
        : QWidget { parent }
    {
    }

    virtual void InitStaticStatus() { }
    virtual void InitDynamicStatus() { }
    virtual void SyncDynamicValue(const QUuid& lhs_node_id, const QUuid& rhs_node_id)
    {
        Q_UNUSED(lhs_node_id)
        Q_UNUSED(rhs_node_id)
    }
    virtual void SyncStaticValue(const QUuid& node_id) { Q_UNUSED(node_id) }

    inline double Operate(double lhs, double rhs, const QString& operation)
    {
        switch (operation.at(0).toLatin1()) {
        case '+':
            return lhs + rhs;
        case '-':
            return lhs - rhs;
        default:
            return 0.0;
        }
    }

    inline void InitDoubleSpinBox(QDoubleSpinBox* box) const
    {
        Q_ASSERT(box);

        box->setRange(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max());
        box->setSpecialValueText("--");
        box->setValue(std::numeric_limits<double>::lowest());
    }
};

inline const char* kTreeWidget = "TreeWidget";
