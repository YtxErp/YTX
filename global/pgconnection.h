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

#include <QSqlDatabase>
#include <QString>

#include "component/using.h"

class PGConnection {
public:
    static PGConnection& Instance();
    bool InitConnection(CString& user, CString& password, CString& db_name);
    QSqlDatabase* GetConnection();

private:
    PGConnection();
    ~PGConnection();

    PGConnection(const PGConnection&) = delete;
    PGConnection& operator=(const PGConnection&) = delete;
    PGConnection(PGConnection&&) = delete;
    PGConnection& operator=(PGConnection&&) = delete;

private:
    QSqlDatabase db_;
    bool is_initialized_ { false };
};

#endif // PGCONNECTION
