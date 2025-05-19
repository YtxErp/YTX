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

#ifndef PGCONNECTION
#define PGCONNECTION

#include <QMutex>
#include <QQueue>
#include <QSqlDatabase>
#include <QString>

class PGConnection {
public:
    static PGConnection& Instance();

    bool Init(const QString& user, const QString& password, const QString& db_name, int pool_size = 5);
    QSqlDatabase Acquire();
    void Release(const QSqlDatabase& db);

private:
    PGConnection();
    ~PGConnection();

    PGConnection(const PGConnection&) = delete;
    PGConnection& operator=(const PGConnection&) = delete;

private:
    QString user_ {};
    QString password_ {};
    QString db_name_ {};

    QMutex mutex_ {};
    QQueue<QSqlDatabase> available_dbs_ {};
    QSet<QString> used_names_ {};

    int connection_counter_ = 0;
};

#endif // PGCONNECTION
