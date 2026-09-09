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

#include <QDateTime>
#include <QJsonObject>
#include <QUuid>

#include "component/constant.h"

namespace statement {

struct PrimaryRow final {
    QUuid partner_id {};
    double count {};
    double measure {};
    double amount {};

    void Reset();
    void ReadJson(const QJsonObject& object);
};

inline void PrimaryRow::Reset() { *this = PrimaryRow {}; }

inline void PrimaryRow::ReadJson(const QJsonObject& object)
{
    if (const auto val = object.value(kPartnerId); val.isString())
        partner_id = QUuid(val.toString());
    if (const auto val = object.value(kCount); val.isString())
        count = val.toString().toDouble();
    if (const auto val = object.value(kMeasure); val.isString())
        measure = val.toString().toDouble();
    if (const auto val = object.value(kAmount); val.isString())
        amount = val.toString().toDouble();
}

struct SecondaryRow final {
    QDateTime issued_time {};
    QString code {};
    double count {};
    double measure {};
    double amount {};
    // NOTE: status is local only, not serialized
    int status {};
    QString description {};
    QUuid employee_id {};

    void Reset();
    void ReadJson(const QJsonObject& object);
};

inline void SecondaryRow::Reset() { *this = SecondaryRow {}; }

inline void SecondaryRow::ReadJson(const QJsonObject& object)
{
    if (const auto val = object.value(kIssuedTime); val.isString())
        issued_time = QDateTime::fromString(val.toString(), Qt::ISODate).toLocalTime();
    if (const auto val = object.value(kCount); val.isString())
        count = val.toString().toDouble();
    if (const auto val = object.value(kMeasure); val.isString())
        measure = val.toString().toDouble();
    if (const auto val = object.value(kAmount); val.isString())
        amount = val.toString().toDouble();
    if (const auto val = object.value(kDescription); val.isString())
        description = val.toString();
    if (const auto val = object.value(kCode); val.isString())
        code = val.toString();
    if (const auto val = object.value(kEmployeeId); val.isString())
        employee_id = QUuid(val.toString());
}

struct TertiaryRow final {
    QDateTime issued_time {};
    QString code {};
    QUuid internal_sku {};
    double count {};
    double measure {};
    double unit_price {};
    double amount {};
    // NOTE: status is local only, not serialized
    int status {};
    QString description {};

    void Reset();
    void ReadJson(const QJsonObject& object);
};

inline void TertiaryRow::Reset() { *this = TertiaryRow {}; }

inline void TertiaryRow::ReadJson(const QJsonObject& object)
{
    if (const auto val = object.value(kIssuedTime); val.isString())
        issued_time = QDateTime::fromString(val.toString(), Qt::ISODate).toLocalTime();
    if (const auto val = object.value(kInternalSku); val.isString())
        internal_sku = QUuid(val.toString());
    if (const auto val = object.value(kCount); val.isString())
        count = val.toString().toDouble();
    if (const auto val = object.value(kMeasure); val.isString())
        measure = val.toString().toDouble();
    if (const auto val = object.value(kUnitPrice); val.isString())
        unit_price = val.toString().toDouble();
    if (const auto val = object.value(kAmount); val.isString())
        amount = val.toString().toDouble();
    if (const auto val = object.value(kDescription); val.isString())
        description = val.toString();
    if (const auto val = object.value(kCode); val.isString())
        code = val.toString();
}

using CTertiaryList = const QList<TertiaryRow*>;

}
