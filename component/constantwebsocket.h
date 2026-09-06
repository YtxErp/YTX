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

// ================= WS Key Naming Convention =================
//
// Format:
//   <subject>_<verb>
//   <subject>_<qualifier>_<verb>
//
// Subject:
//   - Use singular business noun (node, entry, settlement, tag)
//   - Lowercase snake_case
//
// Verb:
//   - Always base form: insert, update, delete, search, apply, recall, ack, notify, allow, deny
//   - Direction semantics (soft convention):
//       • apply -> server actively pushes this message to client
//       • ack   -> client sends request, server acknowledges
//       • notify -> server informs client of events (similar to apply)
//   - Other verbs follow context-dependent semantics
//
// Note:
//   Message direction (client/server) is context-dependent.
//   The same key may be used in both directions.
//
// Notes:
//   - Avoid tense variants (inserted / updating / deletion)
//   - Avoid plural subjects
//   - Key verb is suffix, handler function verb is prefix
//     e.g. "node_insert" -> InsertNode(), "login_notify" -> NotifyLogin()
// =============================================================
enum class WsKey : uint16_t {
    // --- Auth ---
    kLogin = 1000,
    kLoginOutcome = 1001,

    kRegister = 1010,
    kRegisterOutcome = 1011,

    // --- Config ---
    kDefaultUnitDeny = 1100,
    kDefaultUnitUpdate = 1101,

    kDocumentDirUpdate = 1110,

    kPartnerEntryApply = 1120,
    kSharedConfigApply = 1121,

    // --- Audit Log ---
    kAuditLogAck = 1200,

    // --- Settlement ---
    kSettlementPrimary = 1300,
    kSettlementDelete = 1301,
    kSettlementInsert = 1302,
    kSettlementSecondary = 1303,
    kSettlementRecall = 1304,
    kSettlementUpdate = 1305,
    kSettlementView = 1306,

    // --- Tree ---
    kLeafReplace = 1400,
    kBranchDelete = 1401,
    kLeafDelete = 1402,
    kPartnerLeafDelete = 1403,
    kOrderLeafDelete = 1404,

    kLeafDeleteCheck = 1410,
    kLeafDeleteAllow = 1411,
    kLeafDeleteDeny = 1412,

    kNodeDirectionRuleUpdate = 1420,
    kNodeDrag = 1421,
    kNodeInsert = 1422,
    kNodeNameUpdate = 1423,
    kNodeUpdate = 1424,
    kNodeStatus = 1425,

    kTreeApply = 1430,
    kTreeSyncFinish = 1431,

    // --- Tag ---
    kTagApply = 1500,
    kTagDelete = 1501,
    kTagInsert = 1502,
    kTagUpdate = 1503,

    // --- Entry ---
    kTableAck = 1600,
    kEntryInsert = 1601,
    kEntryUpdate = 1602,
    kEntryDelete = 1603,
    kEntriesMark = 1604,
    kEntryLinkedNodeUpdate = 1605,
    kEntryRateUpdate = 1606,
    kEntryNumericUpdate = 1607,

    kEntryDescriptionSearch = 1610,
    kEntryTagSearch = 1611,

    // --- Misc ---
    kOperationDeny = 1700,
    kPermissionDeny = 1710,

    // --- Profile ---
    kAccountNameUpdate = 1800,
    kAccountUsernameUpdate = 1801,

    // --- Workspace Member / Role ---
    kAccountRoleDelete = 1900,
    kAccountRoleUpdate = 1901,

    kWorkspaceMemberAck = 1910,
    kWorkspaceMemberDelete = 1911,
    kWorkspaceMemberUpdate = 1912,

    // --- Statement ---
    kStatementPrimary = 2000,
    kStatementSecondary = 2001,
    kStatementTertiary = 2002,

    // --- Order Delivery ---
    kBindingRequest = 2100,
    kBindingRequestOutcome = 2101,

    kOrderDeliver = 2112,
    kOrderDeliverOutcome = 2113,
    kOrderNotify = 2114,
    kOrderFetch = 2115,
    kOrderDetail = 2116,
    kOrderAccept = 2117,

    // --- Finance ---
    kBalanceSheetAck = 2200,
    kCashFlowStatementAck = 2201,
    kIncomeStatementAck = 2202,

    kPeriodClose = 2210,

    // --- Task (2300-2399) ---
    // reserved

    // --- Inventory ---
    kInventoryHeatAck = 2400,

