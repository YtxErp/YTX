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

#include <QString>
#include <QTime>

inline const QString kStrDDCI = QStringLiteral("DDCI");
inline const QString kStrDICD = QStringLiteral("DICD");

inline const QTime kStartTime = { 0, 0 }; // 00:00

// Empty & punctuation
inline const QString kColon = QStringLiteral(":");
inline const QString kDash = QStringLiteral("-");
inline const QString kHalfWidthPeriod = QStringLiteral(".");
inline const QString kFullWidthPeriod = QStringLiteral("。");
inline const QString kSlash = QStringLiteral("/");

inline const QString kSeparator = QStringLiteral("separator");
inline const QString kDeleteConfirm = QStringLiteral("delete_confirm");

// Operator symbols
inline const QString kMinus = QStringLiteral("-");
inline const QString kPlus = QStringLiteral("+");

// File suffixes
inline const QString kDotSuffixINI = QStringLiteral(".ini");
inline const QString kDotSuffixXLSX = QStringLiteral(".xlsx");

// Node kind
inline const QString kBranchKind = QStringLiteral("B");
inline const QString kLeafKind = QStringLiteral("L");

// App ini keys
inline const QString kGeometry = QStringLiteral("geometry");
inline const QString kTreeHeaderState = QStringLiteral("tree_header_state");
inline const QString kTableHeaderState = QStringLiteral("table_header_state");
inline const QString kMainwindow = QStringLiteral("mainwindow");
inline const QString kPrinter = QStringLiteral("printer");
inline const QString kCompanyName = QStringLiteral("company_name");
inline const QString kSection = QStringLiteral("section");
inline const QString kInputSide = QStringLiteral("input_side");
inline const QString kSplitter = QStringLiteral("splitter");
inline const QString kStart = QStringLiteral("start");
inline const QString kEnd = QStringLiteral("end");
inline const QString kYoyStart = QStringLiteral("yoy_start");
inline const QString kYoyEnd = QStringLiteral("yoy_end");
inline const QString kMomStart = QStringLiteral("mom_start");
inline const QString kMomEnd = QStringLiteral("mom_end");
inline const QString kStartSection = QStringLiteral("start/section");
inline const QString kTheme = QStringLiteral("theme");
inline const QString kWarningColor = QStringLiteral("#DC322F");

// Client-initiated data requests
inline const QString kLinkedEntry = QStringLiteral("linked_entry");

// Decimal config
inline const QString kAmountDecimal = QStringLiteral("amount_decimal");
inline const QString kRateDecimal = QStringLiteral("rate_decimal");
inline const QString kQuantityDecimal = QStringLiteral("quantity_decimal");

// Other ini keys
inline const QString kDateFormat = QStringLiteral("date_format");
inline const QString kDynamicLabel = QStringLiteral("dynamic_label");
inline const QString kDynamicNodeLhs = QStringLiteral("dynamic_node_lhs");
inline const QString kDynamicNodeRhs = QStringLiteral("dynamic_node_rhs");
inline const QString kOperation = QStringLiteral("operation");
inline const QString kStaticLabel = QStringLiteral("static_label");
inline const QString kStaticNode = QStringLiteral("static_node");

inline const QString kExternalSku = QStringLiteral("external_sku");
inline const QString kInternalSku = QStringLiteral("internal_sku");

inline const QString kSettlementItemDeselected = QStringLiteral("settlement_item_deselected");
inline const QString kSettlementItemSelected = QStringLiteral("settlement_item_selected");

inline const QString kNodeTotal = QStringLiteral("node_total");
inline const QString kDeletedEntryArray = QStringLiteral("deleted_entry_array");
inline const QString kInsertedEntryArray = QStringLiteral("inserted_entry_array");
inline const QString kUpdatedEntryArray = QStringLiteral("updated_entry_array");

// App language
inline const QString kEnUS = QStringLiteral("en_US");
inline const QString kLanguage = QStringLiteral("language");
inline const QString kZhCN = QStringLiteral("zh_CN");
inline const QString kSolarizedDark = QStringLiteral("Solarized Dark");
inline const QSet<QString> kSupportedLanguages { kZhCN, kEnUS };

