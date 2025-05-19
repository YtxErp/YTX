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

#ifndef PGCONNECTIONPOOL_H
#define PGCONNECTIONPOOL_H

#include <QMutex>
#include <QQueue>
#include <QSqlDatabase>
#include <QString>

#include "component/using.h"

class PGConnectionPool {
public:
    static PGConnectionPool& Instance();

    bool Initialize(CString& host, int port, CString& user, CString& password, CString& db_name, int pool_size = 5);
    QSqlDatabase GetConnection();
    void ReturnConnection(const QSqlDatabase& db);
    void Reset();

private:
    PGConnectionPool() = default;
    ~PGConnectionPool();

    PGConnectionPool(const PGConnectionPool&) = delete;
    PGConnectionPool& operator=(const PGConnectionPool&) = delete;

private:
    QMutex mutex_ {};
    QQueue<QSqlDatabase> available_dbs_ {};
    QSet<QString> used_names_ {};
};

#endif // PGCONNECTIONPOOL_H
