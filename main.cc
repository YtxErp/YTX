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

#include <QApplication>
#include <QDir>
#include <QStandardPaths>

#include "global/dailylogger.h"
#include "mainwindow.h"

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);

    DailyLogger::Instance().Install();
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);

    // Centralize config directory creation
    const QString config_location { QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) };
    if (!QDir(config_location).exists() && !QDir().mkpath(config_location)) {
        qCritical() << "Failed to create config directory:" << config_location;
        return EXIT_FAILURE;
    }

    const QString data_location { QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) };
    if (!QDir(data_location).exists() && !QDir().mkpath(data_location)) {
        qCritical() << "Failed to create data directory:" << data_location;
        return EXIT_FAILURE;
    }

    MainWindow mainwindow {};

    mainwindow.show();
    return application.exec();
}
