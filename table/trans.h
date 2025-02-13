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

#ifndef TRANS_H
#define TRANS_H

#include <QStringList>

struct Trans {
    int id {};
    QString date_time {};
    QString code {};
    int lhs_node {};
    double lhs_ratio { 1.0 };
    double lhs_debit {};
    double lhs_credit {};
    QString description {};
    QStringList document {};
    bool state { false };
    double rhs_credit {};
    double rhs_debit {};
    double rhs_ratio { 1.0 };
    int rhs_node {};

    // order
    int support_id {};
    double discount_price {};
    double unit_price {};
    double net_amount {};

    void Reset()
    {
        id = 0;
        date_time.clear();
        code.clear();
        lhs_node = 0;
        lhs_ratio = 1.0;
        lhs_debit = 0.0;
        lhs_credit = 0.0;
        description.clear();
        rhs_node = 0;
        rhs_ratio = 1.0;
        rhs_debit = 0.0;
        rhs_credit = 0.0;
        state = false;
        document.clear();
        support_id = 0;
        discount_price = 0.0;
        unit_price = 0.0;
        net_amount = 0.0;
    }
};

struct TransShadow {
    int* id {};
    QString* date_time {};
    QString* code {};
    int* lhs_node {};
    double* lhs_ratio {};
    double* lhs_debit {};
    double* lhs_credit {};
    QString* description {};
    QStringList* document {};
    bool* state {};
    double* rhs_credit {};
    double* rhs_debit {};
    double* rhs_ratio {};
    int* rhs_node {};
    int* support_id {};
    double* discount_price {};
    double* unit_price {};
    double* net_amount {};

    double subtotal {};

    void Reset()
    {
        id = nullptr;
        date_time = nullptr;
        code = nullptr;
        lhs_node = nullptr;
        lhs_ratio = nullptr;
        lhs_debit = nullptr;
        lhs_credit = nullptr;
        description = nullptr;
        rhs_node = nullptr;
        rhs_ratio = nullptr;
        rhs_debit = nullptr;
        rhs_credit = nullptr;
        state = nullptr;
        document = nullptr;
        support_id = nullptr;
        discount_price = nullptr;
        unit_price = nullptr;
        net_amount = nullptr;

        subtotal = 0.0;
    }
};

using TransList = QList<Trans*>;
using TransShadowList = QList<TransShadow*>;

#endif // TRANS_H
