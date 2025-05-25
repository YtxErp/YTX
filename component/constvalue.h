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

#ifndef CONSTVALUE_H
#define CONSTVALUE_H

// Coefficients
inline constexpr int kCoefficient8 = 8;
inline constexpr int kCoefficient16 = 16;

inline constexpr double kTolerance = 1e-9;

// Sizes & Limits
inline constexpr int kRowHeight = 24;
inline constexpr int kHundred = 100;
inline constexpr int kThreeThousand = 3000;
inline constexpr long long kBatchSize = 50;
inline constexpr long long kMaxRecentFile = 10;

// Constants for rule
inline constexpr bool kRuleIS = 0;
inline constexpr bool kRuleMS = 1;
inline constexpr bool kRuleDICD = 0;
inline constexpr bool kRuleDDCI = 1;

// Constants for datetime
inline constexpr char kDateTime[] = "date_time";
inline constexpr char kDateTimeFST[] = "yyyy-MM-dd HH:mm";
inline constexpr char kDateFST[] = "yyyy-MM-dd";
inline constexpr char kMonthFST[] = "yyyyMM";

inline constexpr char kFullWidthPeriod[] = u8"。";
inline constexpr char kHalfWidthPeriod[] = u8".";

// Punctuation symbols
inline constexpr char kColon[] = ":";
inline constexpr char kDash[] = "-";
inline constexpr char kSemicolon[] = ";";
inline constexpr char kSlash[] = "/";

// Settings keys
inline constexpr char kSeparator[] = "separator";

// Constants for operators
inline constexpr char kMinux[] = "-";
inline constexpr char kPlus[] = "+";

// Constants for table column check state, kState
inline constexpr char kCheck[] = "check";

// Constants for files' suffix
inline constexpr char kDotSuffixINI[] = ".ini";
inline constexpr char kDotSuffixLOCK[] = ".lock";
inline constexpr char kDotSuffixXLSX[] = ".xlsx";
inline constexpr char kDotSuffixYTX[] = ".ytx";

inline constexpr char kSuffixPERCENT[] = "%";
inline constexpr char kSuffixXLSX[] = "xlsx";

inline constexpr char kLicense[] = "license";
inline constexpr char kActivationCode[] = "activation_code";
inline constexpr char kSignatureCiphertext[] = "signature_ciphertext";
inline constexpr char kSignatureIV[] = "signature_iv";
inline constexpr char kSignatureTag[] = "signature_tag";

inline constexpr char kActivationUrl[] = "activation_url";
inline constexpr char kActivate[] = "activate";

inline constexpr char kSourceConnection[] = "source_connection";
inline constexpr char kDestinationConnection[] = "destination_connection";

// Constants for app's language
inline constexpr char kLanguage[] = "language";
inline constexpr char kEnUS[] = "en_US";
inline constexpr char kZhCN[] = "zh_CN";

inline constexpr char kMsgType[] = "msg_type";
inline constexpr char kMsgData[] = "msg_data";

// Constants for tree and table's column
inline constexpr char kAmount[] = "amount";
inline constexpr char kAncestor[] = "ancestor";
inline constexpr char kCode[] = "code";
inline constexpr char kColor[] = "color";
inline constexpr char kCommission[] = "commission";
inline constexpr char kDeadline[] = "deadline";
inline constexpr char kDescendant[] = "descendant";
inline constexpr char kDescription[] = "description";
inline constexpr char kDiscount[] = "discount";
inline constexpr char kDiscountPrice[] = "discount_price";
inline constexpr char kDistance[] = "distance";
inline constexpr char kDocument[] = "document";
inline constexpr char kEmployee[] = "employee";
inline constexpr char kFinished[] = "finished";
inline constexpr char kFirst[] = "first";
inline constexpr char kGrossAmount[] = "gross_amount";
inline constexpr char kInsideProduct[] = "inside_product";
inline constexpr char kName[] = "name";
inline constexpr char kNetAmount[] = "net_amount";
inline constexpr char kNodeID[] = "node/id";
inline constexpr char kNote[] = "note";
inline constexpr char kOutsideProduct[] = "outside_product";
inline constexpr char kParty[] = "party";
inline constexpr char kPaymentTerm[] = "payment_term";
inline constexpr char kRemoved[] = "removed";
inline constexpr char kRule[] = "rule";
inline constexpr char kSecond[] = "second";
inline constexpr char kSettlement[] = "settlement";
inline constexpr char kSettlementID[] = "settlement_id";
inline constexpr char kState[] = "state";
inline constexpr char kSupportID[] = "support_id";
inline constexpr char kTaxRate[] = "tax_rate";
inline constexpr char kNodeType[] = "node_type";
inline constexpr char kUnit[] = "unit";
inline constexpr char kUnitCost[] = "unit_cost";
inline constexpr char kUnitPrice[] = "unit_price";

