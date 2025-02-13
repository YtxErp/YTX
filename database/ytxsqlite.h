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

#ifndef YTXSQLITE_H
#define YTXSQLITE_H

#include <QSqlDatabase>

#include "component/enumclass.h"
#include "component/settings.h"
#include "component/using.h"

class YtxSqlite {
public:
    YtxSqlite() = default;
    explicit YtxSqlite(Section section);

    void QuerySettings(Settings& settings, Section section);
    void UpdateSettings(CSettings& settings, Section section);
    void NewFile(CString& file_path);

private:
    QString NodeFinance();
    QString NodeStakeholder();
    QString NodeProduct();
    QString NodeTask();
    QString NodeOrder(CString& order);

    QString Path(CString& table_name);

    QString TransFinance();
    QString TransTask();
    QString TransProduct();
    QString TransStakeholder();
    QString TransOrder(CString& order);

private:
    QSqlDatabase* db_ {};
};

#endif // YTXSQLITE_H
