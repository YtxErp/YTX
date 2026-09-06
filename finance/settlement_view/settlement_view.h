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

#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QUuid>

#include "component/constant.h"

namespace settlement_view {

struct Row final {
    QUuid partner_id {};

    double previous_balance {};
    QList<double> months {};
    double current_amount {};
    double current_settled {};
    double current_unsettled {};
    double current_balance {};

    void Reset() { *this = Row {}; }
    void ReadJson(const QJsonObject& object);
};

enum class ColumnType {
    kPartner,
    kPreviousBalance,
    kMonth,
    kCurrentAmount,
    kCurrentSettled,
    kCurrentUnsettled,
    kCurrentBalance,
};

struct Column final {
    ColumnType type {};
    QString title {};
    int month_index { -1 };
};

inline void Row::ReadJson(const QJsonObject& object)
{
    if (const auto val = object.value(kPartnerId); val.isString())
        partner_id = QUuid(val.toString());

    if (const auto val = object.value(kPBalance); val.isString())
        previous_balance = val.toString().toDouble();

    if (const auto val = object.value(kMonths); val.isArray()) {
        const auto array { val.toArray() };
        months.reserve(array.size());

        for (const auto& value : array) {
            if (value.isString())
                months.append(value.toString().toDouble());
        }
    }

    if (const auto val = object.value(kCAmount); val.isString())
        current_amount = val.toString().toDouble();

    if (const auto val = object.value(kCSettled); val.isString())
        current_settled = val.toString().toDouble();

    if (const auto val = object.value(kCUnsettled); val.isString())
        current_unsettled = val.toString().toDouble();

    if (const auto val = object.value(kCBalance); val.isString())
        current_balance = val.toString().toDouble();
}

}