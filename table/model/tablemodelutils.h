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

#ifndef TABLEMODELUTILS_H
#define TABLEMODELUTILS_H

#include <QMutex>

#include "component/using.h"
#include "database/sqlite/sqlite.h"
#include "table/trans.h"

class TableModelUtils {
public:
    template <typename T>
    static bool UpdateField(Sqlite* sql, TransShadow* trans_shadow, CString& table, const T& value, CString& field, T* TransShadow::* member,
        const std::function<void()>& action = {})
    {
        assert(sql && "Sqlite pointer is null");
        assert(trans_shadow && "TransShadow pointer is null");
        assert(member && "Member pointer is null");

        T*& member_ptr { std::invoke(member, trans_shadow) };

        assert(member_ptr && "Member pointer must not be null");

        if (*member_ptr == value)
            return false;

        *member_ptr = value;
        assert(trans_shadow->rhs_node && "Rhs_node pointer is null");

        if (*trans_shadow->rhs_node == 0)
            return false;

        sql->UpdateField(table, value, field, *trans_shadow->id);
        if (action)
            action();

        return true;
    }

    static void AccumulateSubtotal(QMutex& mutex, QList<TransShadow*>& trans_shadow_list, int start, bool rule);
    static double Balance(bool rule, double debit, double credit) { return (rule ? 1 : -1) * (credit - debit); };
    static bool UpdateRhsNode(TransShadow* trans_shadow, int value);
};

#endif // TABLEMODELUTILS_H
