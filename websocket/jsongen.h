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

#include "component/using.h"
#include "enum/bindingmode.h"
#include "enum/section.h"
#include "tag/tagrow.h"
#include "tree/node.h"
#include "utils/daterange.h"

// Only client-owned data is included in the request.
// Server-owned fields are computed and filled by the server.

namespace JsonGen {

QJsonObject Login(CString& email, CString& password, CString& workspace, const utils::DateTimeRange& range);
QJsonObject Register(CString& email, CString& password);

QJsonObject NodeInsert(Section section, const Node* node, CUuid& parent_id);
QJsonObject NodeDrag(Section section, CUuid& node_id, CUuid& parent_id);

QJsonObject LeafDelete(Section section, CUuid& node_id, int version);
QJsonObject BranchDelete(Section section, CUuid& node_id, CUuid& parent_id, int version);

QJsonObject LeafDeleteCheck(Section section, CUuid& node_id, int version);
QJsonObject LeafReplace(Section section, CUuid& old_id, CUuid& new_id, int version);

QJsonObject MarkEntries(Section section, CUuid& node_id, int operation);
QJsonObject EntryUpdate(Section section, CUuid& entry_id, CJsonObject& update, int version);
QJsonObject EntryValue(Section section, CUuid& entry_id, CJsonObject& update, InputSide side, int version);
QJsonObject EntryMessage(Section section, CUuid& entry_id);

QJsonObject TagUpdate(Section section, CUuid& id, CJsonObject& update, int version);
QJsonObject TagInsert(Section section, const tag::Row* tag);
QJsonObject TagDelete(Section section, CUuid& tag_id, int version);

QJsonObject WorkspaceMemberUpdate(CUuid& id, CJsonObject& update, int version);
QJsonObject WorkspaceMemberDelete(CUuid& id);
QJsonObject WorkspaceMemberAck(CUuid& widget_id, CString& workspace);
QJsonObject AuditLogAck(CUuid& widget_id, CString& workspace, const utils::DateTimeRange& range, Section section);

QJsonObject TreeAck(Section section, const utils::DateTimeRange& range);
QJsonObject TableAck(Section section, CUuid& node_id, CUuid& entry_id = {});
QJsonObject NodeAck(Section section, CUuid& node_id);
QJsonObject OrderHistory(Section section, CUuid& widget_id, CUuid& node_id, NodeUnit unit, const utils::DateTimeRange& range);
QJsonObject StatementPrimary(Section section, CUuid& widget_id, int unit, const utils::DateTimeRange& range);
QJsonObject StatementSecondary(Section section, CUuid& widget_id, CUuid& partner_id, int unit, const utils::DateTimeRange& range);
QJsonObject StatementTertiary(Section section, CUuid& widget_id, CUuid& partner_id, int unit, const utils::DateTimeRange& range);
QJsonObject SettlementPrimary(Section section, CUuid& widget_id, const utils::DateTimeRange& range);
QJsonObject SettlementSecondary(Section section, CUuid& widget_id, CUuid& partner_id, CUuid& settlement_id);
QJsonObject SettlementDelete(Section section, CUuid& settlement_id, int version);
QJsonObject SettlementView(Section section, CUuid& widget_id, const utils::DateTimeRange& range);

QJsonObject NodeDirectionRule(Section section, CUuid& node_id, bool direction_rule, int version);
QJsonObject NodeStatus(Section section, CUuid& node_id, int status, int version);
QJsonObject NodeName(Section section, CUuid& node_id, CString& name, int version);
QJsonObject NodeUpdate(Section section, CUuid& node_id, CJsonObject& update, int version);
QJsonObject DocumentDir(Section section, CString& document_dir);
QJsonObject DefaultUnit(Section section, int unit);

QJsonObject EntryDescriptionSearch(Section section, CString& keyword);
QJsonObject EntryTagSearch(Section section, const QSet<QString>& tags);
QJsonObject OrderNameSearch(Section section, CString& keyword);
QJsonObject OrderTagSearch(Section section, const QSet<QString>& tags);

QJsonObject OrderRecall(Section section, CUuid& node_id, CJsonObject& update, int version);
QJsonObject AccountName(CString& email, CString& name);
QJsonObject AccountUsername(CString& email, CString& username);

QJsonObject InventoryHeadAck(Section section, CUuid& widget_id, const utils::DateTimeRange& range, int moc, int mpc, int mam);
QJsonObject PartnerHeadAck(Section section, CUuid& widget_id, const utils::DateTimeRange& range, int moc, int mid, int mam);
QJsonObject BalanceSheetAck(CUuid& widget_id, CUuid& asset, CUuid& liability, CUuid& equity, const utils::DateTimeRange& range, int level);
QJsonObject IncomeStatementAck(CUuid& widget_id, CUuid& income, CUuid& expense, const utils::DateTimeRange& range, int level);
QJsonObject CashFlowStatementAck(CUuid& widget_id, const utils::DateTimeRange& range);

}