// Constants for app's ini
inline constexpr char kFile[] = "file";
inline constexpr char kGeometry[] = "geometry";
inline constexpr char kMainwindow[] = "mainwindow";
inline constexpr char kPrinter[] = "printer";
inline constexpr char kSection[] = "section";
inline constexpr char kSolarizedDark[] = "Solarized Dark";
inline constexpr char kSplitter[] = "splitter";
inline constexpr char kStart[] = "start";
inline constexpr char kStartSection[] = "start/section";
inline constexpr char kTheme[] = "theme";

// Constants for file's ini
inline constexpr char kCompany[] = "company";
inline constexpr char kHeaderState[] = "header_state";
inline constexpr char kTabID[] = "tab_id";

inline constexpr char kAmountDecimal[] = "amount_decimal";
inline constexpr char kCommonDecimal[] = "common_decimal";
inline constexpr char kDateFormat[] = "date_format";
inline constexpr char kDefaultUnit[] = "default_unit";
inline constexpr char kDocumentPath[] = "document_path";
inline constexpr char kDynamicLabel[] = "dynamic_label";
inline constexpr char kDynamicNodeLhs[] = "dynamic_node_lhs";
inline constexpr char kDynamicNodeRhs[] = "dynamic_node_rhs";
inline constexpr char kOperation[] = "operation";
inline constexpr char kStaticLabel[] = "static_label";
inline constexpr char kStaticNode[] = "static_node";

// Constants for others
inline constexpr char kQSQLITE[] = "QSQLITE";
inline constexpr char kYTX[] = "ytx";

inline constexpr char kLogin[] = "login";
inline constexpr char kHost[] = "host";
inline constexpr char kPort[] = "port";
inline constexpr char kUser[] = "user";
inline constexpr char kPassword[] = "password";
inline constexpr char kDatabase[] = "database";
inline constexpr char kIsSaved[] = "is_saved";

// Constants for sections
inline constexpr char kFinance[] = "finance";
inline constexpr char kFinancePath[] = "finance_path";
inline constexpr char kFinanceTrans[] = "finance_transaction";

inline constexpr char kProduct[] = "product";
inline constexpr char kProductPath[] = "product_path";
inline constexpr char kProductTrans[] = "product_transaction";

inline constexpr char kPurchase[] = "purchase";
inline constexpr char kPurchasePath[] = "purchase_path";
inline constexpr char kPurchaseSettlement[] = "purchase_settlement";
inline constexpr char kPurchaseTrans[] = "purchase_transaction";

inline constexpr char kSales[] = "sales";
inline constexpr char kSalesPath[] = "sales_path";
inline constexpr char kSalesSettlement[] = "sales_settlement";
inline constexpr char kSalesTrans[] = "sales_transaction";

inline constexpr char kStakeholder[] = "stakeholder";
inline constexpr char kStakeholderPath[] = "stakeholder_path";
inline constexpr char kStakeholderTrans[] = "stakeholder_transaction";

inline constexpr char kTask[] = "task";
inline constexpr char kTaskPath[] = "task_path";
inline constexpr char kTaskTrans[] = "task_transaction";

#endif // CONSTVALUE_H
