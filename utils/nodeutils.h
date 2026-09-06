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

#include <QQueue>

#include "component/constantbool.h"
#include "component/constantint.h"
#include "component/using.h"
#include "enum/nodeenum.h"
#include "enum/section.h"
#include "tree/node.h"
#include "utils/entryutils.h"

template <typename T>
concept TreeNode = requires(T node) {
    { node.parent } -> std::same_as<T*&>;
    { node.children } -> std::same_as<QList<T*>&>;
};

template <typename T>
concept HasColor = requires(T node) { node.color; };

namespace node {

constexpr int DirectionRuleColumn(Section section)
{
    switch (section) {
    case Section::kFinance:
        return std::to_underlying(NodeEnumF::kDirectionRule);
    case Section::kTask:
        return std::to_underlying(NodeEnumT::kDirectionRule);
    case Section::kPartner:
        return -1;
    case Section::kInventory:
        return std::to_underlying(NodeEnumI::kDirectionRule);
    case Section::kSale:
    case Section::kPurchase:
        return std::to_underlying(NodeEnumO::kDirectionRule);
    }

    Q_UNREACHABLE();
}

constexpr int StatusColumn(Section section)
{
    switch (section) {
    case Section::kFinance:
    case Section::kTask:
    case Section::kInventory:
    case Section::kSale:
    case Section::kPurchase:
        return -1;
    case Section::kPartner:
        return std::to_underlying(NodeEnumP::kStatus);
    }

    Q_UNREACHABLE();
}

constexpr int DescriptionColumn(Section section)
{
    switch (section) {
    case Section::kFinance:
        return std::to_underlying(NodeEnumF::kDescription);
    case Section::kTask:
        return std::to_underlying(NodeEnumT::kDescription);
    case Section::kPartner:
        return std::to_underlying(NodeEnumP::kDescription);
    case Section::kInventory:
        return std::to_underlying(NodeEnumI::kDescription);
    case Section::kSale:
    case Section::kPurchase:
        return std::to_underlying(NodeEnumO::kDescription);
    }

    Q_UNREACHABLE();
}

constexpr int ColorColumn(Section section)
{
    switch (section) {
    case Section::kFinance:
        return std::to_underlying(NodeEnumF::kColor);
    case Section::kTask:
        return std::to_underlying(NodeEnumT::kColor);
    case Section::kPartner:
        return std::to_underlying(NodeEnumP::kColor);
    case Section::kInventory:
        return std::to_underlying(NodeEnumI::kColor);
    case Section::kSale:
    case Section::kPurchase:
        return -1;
    }

    Q_UNREACHABLE();
}

constexpr int TagColumn(Section section)
{
    switch (section) {
    case Section::kFinance:
        return std::to_underlying(NodeEnumF::kTag);
    case Section::kTask:
        return std::to_underlying(NodeEnumT::kTag);
    case Section::kInventory:
        return std::to_underlying(NodeEnumI::kTag);
    case Section::kPartner:
        return std::to_underlying(NodeEnumP::kTag);
    case Section::kSale:
    case Section::kPurchase:
        return std::to_underlying(NodeEnumO::kTag);
    }

    Q_UNREACHABLE();
}

constexpr std::pair<int, int> NumericColumnRange(Section section)
{
    switch (section) {
    case Section::kFinance:
        return { std::to_underlying(NodeEnumF::kInitialTotal), std::to_underlying(NodeEnumF::kFinalTotal) };
    case Section::kTask:
        return { std::to_underlying(NodeEnumT::kInitialTotal), std::to_underlying(NodeEnumT::kFinalTotal) };
    case Section::kPartner:
        return { std::to_underlying(NodeEnumP::kInitialTotal), std::to_underlying(NodeEnumP::kInitialTotal) };
    case Section::kInventory:
        return { std::to_underlying(NodeEnumI::kInitialTotal), std::to_underlying(NodeEnumI::kFinalTotal) };
    case Section::kSale:
    case Section::kPurchase:
        return { std::to_underlying(NodeEnumO::kCountTotal), std::to_underlying(NodeEnumO::kFinalTotal) };
    }

    Q_UNREACHABLE();
}

constexpr std::pair<int, int> CacheColumnRange(Section section)
{
    switch (section) {
    case Section::kFinance:
        return { std::to_underlying(NodeEnumF::kCode), std::to_underlying(NodeEnumF::kRoles) };
    case Section::kTask:
        return { std::to_underlying(NodeEnumT::kCode), std::to_underlying(NodeEnumT::kDocument) };
    case Section::kPartner:
        return { std::to_underlying(NodeEnumP::kCode), std::to_underlying(NodeEnumP::kStatus) };
    case Section::kInventory:
        return { std::to_underlying(NodeEnumI::kCode), std::to_underlying(NodeEnumI::kCommission) };
    case Section::kSale:
    case Section::kPurchase:
        return { std::to_underlying(NodeEnumO::kIssuedTime), std::to_underlying(NodeEnumO::kFinalTotal) };
    }

    Q_UNREACHABLE();
}

inline QString UnitString(NodeUnit unit)
{
    switch (unit) {
    case NodeUnit::OMonthly:
        return QObject::tr("MON");
    case NodeUnit::OImmediate:
        return QObject::tr("IMM");
    case NodeUnit::OPending:
        return QObject::tr("PEND");
    default:
        return QString();
    }
}

inline QString DirectionRuleString(bool direction_rule) { return direction_rule == direction_rule::kRO ? QObject::tr("RO") : QObject::tr("FO"); }

bool IsDescendant(const Node* descendant, const Node* ancestor);

// Update a QString or int field of an object and update the change in a QJsonObject.
// Returns true if the value was changed.
template <typename Field, typename T, typename F = std::nullptr_t>
bool UpdateField(QJsonObject& update, T* object, CString& field, const Field& value, Field T::* member, F&& restart_timer = nullptr)
{
    Q_ASSERT(object != nullptr);

    Field& current_value { object->*member };

    if (current_value == value)
        return false;

    current_value = value;
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
    update.insert(field, QString::number(value, 'f', numeric_const::kDecimalPlaces8));

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
    update.insert(field, utils::WriteStringList(value));

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
    update.insert(field, value.toString(QUuid::WithoutBraces));

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
    update.insert(field, value.toString(Qt::ISODate));

    if constexpr (!std::is_same_v<std::decay_t<F>, std::nullptr_t>) {
        restart_timer();
    }

    return true;
}

template <TreeNode T, typename F> void SortSubtree(T* node, F&& compare)
{
    Q_ASSERT(node != nullptr);

    QQueue<T*> queue;
    queue.enqueue(node);

    while (!queue.isEmpty()) {
        T* current = queue.dequeue();

        if (current->children.isEmpty())
            continue;

        std::ranges::sort(current->children, compare);

        for (T* child : std::as_const(current->children)) {
            queue.enqueue(child);
        }
    }
}

}

namespace utils {

inline std::tuple<int, int, int> ColorSortKey(const QString& color_name)
{
    const QColor color(color_name);

    // Invalid/empty colors sort first
    if (!color.isValid())
        return { -1, -1, -1 };

    int hue = color.hsvHue();

    // Qt returns -1 for grayscale colors
    // Put grayscale colors after normal hues
    if (hue < 0)
        hue = 360;

    return { hue,
        -color.hsvSaturation(), // vivid first
        color.value() };
}

inline double GrowthRate(double current, double previous)
{
    if (qFuzzyIsNull(previous))
        return std::numeric_limits<double>::quiet_NaN();

    return (current - previous) / std::abs(previous);
}

template <HasColor T> inline bool CompareColor(const T* lhs, const T* rhs, Qt::SortOrder order)
{
    Q_ASSERT(lhs != nullptr);
    Q_ASSERT(rhs != nullptr);

    const auto l_key = ColorSortKey(lhs->color);
    const auto r_key = ColorSortKey(rhs->color);

    return (order == Qt::AscendingOrder) ? (l_key < r_key) : (l_key > r_key);
}

};
