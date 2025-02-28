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

#ifndef CLASSPARAMS_H
#define CLASSPARAMS_H

#include <QStandardItemModel>

#include "component/settings.h"
#include "database/sqlite/sqlite.h"
#include "table/model/tablemodel.h"
#include "tree/model/treemodel.h"
#include "tree/node.h"

struct EditNodeParamsFPTS {
    Node* node {};
    QStandardItemModel* unit_model {};
    CString& parent_path {};
    CStringList& name_list {};
    bool type_enable {};
    bool unit_enable {};
};

struct EditNodeParamsOrder {
    Node* node {};
    Sqlite* sql {};
    TableModel* order_table {};
    TreeModel* stakeholder_tree {};
    CSettings* settings {};
    Section section {};
};

using CEditNodeParamsFPTS = const EditNodeParamsFPTS;
using CEditNodeParamsOrder = const EditNodeParamsOrder;

#endif // CLASSPARAMS_H
