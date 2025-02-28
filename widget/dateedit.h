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

#ifndef DATEEDIT_H
#define DATEEDIT_H

#include <QApplication>
#include <QDateEdit>
#include <QStyle>

class DateEdit final : public QDateEdit {
    Q_OBJECT

public:
    DateEdit(QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    bool LastMonthEnd(QDate& date);
    bool NextMonthStart(QDate& date);

    bool HandleSpecialKeys(int key, QDate& date);
    bool AdjustDate(QDate& date, int days = 0, int months = 0, int years = 0);
    bool SetToCurrentDate(QDate& date);
};

#endif // DATEEDIT_H
