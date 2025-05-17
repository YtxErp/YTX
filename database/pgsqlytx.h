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

class PgSqlYtx {
public:
    static bool NewFile(const QString& file_name);
    static bool IsPGServerAvailable(const QString& user, const QString& db_name, int timeout_ms);
    static QStringList GetAllDatabaseNames(const QString& user, const QString& db_name, int timeout_ms);

private:
    static QString NodeFinance();
    static QString NodeStakeholder();
    static QString NodeProduct();
    static QString NodeTask();
    static QString NodeOrder(const QString& order);

    static QString Path(const QString& table_name);

    static QString TransFinance();
    static QString TransTask();
    static QString TransProduct();
    static QString TransStakeholder();
    static QString TransOrder(const QString& order);

    static bool NodeIndex(QSqlQuery& query);
    // bool TransIndex(QSqlQuery& query);

    static QString SettlementOrder(const QString& order);
};

#endif // PGSQLYTX_H