    // --- Partner ---
    kPartnerEntryDelete = 2500,
    kPartnerEntryInsert = 2501,
    kPartnerNodeInsert = 2502,

    kPartnerHeatAck = 2510,

    // --- Order ---
    kOrderInsertSave = 2600,
    kOrderInsertRelease = 2601,
    kUnreleasedOrderSave = 2602,
    kUnreleasedOrderRelease = 2603,
    kOrderRecall = 2604,
    kRecalledOrderSave = 2605,
    kRecalledOrderRelease = 2606,

    kOrderNameSearch = 2610,
    kOrderTagSearch = 2611,

    kOrderNodeAck = 2620,
    kOrderTreeAck = 2621,
    kOrderHistory = 2622,
};

constexpr const char* WsMsgToString(WsKey msg)
{
    switch (msg) {
    // --- Auth ---
    case WsKey::kLogin:
        return "kLogin";
    case WsKey::kRegister:
        return "kRegister";
    case WsKey::kLoginOutcome:
        return "kLoginOutcome";
    case WsKey::kRegisterOutcome:
        return "kRegisterOutcome";

        // --- Config ---
    case WsKey::kSharedConfigApply:
        return "kSharedConfigApply";
    case WsKey::kPartnerEntryApply:
        return "kPartnerEntryApply";
    case WsKey::kDefaultUnitUpdate:
        return "kDefaultUnitUpdate";
    case WsKey::kDefaultUnitDeny:
        return "kDefaultUnitDeny";
    case WsKey::kDocumentDirUpdate:
        return "kDocumentDirUpdate";

        // --- Ack ---
    case WsKey::kOrderTreeAck:
        return "kOrderTreeAck";
    case WsKey::kTableAck:
        return "kTableAck";
    case WsKey::kOrderHistory:
        return "kOrderHistory";
    case WsKey::kStatementPrimary:
        return "kStatementPrimary";
    case WsKey::kStatementSecondary:
        return "kStatementSecondary";
    case WsKey::kStatementTertiary:
        return "kStatementTertiary";
    case WsKey::kSettlementPrimary:
        return "kSettlementPrimary";
    case WsKey::kSettlementSecondary:
        return "kSettlementSecondary";
    case WsKey::kOrderNodeAck:
        return "kOrderNodeAck";

        // --- Settlement ---
    case WsKey::kSettlementInsert:
        return "kSettlementInsert";
    case WsKey::kSettlementUpdate:
        return "kSettlementUpdate";
    case WsKey::kSettlementRecall:
        return "kSettlementRecall";
    case WsKey::kSettlementDelete:
        return "kSettlementDelete"; // no handler needed
    case WsKey::kSettlementView:
        return "kSettlementView";

        // --- Tree ---
    case WsKey::kTreeApply:
        return "kTreeApply";
    case WsKey::kTreeSyncFinish:
        return "kTreeSyncFinish";
    case WsKey::kNodeInsert:
        return "kNodeInsert";
    case WsKey::kNodeUpdate:
        return "kNodeUpdate";
    case WsKey::kNodeStatus:
        return "kNodeStatus";
    case WsKey::kOrderNameSearch:
        return "kOrderNameSearch";
    case WsKey::kNodeNameUpdate:
        return "kNodeNameUpdate";
    case WsKey::kNodeDrag:
        return "kNodeDrag";
    case WsKey::kLeafDelete:
        return "kLeafDelete";
    case WsKey::kPartnerLeafDelete:
        return "kLeafDeletePartner";
    case WsKey::kOrderLeafDelete:
        return "kLeafDeleteOrder";
    case WsKey::kLeafReplace:
        return "kLeafReplace";
    case WsKey::kBranchDelete:
        return "kBranchDelete";
    case WsKey::kNodeDirectionRuleUpdate:
        return "kNodeDirectionRuleUpdate";

        // --- Leaf delete flow ---
    case WsKey::kLeafDeleteCheck:
        return "kLeafDeleteCheck";
    case WsKey::kLeafDeleteDeny:
        return "kLeafDeleteDeny";
    case WsKey::kLeafDeleteAllow:
        return "kLeafDeleteAllow";

        // --- Tag ---
    case WsKey::kTagApply:
        return "kTagApply";
    case WsKey::kTagInsert:
        return "kTagInsert";
    case WsKey::kTagUpdate:
        return "kTagUpdate";
    case WsKey::kTagDelete:
        return "kTagDelete";

        // --- Entry ---
    case WsKey::kEntryInsert:
        return "kEntryInsert";
    case WsKey::kEntryUpdate:
        return "kEntryUpdate";
    case WsKey::kEntryDelete:
        return "kEntryDelete";
    case WsKey::kEntryDescriptionSearch:
        return "kEntryDescriptionSearch";
    case WsKey::kEntryTagSearch:
        return "kEntryTagSearch";
    case WsKey::kEntriesMark:
        return "kEntriesMark";
    case WsKey::kEntryLinkedNodeUpdate:
        return "kEntryLinkedNodeUpdate";
    case WsKey::kEntryRateUpdate:
        return "kEntryRateUpdate";
    case WsKey::kEntryNumericUpdate:
        return "kEntryNumericUpdate";

        // --- Order ---
    case WsKey::kOrderInsertSave:
        return "kOrderInsertSave";
    case WsKey::kUnreleasedOrderSave:
        return "kUnreleasedOrderSave";
    case WsKey::kOrderInsertRelease:
        return "kOrderInsertRelease";
    case WsKey::kUnreleasedOrderRelease:
        return "kUnreleasedOrderRelease";
    case WsKey::kOrderRecall:
        return "kOrderRecall";
    case WsKey::kRecalledOrderSave:
        return "kRecalledOrderSave";
    case WsKey::kRecalledOrderRelease:
        return "kRecalledOrderRelease";

        // --- Misc ---
    case WsKey::kOperationDeny:
        return "kOperationDeny";
    case WsKey::kPermissionDeny:
        return "kPermissionDeny";

        // --- Profile ---
    case WsKey::kAccountNameUpdate:
        return "kAccountNameUpdate";
    case WsKey::kAccountUsernameUpdate:
        return "kAccountUsernameUpdate";

        // --- Workspace Member ---
    case WsKey::kWorkspaceMemberAck:
        return "kWorkspaceMemberAck";
    case WsKey::kWorkspaceMemberUpdate:
        return "kWorkspaceMemberUpdate";
    case WsKey::kWorkspaceMemberDelete:
        return "kWorkspaceMemberDelete";
    case WsKey::kAccountRoleUpdate:
        return "kAccountRoleUpdate";
    case WsKey::kAccountRoleDelete:
        return "kAccountRoleDelete";
    case WsKey::kAuditLogAck:
        return "kAuditLogAck";
    case WsKey::kOrderTagSearch:
        return "kOrderTagSearch";
    case WsKey::kPeriodClose:
        return "kPeriodClose";
    case WsKey::kInventoryHeatAck:
        return "kInventoryHeatAck";
    case WsKey::kPartnerHeatAck:
        return "kPartnerHeatAck";
    case WsKey::kBalanceSheetAck:
        return "kBalanceSheetAck";
    case WsKey::kIncomeStatementAck:
        return "kIncomeStatementAck";
    case WsKey::kCashFlowStatementAck:
        return "kCashFlowStatementAck";

    case WsKey::kPartnerEntryDelete:
        return "kPartnerEntryDelete";
    case WsKey::kPartnerEntryInsert:
        return "kPartnerEntryInsert";

        // --- Order delivery binding ---
    case WsKey::kBindingRequest:
        return "kBindingRequest";
    case WsKey::kBindingRequestOutcome:
        return "kBindingRequestOutcome";
        // --- Order delivery ---
    case WsKey::kOrderDeliver:
        return "kOrderDeliver";
    case WsKey::kOrderDeliverOutcome:
        return "kOrderDeliverOutcome";
    case WsKey::kOrderNotify:
        return "kOrderNotify";
    case WsKey::kOrderFetch:
        return "kOrderFetch";
    case WsKey::kOrderDetail:
        return "kOrderDetail";
    case WsKey::kOrderAccept:
        return "kOrderAccept";
    case WsKey::kPartnerNodeInsert:
        return "kPartnerNodeInsert";
    }
}

namespace WsField {
inline const QString kMarkOperation { QStringLiteral("mark_operation") };
inline const QString kNodeUpdate { QStringLiteral("node_update") };
inline const QString kDefaultUnit { QStringLiteral("default_unit") };
inline const QString kDocumentDir { QStringLiteral("document_dir") };
inline const QString kKey { QStringLiteral("key") };
inline const QString kValue { QStringLiteral("value") };
}
