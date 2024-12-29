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

#ifndef PLAINTEXTEDIT_H
#define PLAINTEXTEDIT_H

#include <QDateTime>
#include <QKeyEvent>
#include <QPlainTextEdit>

#include "component/constvalue.h"

class PlainTextEdit final : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit PlainTextEdit(QWidget* parent = nullptr)
        : QPlainTextEdit { parent }
    {
        setTabChangesFocus(true);
        setUndoRedoEnabled(true);
    }

protected:
    void keyPressEvent(QKeyEvent* event) override
    {
        if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Semicolon)
            return insertPlainText(QDate::currentDate().toString(kDateFST));

        if (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier) && event->key() == Qt::Key_Semicolon)
            return insertPlainText(QDateTime::currentDateTime().toString(kDateTimeFST));

        QPlainTextEdit::keyPressEvent(event);
    }
};

#endif // PLAINTEXTEDIT_H
