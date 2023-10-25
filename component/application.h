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

#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>
#include <QFileOpenEvent>

class Application : public QApplication {
    Q_OBJECT

signals:
    // Signal emitted when a file is opened
    void SOpenFile(const QString& file_path);

public:
    // Constructor for the custom application class
    Application(int& argc, char** argv)
        : QApplication(argc, argv)
    {
    }

protected:
    // Override event handler to handle file open events
    bool event(QEvent* event) override
    {
        if (event->type() == QEvent::FileOpen) {
            // Cast the event to QFileOpenEvent
            auto open_event { static_cast<QFileOpenEvent*>(event) };
            // Emit the signal with the file path from the event
            emit SOpenFile(open_event->file());
        }

        // Pass the event to the base class event handler
        return QApplication::event(event);
    }
};

#endif // APPLICATION_H
