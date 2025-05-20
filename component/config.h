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

#ifndef CONFIG_H
#define CONFIG_H

#include <QString>

struct LicenseConfig {
    QString hardware_uuid {};
    QString activation_code {};
    QString activation_url {};
    bool is_activated {};
};

struct LoginConfig {
    QString host {};
    int port {};
    QString user {};
    QString password {};
    QString database {};
    bool is_saved {};
};

struct AppConfig {
    QString theme {};
    QString language {};
    QString separator {};
    QString printer {};

    // Equality operator overload to compare two AppConfig structs
    bool operator==(const AppConfig& other) const noexcept
    {
        return std::tie(theme, language, separator, printer) == std::tie(other.theme, other.language, other.separator, other.printer);
    }

    // Inequality operator overload to compare two AppConfig structs
    bool operator!=(const AppConfig& other) const noexcept { return !(*this == other); }
};

struct FileConfig {
    QString company_name {};

    // Equality operator overload to compare two FileConfig structs
    bool operator==(const FileConfig& other) const noexcept { return std::tie(company_name) == std::tie(other.company_name); }

    // Inequality operator overload to compare two FileConfig structs
    bool operator!=(const FileConfig& other) const noexcept { return !(*this == other); }
};

struct SectionConfig {
    QString static_label {};
    int static_node {};
    QString dynamic_label {};
    int dynamic_node_lhs {};
    QString operation {};
    int dynamic_node_rhs {};
    int default_unit {};
    QString document_path {};
    QString date_format {};
    int amount_decimal {};
    int common_decimal {};

    // Equality operator overload to compare two SectionConfig structs
    bool operator==(const SectionConfig& other) const noexcept
    {
        return std::tie(static_label, static_node, dynamic_label, dynamic_node_lhs, operation, dynamic_node_rhs, default_unit, document_path, date_format,
                   amount_decimal, common_decimal)
            == std::tie(other.static_label, other.static_node, other.dynamic_label, other.dynamic_node_lhs, other.operation, other.dynamic_node_rhs,
                other.default_unit, other.document_path, other.date_format, other.amount_decimal, other.common_decimal);
    }

    // Inequality operator overload to compare two SectionConfig structs
    bool operator!=(const SectionConfig& other) const noexcept { return !(*this == other); }
};

using CAppConfig = const AppConfig;
using CFileConfig = const FileConfig;
using CSectionConfig = const SectionConfig;

#endif // CONFIG_H
