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

#ifndef DAILYLOGGER_H
#define DAILYLOGGER_H

#include <QFile>
#include <QTextStream>

class DailyLogger {
public:
    static DailyLogger& Instance();
    void Install();

private:
    DailyLogger();
    void HandleMessage(QtMsgType type, const QMessageLogContext& context, const QString& msg);
    static void MessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);

private:
    QFile file_ {};
    QTextStream log_stream_ {};
};

#endif // DAILYLOGGER_H
