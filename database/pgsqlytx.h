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

#ifndef PGSQLYTX_H
#define PGSQLYTX_H

#include <QSqlDatabase>

#include "component/using.h"

class PgSqlYtx {
public:
    static bool IniRoleDatabase(CString super_user, CString super_password, CString new_user, CString new_password, CString db_name, int timeout_ms);
    static bool InitSchema(CString& user, CString& password, CString& db_name, int timeout_ms);
    static bool InitConnection(QSqlDatabase& db, CString& user, CString& password, CString& db_name, CString& connection_name, int timeout_ms);
    static void RemoveConnection(CString& connection_name);

private:
    static QString NodeFinance();
    static QString NodeStakeholder();
    static QString NodeProduct();
    static QString NodeTask();
    static QString NodeOrder(CString& order);

    static QString Path(CString& table_name);

    static QString TransFinance();
    static QString TransTask();
    static QString TransProduct();
    static QString TransStakeholder();
    static QString TransOrder(CString& order);

    static bool NodeIndex(QSqlQuery& query);

    static QString SettlementOrder(CString& order);

#if 0
    static bool IniPGData();
    static bool StartPGServer();
    static bool StopPGServer();

    static bool RunProcess(CString& program, const QStringList& arguments, QString* std_output = nullptr, QString* std_error = nullptr, int timeout_ms = 30000);
    static QString PgBinPath(CString& exe_name);

private:
    static CString kPgDataDir;
    static CString kPgBinBasePath;
#endif
};

#endif // PGSQLYTX_H
