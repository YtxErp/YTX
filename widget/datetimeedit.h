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

#ifndef DATETIMEEDIT_H
#define DATETIMEEDIT_H

#include <QDateTimeEdit>

class DateTimeEdit final : public QDateTimeEdit {
    Q_OBJECT

public:
    DateTimeEdit(QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    bool LastMonthEnd(QDateTime& date_time);
    bool NextMonthStart(QDateTime& date_time);
    bool HandleSpecialKeys(int key, QDateTime& date_time);
    bool AdjustDateTime(QDateTime& date_time, int days = 0, int months = 0, int years = 0);
    bool SetToCurrentDateTime(QDateTime& date_time);
};

#endif // DATETIMEEDIT_H
