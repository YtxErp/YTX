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

#ifndef ENUMCLASS_H
#define ENUMCLASS_H

// Enum

enum UnitOrder { kUnitIM, kUnitMS, kUnitPEND };

enum UnitStakeholder { kUnitCust, kUnitEmp, kUnitVend, kUnitProd };

enum UnitProduct { kUnitPos = 1 };

enum NodeType { kTypeLeaf, kTypeBranch, kTypeSupport };

// Enum class defining sections
enum class Section { kFinance, kProduct, kTask, kStakeholder, kSales, kPurchase };

// for delegate
enum class Filter { kIncludeSpecific, kExcludeSpecific, kIncludeSpecificWithNone, kIncludeAllWithNone };

// Enum class defining trans columns
enum class TableEnum { kID, kDateTime, kLhsRatio, kCode, kDescription, kSupportID, kDocument, kState, kRhsNode };

enum class TableEnumFinance { kID, kDateTime, kLhsRatio, kCode, kDescription, kSupportID, kDocument, kState, kRhsNode, kDebit, kCredit, kSubtotal };

enum class TableEnumTask { kID, kDateTime, kUnitCost, kCode, kDescription, kSupportID, kDocument, kState, kRhsNode, kDebit, kCredit, kSubtotal };

enum class TableEnumProduct { kID, kDateTime, kUnitCost, kCode, kDescription, kSupportID, kDocument, kState, kRhsNode, kDebit, kCredit, kSubtotal };

enum class TableEnumStakeholder { kID, kDateTime, kUnitPrice, kCode, kDescription, kOutsideProduct, kDocument, kState, kInsideProduct };

enum class TableEnumOrder {
    kID,
    kInsideProduct,
    kOutsideProduct,
    kCode,
    kDescription,
    kColor,
    kFirst,
    kSecond,
    kUnitPrice,
    kGrossAmount,
    kDiscountPrice,
    kDiscount,
    kNetAmount
};

enum class TableEnumSearch {
    kID,
    kDateTime,
    kCode,
    kLhsNode,
    kLhsRatio,
    kLhsDebit,
    kLhsCredit,
    kDescription,
    kUnitPrice,
    kSupportID,
    kDiscountPrice,
    kSettled,
    kDocument,
    kState,
    kRhsCredit,
    kRhsDebit,
    kRhsRatio,
    kRhsNode
};

enum class TableEnumSupport {
    kID,
    kDateTime,
    kCode,
    kLhsNode,
    kLhsRatio,
    kLhsDebit,
    kLhsCredit,
    kDescription,
    kUnitPrice,
    kDocument,
    kState,
    kRhsCredit,
    kRhsDebit,
    kRhsRatio,
    kRhsNode
};

// Enum class defining node columns
enum class TreeEnum { kName, kID, kCode, kDescription, kNote, kRule, kType, kUnit };

enum class TreeEnumFinance { kName, kID, kCode, kDescription, kNote, kRule, kType, kUnit, kForeignTotal, kLocalTotal };

enum class TreeEnumTask { kName, kID, kCode, kDescription, kNote, kRule, kType, kUnit, kDateTime, kColor, kDocument, kFinished, kUnitCost, kQuantity, kAmount };

enum class TreeEnumProduct { kName, kID, kCode, kDescription, kNote, kRule, kType, kUnit, kColor, kUnitPrice, kCommission, kQuantity, kAmount };

enum class TreeEnumStakeholder { kName, kID, kCode, kDescription, kNote, kRule, kType, kUnit, kDeadline, kEmployee, kPaymentTerm, kTaxRate, kAmount };

enum class TreeEnumOrder {
    kName,
    kID,
    kParty,
    kDescription,
    kEmployee,
    kRule,
    kType,
    kUnit,
    kDateTime,
    kFirst,
    kSecond,
    kFinished,
    kGrossAmount,
    kDiscount,
    kNetAmount
};

enum class TreeEnumSearch {
    kName,
    kID,
    kCode,
    kDescription,
    kNote,
    kRule,
    kType,
    kUnit,
    kParty,
    kEmployee,
    kDateTime,
    kColor,
    kDocument,
    kFirst,
    kSecond,
    kDiscount,
    kFinished,
    kInitialTotal,
    kFinalTotal
};

// Enum class defining check options
enum class Check { kNone, kAll, kReverse };

#endif // ENUMCLASS_H