// Database & connection
inline const QString kLogin = QStringLiteral("login");
inline const QString kEmail = QStringLiteral("email");
inline const QString kPassword = QStringLiteral("password");
inline const QString kKeyword = QStringLiteral("keyword");
inline const QString kRegister = QStringLiteral("register");
inline const QString kServer = QStringLiteral("server");
inline const QString kWorkspace = QStringLiteral("workspace");
inline const QString kHost = QStringLiteral("host");
inline const QString kNode = QStringLiteral("node");
inline const QString kNodeArray = QStringLiteral("node_array");
inline const QString kPath = QStringLiteral("path");
inline const QString kPathArray = QStringLiteral("path_array");
inline const QString kMeta = QStringLiteral("meta");
inline const QString kEntry = QStringLiteral("entry");
inline const QString kPort = QStringLiteral("port");
inline const QString kSourceConnection = QStringLiteral("source_connection");
inline const QString kUser = QStringLiteral("user");

inline const QString kEntryArray = QStringLiteral("entry_array");
inline const QString kUnsettledOrder = QStringLiteral("unsettled_order");
inline const QString kUpdateEntryDebit = QStringLiteral("update_entry_debit");
inline const QString kUpdateEntryCredit = QStringLiteral("update_entry_credit");

inline const QString kTotal = QStringLiteral("total");
inline const QString kTotalArray = QStringLiteral("total_array");
inline const QString kArray = QStringLiteral("array");

inline const QString kPBalance = QStringLiteral("previous_balance");
inline const QString kCAmount = QStringLiteral("current_amount");
inline const QString kCBalance = QStringLiteral("current_balance");
inline const QString kCSettled = QStringLiteral("current_settled");
inline const QString kCUnsettled = QStringLiteral("current_unsettled");
inline const QString kMonths = QStringLiteral("months");
inline const QString kValues = QStringLiteral("values");
inline const QString kLevel = QStringLiteral("level");
inline const QString kTargetField = QStringLiteral("target_field");

// Message fields
inline const QString kClosingNodeId = QStringLiteral("closing_node_id");
inline const QString kSummaryNodeId = QStringLiteral("summary_node_id");

// Tree & table columns
inline const QString kCode = QStringLiteral("code");
inline const QString kColor = QStringLiteral("color");
inline const QString kCommission = QStringLiteral("commission");
inline const QString kDescription = QStringLiteral("description");
inline const QString kDirectionRule = QStringLiteral("direction_rule");
inline const QString kName = QStringLiteral("name");
inline const QString kDocument = QStringLiteral("document");
inline const QString kCount = QStringLiteral("count");
inline const QString kCountTotal = QStringLiteral("count_total");
inline const QString kMeasure = QStringLiteral("measure");
inline const QString kAmount = QStringLiteral("amount");
inline const QString kMeasureTotal = QStringLiteral("measure_total");
inline const QString kUsername = QStringLiteral("username");
inline const QString kWorkspaceRole = QStringLiteral("workspace_role");
inline const QString kSectionPermissions = QStringLiteral("section_permissions");
inline const QString kRoles = QStringLiteral("roles");
inline const QString kCashKind = QStringLiteral("cash_kind");

inline const QString kId = QStringLiteral("id");
inline const QString kUserId = QStringLiteral("user_id");
inline const QString kSessionId = QStringLiteral("session_id");
inline const QString kEntryId = QStringLiteral("entry_id");
inline const QString kNodeId = QStringLiteral("node_id");
inline const QString kParentId = QStringLiteral("parent_id");
inline const QString kOrderId = QStringLiteral("order_id");
inline const QString kWidgetId = QStringLiteral("widget_id");
inline const QString kOldNodeId = QStringLiteral("old_node_id");
inline const QString kNewNodeId = QStringLiteral("new_node_id");
inline const QString kExpireTime = QStringLiteral("expire_time");
inline const QString kSettlement = QStringLiteral("settlement");
inline const QString kSettlementId = QStringLiteral("settlement_id");
inline const QString kIsSettled = QStringLiteral("is_settled");
inline const QString kUpdate = QStringLiteral("update");
inline const QString kResult = QStringLiteral("result");
inline const QString kLhsTotal = QStringLiteral("lhs_total");
inline const QString kRhsTotal = QStringLiteral("rhs_total");

