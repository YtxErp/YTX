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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>

// Struct representing interface settings
struct Interface {
    QString theme {};
    QString language {};
    QString separator {};

    // Equality operator overload to compare two Interface structs
    bool operator==(const Interface& other) const noexcept
    {
        return std::tie(theme, language, separator) == std::tie(other.theme, other.language, other.separator);
    }

    // Inequality operator overload to compare two Interface structs
    bool operator!=(const Interface& other) const noexcept { return !(*this == other); }
};

// Struct representing section settings
struct Settings {
    QString static_label {};
    int static_node {};
    QString dynamic_label {};
    int dynamic_node_lhs {};
    QString operation {};
    int dynamic_node_rhs {};
    int default_unit {};
    QString document_dir {};
    QString date_format {};
    int amount_decimal {};
    int common_decimal {};

    // Equality operator overload to compare two Settings structs
    bool operator==(const Settings& other) const noexcept
    {
        return std::tie(static_label, static_node, dynamic_label, dynamic_node_lhs, operation, dynamic_node_rhs, default_unit, document_dir, date_format,
                   amount_decimal, common_decimal)
            == std::tie(other.static_label, other.static_node, other.dynamic_label, other.dynamic_node_lhs, other.operation, other.dynamic_node_rhs,
                other.default_unit, other.document_dir, other.date_format, other.amount_decimal, other.common_decimal);
    }

    // Inequality operator overload to compare two Settings structs
    bool operator!=(const Settings& other) const noexcept { return !(*this == other); }
};

using CInterface = const Interface;
using CSettings = const Settings;

#endif // SETTINGS_H
