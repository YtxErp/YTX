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

#include <QDir>
#include <QStandardPaths>

#include "global/dailylogger.h"
#include "mainwindow.h"

#ifdef Q_OS_MACOS
#include "component/application.h"
#elif defined(Q_OS_WIN32)
#include <QApplication>
#endif

int main(int argc, char* argv[])
{
#ifdef Q_OS_MACOS
    Application application(argc, argv);
#elif defined(Q_OS_WIN32)
    QApplication application(argc, argv);
#endif

    DailyLogger::Instance().Install();
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);

    // Centralize config directory creation
    const QString location { QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) };

    if (!QDir(location).exists() && !QDir().mkpath(location)) {
        qCritical() << "Failed to create config directory:" << location;
        return EXIT_FAILURE;
    }

    MainWindow mainwindow {};

    if (application.arguments().size() >= 2) {
        const QString file_path { application.arguments().at(1) };
        mainwindow.ROpenFile(file_path);
    }

#ifdef Q_OS_MACOS
    QObject::connect(&application, &Application::SOpenFile, &mainwindow, &MainWindow::ROpenFile);
#endif

    mainwindow.show();
    return application.exec();
}
