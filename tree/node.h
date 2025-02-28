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

#ifndef NODE_H
#define NODE_H

#include <QList>
#include <QString>
#include <cmath>
#include <tuple>

inline constexpr double kTolerance = 1e-9;

struct Node {
    Node() = default;
    ~Node() = default;

    Node(const Node& other);
    Node& operator=(const Node& other);

    bool operator==(const Node& other) const noexcept;
    bool operator!=(const Node& other) const noexcept { return !(*this == other); }
    void Reset();

    Node(Node&&) noexcept = delete;
    Node& operator=(Node&&) noexcept = delete;

    QString name {};
    int id {};
    QString code {};
    QString description {};
    QString note {};
    QString date_time {};
    QString color {};
    QStringList document {};
    bool rule { false };
    int type {};
    int unit {};

    double first {};
    double second {};
    double discount {};
    bool finished {};

    // order and stakeholder
    int employee {};
    // order
    int party {};

    double final_total {};
    double initial_total {};

    Node* parent {};
    QList<Node*> children {};
};

inline Node::Node(const Node& other)
    : name(other.name)
    , id(other.id)
    , code(other.code)
    , description(other.description)
    , note(other.note)
    , date_time(other.date_time)
    , color(other.color)
    , document(other.document)
    , rule(other.rule)
    , type(other.type)
    , unit(other.unit)
    , first(other.first)
    , second(other.second)
    , discount(other.discount)
    , finished(other.finished)
    , employee(other.employee)
    , party(other.party)
    , final_total(other.final_total)
    , initial_total(other.initial_total)
{
}

inline Node& Node::operator=(const Node& other)
{
    if (this == &other)
        return *this;

    name = other.name;
    id = other.id;
    code = other.code;
    description = other.description;
    note = other.note;
    rule = other.rule;
    type = other.type;
    unit = other.unit;
    first = other.first;
    second = other.second;
    color = other.color;
    document = other.document;
    discount = other.discount;
    finished = other.finished;
    date_time = other.date_time;
    employee = other.employee;
    party = other.party;
    final_total = other.final_total;
    initial_total = other.initial_total;

    return *this;
}

inline bool Node::operator==(const Node& other) const noexcept
{
    auto AlmostEqual = [](double a, double b, double tolerance = kTolerance) noexcept { return std::abs(a - b) < tolerance; };

    // Compare non-floating-point data members
    bool basic_fields_equal
        = std::tie(name, id, code, party, employee, finished, date_time, color, document, description, note, rule, type, unit, parent, children)
        == std::tie(other.name, other.id, other.code, other.party, other.employee, other.finished, other.date_time, other.color, other.document,
            other.description, other.note, other.rule, other.type, other.unit, other.parent, other.children);

    // Compare floating-point data members, considering tolerance
    bool floating_fields_equal = AlmostEqual(first, other.first) && AlmostEqual(second, other.second) && AlmostEqual(discount, other.discount)
        && AlmostEqual(initial_total, other.initial_total) && AlmostEqual(final_total, other.final_total);

    return basic_fields_equal && floating_fields_equal;
}

inline void Node::Reset()
{
    name.clear();
    id = 0;
    code.clear();
    party = 0;
    employee = 0;
    second = 0.0;
    discount = 0.0;
    finished = false;
    first = 0.0;
    date_time.clear();
    description.clear();
    note.clear();
    color.clear();
    document.clear();
    rule = false;
    type = 0;
    unit = 0;
    final_total = 0.0;
    initial_total = 0.0;
    parent = nullptr;
    children.clear();
}

using NodeHash = QHash<int, Node*>;
using CNodeHash = const QHash<int, Node*>;

#endif // NODE_H
