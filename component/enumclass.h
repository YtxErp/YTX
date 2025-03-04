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

enum NodeType { kTypeLeaf, kTypeBranch, kTypeSupport };

// defining section
enum class Section { kFinance, kProduct, kTask, kStakeholder, kSales, kPurchase };

// Abbreviations: Finance -> F, Product -> P, Task -> T, Stakeholder -> S, Order -> O

// defining section's unit

enum class UnitO { kIS, kMS, kPEND };

enum class UnitS { kCust, kEmp, kVend };

enum class UnitP { kPos = 1 };

enum class UnitT { kProd = 1 };

// data filter
enum class Filter { kIncludeSpecific, kExcludeSpecific, kIncludeSpecificWithNone, kIncludeAllWithNone };

// defining trans column
enum class TableEnum { kDateTime, kID, kLhsRatio, kCode, kDescription, kSupportID, kDocument, kState, kRhsNode };

enum class TableEnumF { kDateTime, kID, kLhsRatio, kCode, kDescription, kSupportID, kDocument, kState, kRhsNode, kDebit, kCredit, kSubtotal };

enum class TableEnumT { kDateTime, kID, kUnitCost, kCode, kDescription, kSupportID, kDocument, kState, kRhsNode, kDebit, kCredit, kSubtotal };

enum class TableEnumP { kDateTime, kID, kUnitCost, kCode, kDescription, kSupportID, kDocument, kState, kRhsNode, kDebit, kCredit, kSubtotal };

enum class TableEnumS { kDateTime, kID, kUnitPrice, kCode, kDescription, kOutsideProduct, kDocument, kState, kInsideProduct };

enum class TableEnumO {
    kInsideProduct,
    kID,
    kOutsideProduct,
    kFirst,
    kSecond,
    kUnitPrice,
    kDiscountPrice,
    kDescription,
    kCode,
    kColor,
    kGrossAmount,
    kDiscount,
    kNetAmount
};

enum class TableEnumSearch {
    kDateTime,
    kID,
    kCode,
    kLhsNode,
    kLhsRatio,
    kLhsDebit,
    kLhsCredit,
    kDescription,
    kUnitPrice,
    kSupportID,
    kDiscountPrice,
    kDiscount,
    kDocument,
    kState,
    kRhsCredit,
    kRhsDebit,
    kRhsRatio,
    kRhsNode
};

enum class TableEnumRefFetcher {
    kDateTime,
    kLhsNode,
    kParty,
    kFirst,
    kSecond,
    kUnitPrice,
    kDiscountPrice,
    kCode,
    kDescription,
    kGrossAmount,
    kDiscount,
    kNetAmount
};

enum class TableEnumSupport {
    kDateTime,
    kID,
    kCode,
    kLhsNode,
    kLhsRatio,
    kLhsDebit,
    kLhsCredit,
    kDescription,
    kDocument,
    kState,
    kRhsCredit,
    kRhsDebit,
    kRhsRatio,
    kRhsNode
};

// defining node column
enum class TreeEnum { kName, kID, kCode, kDescription, kNote, kType, kRule, kUnit };

enum class TreeEnumF { kName, kID, kCode, kDescription, kNote, kType, kRule, kUnit, kForeignTotal, kLocalTotal };

enum class TreeEnumT { kName, kID, kCode, kDescription, kNote, kType, kRule, kUnit, kDateTime, kColor, kDocument, kFinished, kUnitCost, kQuantity, kAmount };

enum class TreeEnumP { kName, kID, kCode, kDescription, kNote, kType, kRule, kUnit, kColor, kUnitPrice, kCommission, kQuantity, kAmount };

enum class TreeEnumS { kName, kID, kCode, kDescription, kNote, kType, kRule, kUnit, kDeadline, kEmployee, kPaymentTerm, kTaxRate, kAmount };

enum class TreeEnumO {
    kName,
    kID,
    kParty,
    kDescription,
    kEmployee,
    kType,
    kRule,
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
    kType,
    kRule,
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
