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

#ifndef TAB_H
#define TAB_H

#include <tuple>

#include "component/enumclass.h"

struct Tab {
    Section section {};
    int node_id {};

    // Equality operator overload to compare two Tab structs
    bool operator==(const Tab& other) const noexcept { return std::tie(section, node_id) == std::tie(other.section, other.node_id); }

    // Inequality operator overload to compare two Tab structs
    bool operator!=(const Tab& other) const noexcept { return !(*this == other); }
};

using CTab = const Tab;

#endif // TAB_H