inline const QString kLhsRate = QStringLiteral("lhs_rate");
inline const QString kRhsRate = QStringLiteral("rhs_rate");
inline const QString kLhsNumeric = QStringLiteral("lhs_numeric");
inline const QString kRhsNumeric = QStringLiteral("rhs_numeric");
inline const QString kLhsDebit = QStringLiteral("lhs_debit");
inline const QString kLhsCredit = QStringLiteral("lhs_credit");
inline const QString kRhsDebit = QStringLiteral("rhs_debit");
inline const QString kRhsCredit = QStringLiteral("rhs_credit");
inline const QString kTag = QStringLiteral("tag");
inline const QString kTagArray = QStringLiteral("tag_array");

inline const QString kInitial = QStringLiteral("initial");
inline const QString kInitialDelta = QStringLiteral("initial_delta");
inline const QString kInitialTotal = QStringLiteral("initial_total");

inline const QString kFinal = QStringLiteral("final");
inline const QString kFinalTotal = QStringLiteral("final_total");

inline const QString kPartnerId = QStringLiteral("partner_id");
inline const QString kEmployeeId = QStringLiteral("employee_id");

inline const QString kDiscount = QStringLiteral("discount");
inline const QString kUnitDiscount = QStringLiteral("unit_discount");
inline const QString kDiscountTotal = QStringLiteral("discount_total");

// Crockford Base32 alphabet (excludes I, L, O, U to avoid confusion)
inline constexpr char kBase32Crockford[] = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";

inline const QString kPasswordRemembered = QStringLiteral("password_remembered");

inline const QString kIssuedTime = QStringLiteral("issued_time");
inline const QString kStatus = QStringLiteral("status");
inline const QString kKind = QStringLiteral("kind");
inline const QString kNote = QStringLiteral("note");
inline const QString kPaymentTerm = QStringLiteral("payment_term");
inline const QString kRhsNode = QStringLiteral("rhs_node");
inline const QString kLhsNode = QStringLiteral("lhs_node");
inline const QString kState = QStringLiteral("state");
inline const QString kUnit = QStringLiteral("unit");
inline const QString kUnitCost = QStringLiteral("unit_cost");
inline const QString kUnitPrice = QStringLiteral("unit_price");

inline const QString kCreatedTime = QStringLiteral("created_time");
inline const QString kVersion = QStringLiteral("version");
inline const QString kAncestor = QStringLiteral("ancestor");
inline const QString kDescendant = QStringLiteral("descendant");

// Others
inline const QString kYTX = QStringLiteral("ytx");
inline const QString kUi = QStringLiteral("ui");
inline const QString kPrint = QStringLiteral("print");
inline const QString kExport = QStringLiteral("export");

// Constants for sections
inline const QString kFinance = QStringLiteral("finance");
inline const QString kFinanceNode = QStringLiteral("finance_node");
inline const QString kFinancePath = QStringLiteral("finance_path");
inline const QString kFinanceEntry = QStringLiteral("finance_entry");

inline const QString kInventory = QStringLiteral("inventory");
inline const QString kInventoryNode = QStringLiteral("inventory_node");
inline const QString kInventoryPath = QStringLiteral("inventory_path");
inline const QString kInventoryEntry = QStringLiteral("inventory_entry");

inline const QString kPurchase = QStringLiteral("purchase");
inline const QString kPurchaseNode = QStringLiteral("purchase_node");
inline const QString kPurchasePath = QStringLiteral("purchase_path");
inline const QString kPurchaseEntry = QStringLiteral("purchase_entry");

inline const QString kSale = QStringLiteral("sale");
inline const QString kSaleNode = QStringLiteral("sale_node");
inline const QString kSalePath = QStringLiteral("sale_path");
inline const QString kSaleEntry = QStringLiteral("sale_entry");

inline const QString kPartner = QStringLiteral("partner");
inline const QString kPartnerNode = QStringLiteral("partner_node");
inline const QString kPartnerPath = QStringLiteral("partner_path");
inline const QString kPartnerEntry = QStringLiteral("partner_entry");

inline const QString kTask = QStringLiteral("task");
inline const QString kTaskNode = QStringLiteral("task_node");
inline const QString kTaskPath = QStringLiteral("task_path");
inline const QString kTaskEntry = QStringLiteral("task_entry");
