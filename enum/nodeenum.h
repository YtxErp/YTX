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

/** @brief Kind of node in a tree structure. */
enum class NodeKind { kLeaf = 0, kBranch };

/** @brief NodeUnit, global unit identifier across ERP. */
/**
 * 0–499   : Financial units (Currency)
 * 500–599 : Task units
 * 600–699 : Inventory units
 * 700–799 : Partner units
 * 800–899 : Order units
 *
 * WARNING: Do NOT modify existing values.
 * These identifiers must remain in sync with the server.
 */
enum class NodeUnit : int {
    // Financial units (Currency), 0–499
    USD = 0, // US Dollar
    EUR = 1, // Euro
    JPY = 2, // Japanese Yen
    GBP = 3, // British Pound
    AUD = 4, // Australian Dollar
    CAD = 5, // Canadian Dollar
    CHF = 6, // Swiss Franc
    CNY = 7, // Chinese Yuan
    HKD = 8, // Hong Kong Dollar
    NZD = 9, // New Zealand Dollar
    SEK = 10, // Swedish Krona
    NOK = 11, // Norwegian Krone
    SGD = 12, // Singapore Dollar
    KRW = 13, // South Korean Won
    MXN = 14, // Mexican Peso
    INR = 15, // Indian Rupee

    // Task units, 500–599
    TTarget = 500, // Task Target
    TOrigin = 510, // Task Origin
    TAction = 520, // Task Action

    // Inventory units, 600–699
    IItem = 600, // Inventory Item
    IPosition = 610, // Inventory Position

    // Partner units, 700–799
    PCustomer = 700, // Partner Customer
    PVendor = 710, // Partner Vendor
    PEmployee = 720, // Partner Employee

    // Order units, 800–899
    OImmediate = 800, // Order Immediate
    OMonthly = 810, // Order Monthly
    OPending = 820, // Order Pending
};

inline const char* kUnitCode(NodeUnit c)
{
    switch (c) {
    // Finance units
    case NodeUnit::USD:
        return "USD";
    case NodeUnit::EUR:
        return "EUR";
    case NodeUnit::JPY:
        return "JPY";
    case NodeUnit::GBP:
        return "GBP";
    case NodeUnit::AUD:
        return "AUD";
    case NodeUnit::CAD:
        return "CAD";
    case NodeUnit::CHF:
        return "CHF";
    case NodeUnit::CNY:
        return "CNY";
    case NodeUnit::HKD:
        return "HKD";
    case NodeUnit::NZD:
        return "NZD";
    case NodeUnit::SEK:
        return "SEK";
    case NodeUnit::NOK:
        return "NOK";
    case NodeUnit::SGD:
        return "SGD";
    case NodeUnit::KRW:
        return "KRW";
    case NodeUnit::MXN:
        return "MXN";
    case NodeUnit::INR:
        return "INR";

    // Task units
    case NodeUnit::TTarget:
        return "TGT";
    case NodeUnit::TAction:
        return "ACT";
    case NodeUnit::TOrigin:
        return "ORG";

    // Inventory units
    case NodeUnit::IItem:
        return "ITM";
    case NodeUnit::IPosition:
        return "POS";

    // Partner units
    case NodeUnit::PCustomer:
        return "CUS";
    case NodeUnit::PEmployee:
        return "EMP";
    case NodeUnit::PVendor:
        return "VEN";

    // Order units
    case NodeUnit::OImmediate:
        return "IMM";
    case NodeUnit::OMonthly:
        return "MON";
    case NodeUnit::OPending:
        return "PEN";
    }
}

inline const char* kUnitSymbol(NodeUnit c)
{
    switch (c) {
    // Financial units (Currency)
    case NodeUnit::USD:
        return "$";
    case NodeUnit::EUR:
        return "€";
    case NodeUnit::JPY:
        return "¥";
    case NodeUnit::GBP:
        return "£";
    case NodeUnit::AUD:
        return "$";
    case NodeUnit::CAD:
        return "$";
    case NodeUnit::CHF:
        return "CHF";
    case NodeUnit::CNY:
        return "¥";
    case NodeUnit::HKD:
        return "$";
    case NodeUnit::NZD:
        return "$";
    case NodeUnit::SEK:
        return "kr";
    case NodeUnit::NOK:
        return "kr";
    case NodeUnit::SGD:
        return "$";
    case NodeUnit::KRW:
        return "₩";
    case NodeUnit::MXN:
        return "$";
    case NodeUnit::INR:
        return "₹";

    // Task units
    case NodeUnit::TTarget:
        return "◎";
    case NodeUnit::TOrigin:
        return "◉";
    case NodeUnit::TAction:
        return "→";

    // Inventory units
    case NodeUnit::IItem:
        return "▣";
    case NodeUnit::IPosition:
        return "⌖";

    // Partner units
    case NodeUnit::PCustomer:
        return "◇";
    case NodeUnit::PEmployee:
        return "◈";
    case NodeUnit::PVendor:
        return "◆";

    // Order units
    case NodeUnit::OImmediate:
        return "‼";
    case NodeUnit::OMonthly:
        return "◷";
    case NodeUnit::OPending:
        return "…";
    }
}

// defining node column
enum class NodeEnum {
    kName = 0,
};

enum class NodeEnumF {
    kName = 0,
    kCode,
    kDescription,
    kTag,
    kColor,
    kDocument,
    kRoles,
    kDirectionRule,
    kKind,
    kUnit,
    kInitialTotal,
    kFinalTotal,
};

enum class NodeEnumI {
    kName = 0,
    kCode,
    kDescription,
    kTag,
    kColor,
    kDocument,
    kUnitPrice,
    kCommission,
    kDirectionRule,
    kKind,
    kUnit,
    kInitialTotal,
    kFinalTotal,
};

enum class NodeEnumT {
    kName = 0,
    kCode,
    kDescription,
    kTag,
    kColor,
    kDocument,
    kDirectionRule,
    kKind,
    kUnit,
    kInitialTotal,
    kFinalTotal,
};

enum class NodeEnumP {
    kName = 0,
    kCode,
    kDescription,
    kTag,
    kColor,
    kDocument,
    kStatus,
    kKind,
    kUnit,
    kInitialTotal,
};

enum class NodeEnumO {
    kName = 0,
    kIssuedTime,
    kEmployeeId,
    kStatus,
    kCode,
    kDescription,
    kTag,
    kDirectionRule,
    kKind,
    kUnit,
    kCountTotal,
    kMeasureTotal,
    kInitialTotal,
    kDiscountTotal,
    kFinalTotal,
};
