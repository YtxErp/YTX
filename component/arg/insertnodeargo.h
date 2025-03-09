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

#ifndef INSERTNODEARGO_H
#define INSERTNODEARGO_H

#include <QStandardItemModel>

#include "component/settings.h"
#include "database/sqlite/sqlite.h"
#include "table/model/transmodel.h"
#include "tree/model/nodemodel.h"
#include "tree/node.h"

struct InsertNodeArgO {
    Node* node {};
    Sqlite* sql {};
    TransModel* order_trans {};
    NodeModel* stakeholder_node {};
    const Settings* settings {};
    Section section {};
};

using CInsertNodeArgO = const InsertNodeArgO;

#endif // INSERTNODEARGO_H
