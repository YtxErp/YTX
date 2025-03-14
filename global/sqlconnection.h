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

#ifndef SQLCONNECTION_H
#define SQLCONNECTION_H

#include <QHash>
#include <QMutex>
#include <QSqlDatabase>

#include "component/enumclass.h"

class SqlConnection {
public:
    static SqlConnection& Instance();
    bool SetDatabaseName(const QString& file_path);
    QString DatabaseName() { return file_path_; }
    QSqlDatabase* Allocate(Section section);
    // bool IsInitialized() { return is_initialized_; }

private:
    SqlConnection() = default;
    ~SqlConnection();

    SqlConnection(const SqlConnection&) = delete;
    SqlConnection& operator=(const SqlConnection&) = delete;
    SqlConnection(SqlConnection&&) = delete;
    SqlConnection& operator=(SqlConnection&&) = delete;

    void LogError(const QString& message) const;
    QSqlDatabase OpenDatabase(const QString& file_path, Section section);

private:
    QMutex mutex_;
    QHash<Section, QSqlDatabase> hash_;
    QString file_path_ {};
    bool is_initialized_ { false };
};

#endif // SQLCONNECTION_H
