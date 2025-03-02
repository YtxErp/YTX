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

#ifndef SPINBOX_H
#define SPINBOX_H

#include <QSpinBox>
#include <QWheelEvent>

class SpinBox final : public QSpinBox {
    Q_OBJECT

public:
    explicit SpinBox(QWidget* parent = nullptr)
        : QSpinBox { parent }
    {
        setStepType(QAbstractSpinBox::AdaptiveDecimalStepType);
        setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        setGroupSeparatorShown(true);
    }

protected:
    void wheelEvent(QWheelEvent* event) override { event->ignore(); }
};

#endif // SPINBOX_H
