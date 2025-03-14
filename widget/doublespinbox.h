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

#ifndef DOUBLESPINBOX_H
#define DOUBLESPINBOX_H

#include <QDoubleSpinBox>
#include <QKeyEvent>
#include <QLineEdit>

#include "component/constvalue.h"

class DoubleSpinBox final : public QDoubleSpinBox {
    Q_OBJECT

public:
    explicit DoubleSpinBox(QWidget* parent = nullptr)
        : QDoubleSpinBox { parent }
    {
        setStepType(QAbstractSpinBox::AdaptiveDecimalStepType);
        setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        setButtonSymbols(QAbstractSpinBox::NoButtons);
        setGroupSeparatorShown(true);
    }

protected:
    void wheelEvent(QWheelEvent* event) override { event->ignore(); };
    void keyPressEvent(QKeyEvent* event) override
    {
        if (event->text() == QString::fromUtf8(kFullWidthPeriod)) {
            QKeyEvent new_event(QEvent::KeyPress, Qt::Key_Period, Qt::NoModifier, kHalfWidthPeriod);
            QDoubleSpinBox::keyPressEvent(&new_event);
            return;
        }

        QDoubleSpinBox::keyPressEvent(event);
    }
};

#endif // DOUBLESPINBOX_H
