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

#include "component/constantint.h"
#include "component/using.h"
#include "enum/entryenum.h"
#include "enum/section.h"
#include "enum/stateenum.h"

namespace utils {

inline QStringList ReadStringList(const QJsonObject& object, const QString& key)
{
    QStringList result {};
    const QJsonArray array { object.value(key).toArray() };
    for (const QJsonValue& value : array)
        result.append(value.toString());
    return result;
}

inline QJsonArray WriteStringList(const QStringList& list)
{
    QJsonArray array {};

    for (const auto& uuid : list) {
        array.append(uuid);
    }

    return array;
}
}

namespace entry {

constexpr int LinkedNodeColumn(Section section)
{
    switch (section) {
    case Section::kFinance:
        return std::to_underlying(EntryEnumF::kRhsNode);
    case Section::kTask:
    case Section::kInventory:
        return std::to_underlying(EntryEnum::kRhsNode);
    case Section::kPartner:
        return std::to_underlying(EntryEnumP::kRhsNode);
    case Section::kSale:
    case Section::kPurchase:
        return std::to_underlying(EntryEnumO::kRhsNode);
    }

    Q_UNREACHABLE();
}

constexpr int TagColumn(Section section)
{
    switch (section) {
    case Section::kFinance:
        return std::to_underlying(EntryEnumF::kTag);
    case Section::kTask:
    case Section::kInventory:
        return std::to_underlying(EntryEnum::kTag);
    case Section::kPartner:
        return std::to_underlying(EntryEnumP::kTag);
    case Section::kSale:
    case Section::kPurchase:
        return std::to_underlying(EntryEnumO::kTag);
    }

    Q_UNREACHABLE();
}

constexpr int DescriptionColumn(Section section)
{
    switch (section) {
    case Section::kFinance:
        return std::to_underlying(EntryEnumF::kDescription);
    case Section::kInventory:
    case Section::kTask:
        return std::to_underlying(EntryEnum::kDescription);
    case Section::kPartner:
        return std::to_underlying(EntryEnumP::kDescription);
    case Section::kSale:
    case Section::kPurchase:
        return std::to_underlying(EntryEnumO::kDescription);
    }

    Q_UNREACHABLE();
}

constexpr int IssuedTimeColumn(Section section)
{
    switch (section) {
    case Section::kFinance:
        return std::to_underlying(EntryEnumF::kIssuedTime);
    case Section::kInventory:
    case Section::kTask:
        return std::to_underlying(EntryEnum::kIssuedTime);
    case Section::kPartner:
        return std::to_underlying(EntryEnumP::kIssuedTime);
    case Section::kSale:
    case Section::kPurchase:
        // Sale/Purchase entries do not display issued_time column;
        // skip directly to rhs_node on new row insertion.
        return std::to_underlying(EntryEnumO::kRhsNode);
    }

    Q_UNREACHABLE();
}

constexpr int SearchDescriptionColumn(Section section)
{
    switch (section) {
    case Section::kFinance:
        return std::to_underlying(FullEntryEnumF::kDescription);
    case Section::kInventory:
    case Section::kTask:
        return std::to_underlying(FullEntryEnum::kDescription);
    case Section::kPartner:
        return std::to_underlying(EntryEnumP::kDescription);
    case Section::kSale:
    case Section::kPurchase:
        return std::to_underlying(EntryEnumO::kDescription);
    }

    Q_UNREACHABLE();
}

constexpr int BalanceColumn(Section section)
{
    switch (section) {
    case Section::kFinance:
        return std::to_underlying(EntryEnumF::kBalance);
    case Section::kTask:
    case Section::kInventory:
        return std::to_underlying(EntryEnum::kBalance);
    case Section::kPartner:
    case Section::kSale:
    case Section::kPurchase:
        return -1;
    }

    Q_UNREACHABLE();
}

constexpr int StatusColumn(Section section)
{
    switch (section) {
    case Section::kFinance:
        return std::to_underlying(EntryEnumF::kStatus);
    case Section::kTask:
    case Section::kInventory:
        return std::to_underlying(EntryEnum::kStatus);
    case Section::kPartner:
        return std::to_underlying(EntryEnumP::kStatus);
    case Section::kSale:
    case Section::kPurchase:
        return -1;
    }

    Q_UNREACHABLE();
}

constexpr std::pair<int, int> CacheColumnRange(Section section)
{
    switch (section) {
    case Section::kFinance:
        return { std::to_underlying(EntryEnumF::kIssuedTime), std::to_underlying(EntryEnumF::kStatus) };
    case Section::kInventory:
    case Section::kTask:
        return { std::to_underlying(EntryEnum::kIssuedTime), std::to_underlying(EntryEnum::kStatus) };
    case Section::kPartner:
        return { std::to_underlying(EntryEnumP::kIssuedTime), std::to_underlying(EntryEnumP::kExternalSku) };
    case Section::kSale:
    case Section::kPurchase:
        return { -1, -1 };
    }

    Q_UNREACHABLE();
}

constexpr std::pair<int, int> NumericColumnRange(Section section)
{
    switch (section) {
    case Section::kFinance:
        return { std::to_underlying(EntryEnumF::kDebit), std::to_underlying(EntryEnumF::kCredit) };
    case Section::kTask:
    case Section::kInventory:
        return { std::to_underlying(EntryEnum::kDebit), std::to_underlying(EntryEnum::kCredit) };
    case Section::kPartner:
        return { -1, -1 };
    case Section::kSale:
    case Section::kPurchase:
        return { std::to_underlying(EntryEnumO::kInitial), std::to_underlying(EntryEnumO::kInitial) };
    }

    Q_UNREACHABLE();
}

template <typename T, typename F = std::nullptr_t>
bool UpdateShadowIssuedTime(QJsonObject& update, T* object, CString& field, const QDateTime& value, QDateTime* T::* member, F&& restart_timer = nullptr)
{
    Q_ASSERT(object != nullptr);

    QDateTime* member_ptr { object->*member };
    if (!member_ptr) {
        qWarning() << "Member pointer must not be null.";
        return false;
    }

    if (*member_ptr == value)
        return false;

    *member_ptr = value;

    if (*object->sync_state == SyncState::kCreating) {
        return true;
    }

    update.insert(field, value.toUTC().toString(Qt::ISODate));

    if constexpr (!std::is_same_v<std::decay_t<F>, std::nullptr_t>) {
        restart_timer();
    }

    return true;
}

template <typename T, typename F = std::nullptr_t>
bool UpdateShadowStringList(QJsonObject& update, T* object, CString& field, const QStringList& value, QStringList* T::* member, F&& restart_timer = nullptr)
{
    Q_ASSERT(object != nullptr);

    QStringList* member_ptr { object->*member };

    if (!member_ptr) {
        qWarning() << "Member pointer must not be null.";
        return false;
    }

    if (*member_ptr == value)
        return false;

    *member_ptr = value;

    if (*object->sync_state == SyncState::kCreating) {
        return true;
    }

    update.insert(field, utils::WriteStringList(value));

    if constexpr (!std::is_same_v<std::decay_t<F>, std::nullptr_t>) {
        restart_timer();
    }

    return true;
}

template <typename Field, typename T, typename F = std::nullptr_t>
bool UpdateShadowField(QJsonObject& update, T* object, CString& field, const Field& value, Field* T::* member, F&& restart_timer = nullptr)
{
    Q_ASSERT(object != nullptr);

    Field* member_ptr { object->*member };

    if (!member_ptr) {
        qWarning() << "Member pointer must not be null.";
        return false;
    }

    if (*member_ptr == value)
        return false;

    *member_ptr = value;

    if (*object->sync_state == SyncState::kCreating) {
        return true;
    }

    update.insert(field, value);

    if constexpr (!std::is_same_v<std::decay_t<F>, std::nullptr_t>) {
        restart_timer();
    }

    return true;
}

template <typename T, typename F = std::nullptr_t>
bool UpdateIssuedTime(QJsonObject& update, T* object, CString& field, const QDateTime& value, QDateTime T::* member, F&& restart_timer = nullptr)
{
    Q_ASSERT(object != nullptr);

    QDateTime& current_value { object->*member };

    if (current_value == value)
        return false;

    current_value = value;

    if (object->sync_state == SyncState::kCreating) {
        return true;
    }

    update.insert(field, value.toUTC().toString(Qt::ISODate));

    if constexpr (!std::is_same_v<std::decay_t<F>, std::nullptr_t>) {
        restart_timer();
    }

    return true;
}

template <typename T, typename F = std::nullptr_t>
bool UpdateStringList(QJsonObject& update, T* object, CString& field, const QStringList& value, QStringList T::* member, F&& restart_timer = nullptr)
{
    Q_ASSERT(object != nullptr);

    QStringList& current_value { object->*member };

    if (current_value == value)
        return false;

    current_value = value;

    if (object->sync_state == SyncState::kCreating) {
        return true;
    }

    update.insert(field, utils::WriteStringList(value));

    if constexpr (!std::is_same_v<std::decay_t<F>, std::nullptr_t>) {
        restart_timer();
    }

    return true;
}

template <typename Field, typename T, typename F = std::nullptr_t>
bool UpdateField(QJsonObject& update, T* object, CString& field, const Field& value, Field T::* member, F&& restart_timer = nullptr)
{
    Q_ASSERT(object != nullptr);

    Field& current_value { object->*member };

    if (current_value == value)
        return false;

    current_value = value;

    if (object->sync_state == SyncState::kCreating) {
        return true;
    }

    update.insert(field, value);

    if constexpr (!std::is_same_v<std::decay_t<F>, std::nullptr_t>) {
        restart_timer();
    }

    return true;
}

template <typename Field, typename T, typename F = std::nullptr_t>
bool UpdateDouble(QJsonObject& update, T* object, CString& field, const Field& value, Field T::* member, F&& restart_timer = nullptr)
{
    Q_ASSERT(object != nullptr);

    Field& current_value { object->*member };

    if (qFuzzyCompare(current_value, value))
        return false;

    current_value = value;

    if (object->sync_state == SyncState::kCreating) {
        return true;
    }

    update.insert(field, QString::number(value, 'f', numeric_const::kDecimalPlaces8));

    if constexpr (!std::is_same_v<std::decay_t<F>, std::nullptr_t>) {
        restart_timer();
    }

    return true;
}

template <typename T, typename F = std::nullptr_t>
bool UpdateUuid(QJsonObject& update, T* object, CString& field, const QUuid& value, QUuid T::* member, F&& restart_timer = nullptr)
{
    Q_ASSERT(object != nullptr);

    QUuid& current_value { object->*member };

    if (current_value == value)
        return false;

    current_value = value;

    if (object->sync_state == SyncState::kCreating) {
        return true;
    }

    update.insert(field, value.toString(QUuid::WithoutBraces));

    if constexpr (!std::is_same_v<std::decay_t<F>, std::nullptr_t>) {
        restart_timer();
    }

    return true;
}
}
