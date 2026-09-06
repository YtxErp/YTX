#include "jsongen.h"

#include <QJsonArray>

#include "component/constant.h"
#include "component/constantstring.h"
#include "component/constantwebsocket.h"

namespace JsonGen {

QJsonObject NodeInsert(Section section, const Node* node, CUuid& parent_id)
{
    Q_ASSERT(node != nullptr);

    const QJsonObject node_json { node->WriteJson() };

    QJsonObject path_json {};
    path_json.insert(kAncestor, parent_id.toString(QUuid::WithoutBraces));
    path_json.insert(kDescendant, node->id.toString(QUuid::WithoutBraces));

    QJsonObject message {};
    message.insert(kSection, std::to_underlying(section));
    message.insert(kNode, node_json);
    message.insert(kPath, path_json);

    return message;
}

QJsonObject NodeDrag(Section section, CUuid& node_id, CUuid& parent_id)
{
    QJsonObject path_json {};
    path_json.insert(kAncestor, parent_id.toString(QUuid::WithoutBraces));
    path_json.insert(kDescendant, node_id.toString(QUuid::WithoutBraces));

    QJsonObject message {};
    message.insert(kSection, std::to_underlying(section));
    message.insert(kPath, path_json);
    return message;
}

QJsonObject LeafDelete(Section section, CUuid& node_id, int version)
{
    QJsonObject message {};
    message.insert(kSection, std::to_underlying(section));
    message.insert(kNodeId, node_id.toString(QUuid::WithoutBraces));
    message.insert(kVersion, version);
    return message;
}

QJsonObject BranchDelete(Section section, CUuid& node_id, CUuid& parent_id, int version)
{
    QJsonObject message {};
    message.insert(kSection, std::to_underlying(section));
    message.insert(kNodeId, node_id.toString(QUuid::WithoutBraces));
    message.insert(kParentId, parent_id.toString(QUuid::WithoutBraces));
    message.insert(kVersion, version);
    return message;
}

QJsonObject Login(CString& email, CString& password, CString& workspace, const utils::DateTimeRange& range)
{
    QJsonObject message {};
    message.insert(kEmail, email);
    message.insert(kPassword, password);
    message.insert(kWorkspace, workspace);
    message.insert(kStart, range.start.toString(Qt::ISODate));
    message.insert(kEnd, range.end.toString(Qt::ISODate));
    return message;
}

QJsonObject LeafDeleteCheck(Section section, CUuid& node_id, int version)
{
    QJsonObject message {};
    message.insert(kSection, std::to_underlying(section));
    message.insert(kNodeId, node_id.toString(QUuid::WithoutBraces));
    message.insert(kVersion, version);
    return message;
}

QJsonObject LeafReplace(Section section, CUuid& old_id, CUuid& new_id, int version)
{
    QJsonObject message {};
    message.insert(kSection, std::to_underlying(section));
    message.insert(kOldNodeId, old_id.toString(QUuid::WithoutBraces));
    message.insert(kNewNodeId, new_id.toString(QUuid::WithoutBraces));
    message.insert(kVersion, version);
    return message;
}

QJsonObject TableAck(Section section, CUuid& node_id, CUuid& entry_id)
{
    QJsonObject message {};
    message.insert(kSection, std::to_underlying(section));
    message.insert(kNodeId, node_id.toString(QUuid::WithoutBraces));
    message.insert(kEntryId, entry_id.toString(QUuid::WithoutBraces));
    return message;
}

QJsonObject MarkEntries(Section section, CUuid& node_id, int operation)
{
    QJsonObject message {};
    message.insert(kSection, std::to_underlying(section));
    message.insert(kNodeId, node_id.toString(QUuid::WithoutBraces));
    message.insert(WsField::kMarkOperation, operation);
    return message;
}

QJsonObject TreeAck(Section section, const utils::DateTimeRange& range)
{
    QJsonObject message {};
    message.insert(kSection, std::to_underlying(section));
    message.insert(kStart, range.start.toString(Qt::ISODate));
    message.insert(kEnd, range.end.toString(Qt::ISODate));
    return message;
}

QJsonObject InventoryHeadAck(Section section, CUuid& widget_id, const utils::DateTimeRange& range, int moc, int mpc, int mam)
{
    QJsonObject message {};
    message.insert(kSection, std::to_underlying(section));
    message.insert(kWidgetId, widget_id.toString(QUuid::WithoutBraces));
    message.insert(kStart, range.start.toString(Qt::ISODate));
    message.insert(kEnd, range.end.toString(Qt::ISODate));
    message.insert(section_heat::kMinOrderCount, moc);
    message.insert(section_heat::kMinPartnerCount, mpc);
    message.insert(section_heat::kMinActiveMonths, mam);
    return message;
}

QJsonObject DocumentDir(Section section, CString& document_dir)
{
    QJsonObject message {};
    message.insert(kSection, std::to_underlying(section));
    message.insert(WsField::kDocumentDir, document_dir);
    return message;
}

QJsonObject DefaultUnit(Section section, int unit)
{
    QJsonObject message {};
    message.insert(kSection, std::to_underlying(section));
    message.insert(WsField::kDefaultUnit, unit);
    return message;
}

QJsonObject Register(CString& email, CString& password)
{
    QJsonObject message {};
    message.insert(kEmail, email);
    message.insert(kPassword, password);
    return message;
}

QJsonObject EntryDescriptionSearch(Section section, CString& keyword)
{
    QJsonObject message {};
    message.insert(kSection, std::to_underlying(section));
    message.insert(kKeyword, keyword);
    return message;
}

QJsonObject EntryTagSearch(Section section, const QSet<QString>& tags)
{
    QJsonObject message {};
    message.insert(kSection, std::to_underlying(section));

    // Convert QSet<QString> to QJsonArray
    QJsonArray tag_array {};
    for (const QString& tag_id : tags) {
        tag_array.append(tag_id);
    }
    message.insert(kTagArray, tag_array);

    return message;
}

QJsonObject NodeAck(Section section, CUuid& node_id)
{
    QJsonObject message {};
    message.insert(kSection, std::to_underlying(section));
    message.insert(kNodeId, node_id.toString(QUuid::WithoutBraces));
    return message;
}

QJsonObject NodeDirectionRule(Section section, CUuid& node_id, bool direction_rule, int version)
{
    QJsonObject message {};
    message.insert(kSection, std::to_underlying(section));
    message.insert(kNodeId, node_id.toString(QUuid::WithoutBraces));
    message.insert(kDirectionRule, direction_rule);
    message.insert(kVersion, version);
    return message;
}

QJsonObject NodeStatus(Section section, CUuid& node_id, int status, int version)
{
    QJsonObject message {};
    message.insert(kSection, std::to_underlying(section));
    message.insert(kNodeId, node_id.toString(QUuid::WithoutBraces));
    message.insert(kStatus, status);
    message.insert(kVersion, version);
    return message;
}

QJsonObject EntryUpdate(Section section, CUuid& entry_id, CJsonObject& update, int version)
{
    QJsonObject message {};
    message.insert(kSection, std::to_underlying(section));
    message.insert(kEntryId, entry_id.toString(QUuid::WithoutBraces));
    message.insert(kUpdate, update);
    message.insert(kVersion, version);
    return message;
}

QJsonObject NodeUpdate(Section section, CUuid& node_id, CJsonObject& update, int version)
{
    QJsonObject message {};
    message.insert(kSection, std::to_underlying(section));
    message.insert(kNodeId, node_id.toString(QUuid::WithoutBraces));
    message.insert(kUpdate, update);
    message.insert(kVersion, version);
    return message;
}

QJsonObject NodeName(Section section, CUuid& node_id, CString& name, int version)
{
    QJsonObject message {};
    message.insert(kSection, std::to_underlying(section));
    message.insert(kNodeId, node_id.toString(QUuid::WithoutBraces));
    message.insert(kName, name);
    message.insert(kVersion, version);
    return message;
}

QJsonObject OrderNameSearch(Section section, CString& keyword)
{
    QJsonObject message {};
    message.insert(kSection, std::to_underlying(section));
    message.insert(kKeyword, keyword);
    return message;
}

QJsonObject OrderRecall(Section section, CUuid& node_id, CJsonObject& update, int version)
{
    QJsonObject message {};
    message.insert(kSection, std::to_underlying(section));
    message.insert(WsField::kNodeUpdate, update);
    message.insert(kNodeId, node_id.toString(QUuid::WithoutBraces));
    message.insert(kVersion, version);
    return message;
}

QJsonObject EntryValue(Section section, CUuid& entry_id, CJsonObject& update, InputSide side, int version)
{
    QJsonObject message {};

    message.insert(kSection, std::to_underlying(section));
    message.insert(kUpdate, update);
    message.insert(kInputSide, std::to_underlying(side));
    message.insert(kEntryId, entry_id.toString(QUuid::WithoutBraces));
    message.insert(kVersion, version);
    return message;
}

QJsonObject EntryMessage(Section section, CUuid& entry_id)
{
    QJsonObject message {};
    message.insert(kSection, std::to_underlying(section));
    message.insert(kEntryId, entry_id.toString(QUuid::WithoutBraces));
    return message;
}

QJsonObject OrderHistory(Section section, CUuid& widget_id, CUuid& node_id, NodeUnit unit, const utils::DateTimeRange& range)
{
    QJsonObject message {};

    message.insert(kSection, std::to_underlying(section));
    message.insert(kWidgetId, widget_id.toString(QUuid::WithoutBraces));
    message.insert(kNodeId, node_id.toString(QUuid::WithoutBraces));
    message.insert(kUnit, std::to_underlying(unit));
    message.insert(kStart, range.start.toString(Qt::ISODate));
    message.insert(kEnd, range.end.toString(Qt::ISODate));

    return message;
}

QJsonObject StatementPrimary(Section section, CUuid& widget_id, int unit, const utils::DateTimeRange& range)
{
    QJsonObject message {};

    message.insert(kSection, std::to_underlying(section));
    message.insert(kWidgetId, widget_id.toString(QUuid::WithoutBraces));
    message.insert(kUnit, unit);
    message.insert(kStart, range.start.toString(Qt::ISODate));
    message.insert(kEnd, range.end.toString(Qt::ISODate));

    return message;
}

QJsonObject StatementSecondary(Section section, CUuid& widget_id, CUuid& partner_id, int unit, const utils::DateTimeRange& range)
{
    QJsonObject message {};

    message.insert(kSection, std::to_underlying(section));
    message.insert(kWidgetId, widget_id.toString(QUuid::WithoutBraces));
    message.insert(kPartnerId, partner_id.toString(QUuid::WithoutBraces));
    message.insert(kUnit, unit);
    message.insert(kStart, range.start.toString(Qt::ISODate));
    message.insert(kEnd, range.end.toString(Qt::ISODate));

    return message;
}

QJsonObject StatementTertiary(Section section, CUuid& widget_id, CUuid& partner_id, int unit, const utils::DateTimeRange& range)
{
    QJsonObject message {};

    message.insert(kSection, std::to_underlying(section));
    message.insert(kWidgetId, widget_id.toString(QUuid::WithoutBraces));
    message.insert(kPartnerId, partner_id.toString(QUuid::WithoutBraces));
    message.insert(kUnit, unit);
    message.insert(kStart, range.start.toString(Qt::ISODate));
    message.insert(kEnd, range.end.toString(Qt::ISODate));

    return message;
}

QJsonObject SettlementPrimary(Section section, CUuid& widget_id, const utils::DateTimeRange& range)
{
    QJsonObject message {};

    message.insert(kSection, std::to_underlying(section));
    message.insert(kWidgetId, widget_id.toString(QUuid::WithoutBraces));
    message.insert(kStart, range.start.toString(Qt::ISODate));
    message.insert(kEnd, range.end.toString(Qt::ISODate));

    return message;
}

QJsonObject SettlementSecondary(Section section, CUuid& widget_id, CUuid& partner_id, CUuid& settlement_id)
{
    QJsonObject message {};

    message.insert(kSection, std::to_underlying(section));
    message.insert(kWidgetId, widget_id.toString(QUuid::WithoutBraces));
    message.insert(kPartnerId, partner_id.toString(QUuid::WithoutBraces));
    message.insert(kSettlementId, settlement_id.toString(QUuid::WithoutBraces));

    return message;
}

QJsonObject SettlementDelete(Section section, CUuid& settlement_id, int version)
{
    QJsonObject message {};

    message.insert(kSection, std::to_underlying(section));
    message.insert(kSettlementId, settlement_id.toString(QUuid::WithoutBraces));
    message.insert(kVersion, version);
    return message;
}

QJsonObject TagUpdate(Section section, CUuid& id, CJsonObject& update, int version)
{
    QJsonObject message {};
    message.insert(kSection, std::to_underlying(section));
    message.insert(kId, id.toString(QUuid::WithoutBraces));
    message.insert(kUpdate, update);
    message.insert(kVersion, version);
    return message;
}

QJsonObject WorkspaceMemberAck(CUuid& widget_id, CString& workspace)
{
    QJsonObject message {};
    message.insert(kWidgetId, widget_id.toString(QUuid::WithoutBraces));
    message.insert(kWorkspace, workspace);
    return message;
}

QJsonObject WorkspaceMemberUpdate(CUuid& id, CJsonObject& update, int version)
{
    QJsonObject message {};
    message.insert(kId, id.toString(QUuid::WithoutBraces));
    message.insert(kUpdate, update);
    message.insert(kVersion, version);
    return message;
}

QJsonObject WorkspaceMemberDelete(CUuid& id)
{
    QJsonObject message {};

    message.insert(kId, id.toString(QUuid::WithoutBraces));

    return message;
}

QJsonObject TagInsert(Section section, const tag::Row* tag)
{
    QJsonObject message {};

    message.insert(kSection, std::to_underlying(section));
    message.insert(kTag, tag->WriteJson());

    return message;
}

QJsonObject TagDelete(Section section, CUuid& tag_id, int version)
{
    QJsonObject message {};

    message.insert(kSection, std::to_underlying(section));
    message.insert(kId, tag_id.toString(QUuid::WithoutBraces));
    message.insert(kVersion, version);

    return message;
}

QJsonObject AccountName(CString& email, CString& name)
{
    QJsonObject message {};
    message.insert(kEmail, email);
    message.insert(kName, name);
    return message;
}

QJsonObject AccountUsername(CString& email, CString& username)
{
    QJsonObject message {};
    message.insert(kEmail, email);
    message.insert(kUsername, username);
    return message;
}

QJsonObject AuditLogAck(CUuid& widget_id, CString& workspace, const utils::DateTimeRange& range, Section section)
{
    QJsonObject message {};
    message.insert(kWidgetId, widget_id.toString(QUuid::WithoutBraces));
    message.insert(kWorkspace, workspace);
    message.insert(kStart, range.start.toString(Qt::ISODate));
    message.insert(kEnd, range.end.toString(Qt::ISODate));
    message.insert(kSection, std::to_underlying(section));
    return message;
}

QJsonObject OrderTagSearch(Section section, const QSet<QString>& tags)
{
    QJsonObject message {};
    message.insert(kSection, std::to_underlying(section));

    // Convert QSet<QString> to QJsonArray
    QJsonArray tag_array {};
    for (const QString& tag_id : tags) {
        tag_array.append(tag_id);
    }
    message.insert(kTagArray, tag_array);

    return message;
}

QJsonObject PartnerHeadAck(Section section, CUuid& widget_id, const utils::DateTimeRange& range, int moc, int mid, int mam)
{
    QJsonObject message {};
    message.insert(kSection, std::to_underlying(section));
    message.insert(kWidgetId, widget_id.toString(QUuid::WithoutBraces));
    message.insert(kStart, range.start.toString(Qt::ISODate));
    message.insert(kEnd, range.end.toString(Qt::ISODate));
    message.insert(section_heat::kMinOrderCount, moc);
    message.insert(section_heat::kMinInventoryDiversity, mid);
    message.insert(section_heat::kMinActiveMonths, mam);
    return message;
}

QJsonObject BalanceSheetAck(CUuid& widget_id, CUuid& asset, CUuid& liability, CUuid& equity, const utils::DateTimeRange& range, int level)
{
    QJsonObject message {};
    message.insert(kSection, std::to_underlying(Section::kFinance));
    message.insert(kWidgetId, widget_id.toString(QUuid::WithoutBraces));
    message.insert(kStart, range.start.toString(Qt::ISODate));
    message.insert(kEnd, range.end.toString(Qt::ISODate));
    message.insert(balance_sheet::kAssetId, asset.toString(QUuid::WithoutBraces));
    message.insert(balance_sheet::kLiabilityId, liability.toString(QUuid::WithoutBraces));
    message.insert(balance_sheet::kEquityId, equity.toString(QUuid::WithoutBraces));
    message.insert(kLevel, level);
    return message;
}

QJsonObject IncomeStatementAck(CUuid& widget_id, CUuid& income, CUuid& expense, const utils::DateTimeRange& range, int level)
{
    QJsonObject date_range {};
    date_range.insert(kStart, range.start.toUTC().toString(Qt::ISODate));
    date_range.insert(kEnd, range.end.toUTC().toString(Qt::ISODate));

    QJsonObject message {};
    message.insert(kSection, std::to_underlying(Section::kFinance));
    message.insert(kWidgetId, widget_id.toString(QUuid::WithoutBraces));
    message.insert(income_statement::kIncomeId, income.toString(QUuid::WithoutBraces));
    message.insert(income_statement::kExpenseId, expense.toString(QUuid::WithoutBraces));
    message.insert(income_statement::kDateRange, date_range);
    message.insert(kLevel, level);
    return message;
}

QJsonObject CashFlowStatementAck(CUuid& widget_id, const utils::DateTimeRange& range)
{
    QJsonObject message {};
    message.insert(kSection, std::to_underlying(Section::kFinance));
    message.insert(kWidgetId, widget_id.toString(QUuid::WithoutBraces));
    message.insert(kStart, range.start.toString(Qt::ISODate));
    message.insert(kEnd, range.end.toString(Qt::ISODate));

    return message;
}

QJsonObject SettlementView(Section section, CUuid& widget_id, const utils::DateTimeRange& range)
{
    QJsonObject message {};

    message.insert(kSection, std::to_underlying(section));
    message.insert(kWidgetId, widget_id.toString(QUuid::WithoutBraces));
    message.insert(kStart, range.start.toString(Qt::ISODate));
    message.insert(kEnd, range.end.toString(Qt::ISODate));

    return message;
}

}
