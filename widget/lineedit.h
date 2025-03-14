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

#ifndef LINEEDIT_H
#define LINEEDIT_H

#include <QDateTime>
#include <QKeyEvent>
#include <QLineEdit>
#include <QRegularExpressionValidator>

#include "component/constvalue.h"

class LineEdit final : public QLineEdit {
    Q_OBJECT

public:
    explicit LineEdit(QWidget* parent = nullptr)
        : QLineEdit(parent)
    {
    }

    static inline const QRegularExpression kInputRegex { QStringLiteral("[\\p{L} ()（）\\d]*") };
    static inline const QRegularExpressionValidator kInputValidator { kInputRegex };

protected:
    void keyPressEvent(QKeyEvent* event) override
    {
        if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Semicolon) {
            return insert(QDate::currentDate().toString(kDateFST));
        }

        if (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier) && event->key() == Qt::Key_Semicolon)
            return insert(QDateTime::currentDateTime().toString(kDateTimeFST));

        QLineEdit::keyPressEvent(event);
    }
};

#endif // LINEEDIT_H
