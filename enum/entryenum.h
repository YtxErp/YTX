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

/** @brief Operation */
enum class MarkOperation { kClear = 0, kSelect, kToggle };

enum class NumericSide { kDebit, kCredit };

// defining entry column
enum class EntryEnum {
    // --- Hidden ---
    kLhsNode,
    // --- Visible ---
    kIssuedTime,
    kLhsRate,
    kCode,
    kDescription,
    kTag,
    kDocument,
    kStatus,
    kRhsNode,
    kDebit,
    kCredit,
    kBalance,
};

enum class EntryEnumF {
    // --- Hidden ---
    kLhsNode,
    // --- Visible ---
    kIssuedTime,
    kLhsRate,
    kCode,
    kDescription,
    kCashKind,
    kTag,
    kDocument,
    kStatus,
    kRhsNode,
    kDebit,
    kCredit,
    kBalance,
};

// FullEntryEnumP is same as EntryEnumP
enum class EntryEnumP {
    // --- Hidden ---
    kLhsNode,
    // --- Visible ---
    kIssuedTime,
    kCode,
    kDescription,
    kTag,
    kDocument,
    kStatus,
    kRhsNode,
    kUnitPrice,
    kExternalSku,
};

// FullEntryEnumO is same as EntryEnumO
enum class EntryEnumO {
    // --- Hidden ---
    kLhsNode,
    // --- Visible ---
    kRhsNode,
    kCount,
    kMeasure,
    kUnitPrice,
    kStatus,
    kDescription,
    kTag,
    kExternalSku,
    kInitial,
};

enum class FullEntryEnum {
    kIssuedTime,
    kCode,
    kLhsNode,
    kLhsRate,
    kLhsDebit,
    kLhsCredit,
    kDescription,
    kTag,
    kDocument,
    kStatus,
    kRhsCredit,
    kRhsDebit,
    kRhsRate,
    kRhsNode,
};

enum class FullEntryEnumF {
    kIssuedTime,
    kCode,
    kLhsNode,
    kLhsRate,
    kLhsDebit,
    kLhsCredit,
    kDescription,
    kCashKind,
    kTag,
    kDocument,
    kStatus,
    kRhsCredit,
    kRhsDebit,
    kRhsRate,
    kRhsNode,
};
