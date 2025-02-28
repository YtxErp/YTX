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
    static bool UpdateField(Sqlite* sql, TransShadow* trans_shadow, CString& table, CString& field, const T& value, T* TransShadow::* member,
        const std::function<void()>& action = {})
    {
        if (!sql || !trans_shadow || !member) {
            qWarning() << "Invalid input parameters: Sqlite, TransShadow or Member pointer is null.";
            return false;
        }

        T*& member_ptr { std::invoke(member, trans_shadow) };

        if (!member_ptr) {
            qWarning() << "Member pointer must not be null.";
            return false;
        }

        // Skip if value is the same as current value
        if (*member_ptr == value)
            return false;

        *member_ptr = value;

        // If lhs_node or rhs_node is invalid, skip updating the database
        if (!trans_shadow->lhs_node || !trans_shadow->rhs_node || *trans_shadow->lhs_node == 0 || *trans_shadow->rhs_node == 0) {
            return true; // Return without updating SQLite
        }

        try {
            sql->WriteField(table, field, value, *trans_shadow->id);
        } catch (const std::exception& e) {
            qWarning() << "Failed to update SQLite: " << e.what();
            return false;
        }

        // Execute additional action if provided
        if (action) {
            try {
                action();
            } catch (const std::exception& e) {
                qWarning() << "Failed to execute action: " << e.what();
                return false;
            }
        }

        return true;
    }

    static void AccumulateSubtotal(QMutex& mutex, QList<TransShadow*>& trans_shadow_list, int start, bool rule);
    static double Balance(bool rule, double debit, double credit) { return (rule ? 1 : -1) * (credit - debit); };
    static bool UpdateRhsNode(TransShadow* trans_shadow, int value);
};

#endif // TABLEMODELUTILS_H
