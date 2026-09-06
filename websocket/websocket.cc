#include "websocket.h"

#include <QCoreApplication>
#include <QJsonArray>

#include "component/constantstring.h"
#include "component/constantwebsocket.h"
#include "component/using.h"
#include "entryhub/entryhubi.h"
#include "entryhub/entryhubp.h"
#include "global/userprofile.h"
#include "tree/model/treemodelo.h"
#include "tree/model/treemodelp.h"
#include "utils/mainwindowutils.h"
#include "workspace/sectionpermissions.h"

WebSocket::WebSocket(QObject* parent)
    : QObject(parent)
{
    InitHandler();
    InitConnect();
}

WebSocket::~WebSocket()
{
    StopTimer();

    socket_.disconnect();
    if (ping_timer_)
        ping_timer_->disconnect();
    if (timeout_timer_)
        timeout_timer_->disconnect();

    if (socket_.state() != QAbstractSocket::UnconnectedState) {
        socket_.abort();
    }

    qDebug() << "WebSocket destructed";
}

WebSocket* WebSocket::Instance()
{
    static auto* instance = new WebSocket(qApp);
    return instance;
}

void WebSocket::ReadConfig(const QSharedPointer<QSettings>& local_settings)
{
    local_settings->beginGroup(kServer);
    const QString host { local_settings->value(kHost, "ytxerp.cc").toString() };
    const QString port { local_settings->value(kPort, "443").toString() };
    local_settings->endGroup();

    QString scheme {};

    if (host == "127.0.0.1" || host == "localhost") {
        scheme = "ws";
    } else {
        scheme = "wss";
    }

    const QString url_str { QString("%1://%2:%3").arg(scheme, host, port) };

    server_url_ = QUrl(url_str);
    if (!server_url_.isValid()) {
        qCritical() << "[WS]" << "Invalid URL" << url_str;
    }
}

void WebSocket::Connect()
{
    if (!server_url_.isValid()) {
        qCritical() << "[WS]" << "Invalid URL, stopping connection.";
        return;
    }

    if (socket_.state() == QAbstractSocket::ConnectedState || socket_.state() == QAbstractSocket::ConnectingState) {
        qInfo() << "[WS]" << "Already connected or in the process of connecting.";
        return;
    }

    qInfo() << "[WS]" << "Connecting to server...";
    socket_.open(server_url_);
}

void WebSocket::RConnected()
{
    emit SConnectionAllow();
    InitTimer();
}

void WebSocket::RDisconnected()
{
    StopTimer();

    if (session_id_.isNull()) {
        qInfo() << "[WS]" << "Session ID is empty — disconnected before login.";
        return;
    }

    session_id_ = QUuid();

    if (manual_disconnect_) {
        qInfo() << "[WS]" << "Auto-reconnecting to server after logout.";
        Connect();
    } else {
        emit SRemoteHostClosed();
    }

    manual_disconnect_ = false;
}

void WebSocket::Reset()
{
    if (socket_.state() == QAbstractSocket::ConnectedState || socket_.state() == QAbstractSocket::ConnectingState) {
        manual_disconnect_ = true;
        socket_.close();
    }
}

void WebSocket::RErrorOccurred(QAbstractSocket::SocketError error)
{
    switch (error) {
    case QAbstractSocket::ConnectionRefusedError:
        emit SConnectionDeny();
        qWarning() << "[WS]" << "Connection refused! (The peer refused or timed out)";
        break;
    case QAbstractSocket::RemoteHostClosedError:
        qWarning() << "[WS]" << "Remote host closed connection!";
        break;
    case QAbstractSocket::HostNotFoundError:
        qWarning() << "[WS]" << "Host not found! (Invalid address or DNS failure)";
        break;
    case QAbstractSocket::SocketAccessError:
        qWarning() << "[WS]" << "Socket access error! (Permission denied)";
        break;
    case QAbstractSocket::SocketResourceError:
        qWarning() << "[WS]" << "Socket resource error! (Too many open sockets)";
        break;
    case QAbstractSocket::SocketTimeoutError:
        qWarning() << "[WS]" << "Socket timeout error!";
        break;
    case QAbstractSocket::DatagramTooLargeError:
        qWarning() << "[WS]" << "Datagram too large! (Exceeded system limit)";
        break;
    case QAbstractSocket::NetworkError:
        qWarning() << "[WS]" << "Network error! (Cable unplugged or lost connection)";
        break;
    case QAbstractSocket::AddressInUseError:
        qWarning() << "[WS]" << "Address already in use! (Port conflict)";
        break;
    case QAbstractSocket::SocketAddressNotAvailableError:
        qWarning() << "[WS]" << "Socket address not available! (Address does not belong to host)";
        break;
    default:
        qWarning() << "[WS]" << "Unknown socket error:" << error;
        break;
    }
}

void WebSocket::InitHandler()
{
    handler_obj_[WsKey::kLoginOutcome] = [this](const QJsonObject& obj) { NotifyLoginOutcome(obj); };
    handler_obj_[WsKey::kRegisterOutcome] = [this](const QJsonObject& obj) { NotifyRegisterOutcome(obj); };
    handler_arr_[WsKey::kSharedConfigApply] = [this](const QJsonArray& arr) { ApplySharedConfig(arr); };
    handler_arr_[WsKey::kPartnerEntryApply] = [this](const QJsonArray& arr) { ApplyPartnerEntry(arr); };

    handler_obj_[WsKey::kDefaultUnitUpdate] = [this](const QJsonObject& obj) { UpdateDefaultUnit(obj); };
    handler_obj_[WsKey::kDefaultUnitDeny] = [this](const QJsonObject& obj) { DenyDefaultUnit(obj); };
    handler_obj_[WsKey::kDocumentDirUpdate] = [this](const QJsonObject& obj) { UpdateDocumentDir(obj); };

    handler_obj_[WsKey::kOrderTreeAck] = [this](const QJsonObject& obj) { AckOrderTree(obj); };
    handler_obj_[WsKey::kTableAck] = [this](const QJsonObject& obj) { AckTable(obj); };
    handler_obj_[WsKey::kOrderHistory] = [this](const QJsonObject& obj) { OnOrderHistory(obj); };
    handler_obj_[WsKey::kStatementPrimary] = [this](const QJsonObject& obj) { OnStatementPrimary(obj); };
    handler_obj_[WsKey::kStatementSecondary] = [this](const QJsonObject& obj) { OnStatementSecondary(obj); };
    handler_obj_[WsKey::kStatementTertiary] = [this](const QJsonObject& obj) { OnStatementTertiary(obj); };
    handler_obj_[WsKey::kSettlementPrimary] = [this](const QJsonObject& obj) { OnSettlementPrimary(obj); };
    handler_obj_[WsKey::kSettlementSecondary] = [this](const QJsonObject& obj) { OnSettlementSecondary(obj); };
    handler_obj_[WsKey::kSettlementInsert] = [this](const QJsonObject& obj) { InsertSettlement(obj); };
    handler_obj_[WsKey::kSettlementUpdate] = [this](const QJsonObject& obj) { UpdateSettlement(obj); };
    handler_obj_[WsKey::kSettlementRecall] = [this](const QJsonObject& obj) { RecallSettlement(obj); };
    handler_obj_[WsKey::kSettlementView] = [this](const QJsonObject& obj) { OnSettlementView(obj); };
    handler_obj_[WsKey::kOperationDeny] = [this](const QJsonObject& /*obj*/) { DenyOperation(); };
    handler_obj_[WsKey::kTreeApply] = [this](const QJsonObject& obj) { ApplyTree(obj); };

    handler_obj_[WsKey::kTagApply] = [this](const QJsonObject& obj) { ApplyTag(obj); };
    handler_obj_[WsKey::kTagInsert] = [this](const QJsonObject& obj) { InsertTag(obj); };
    handler_obj_[WsKey::kTagUpdate] = [this](const QJsonObject& obj) { UpdateTag(obj); };
    handler_obj_[WsKey::kTagDelete] = [this](const QJsonObject& obj) { DeleteTag(obj); };

    handler_obj_[WsKey::kTreeSyncFinish] = [this](const QJsonObject& /*obj*/) { FinishTreeSync(); };
    handler_obj_[WsKey::kNodeInsert] = [this](const QJsonObject& obj) { InsertNode(obj); };
    handler_obj_[WsKey::kOrderNodeAck] = [this](const QJsonObject& obj) { AckOrderNode(obj); };
    handler_obj_[WsKey::kLeafDelete] = [this](const QJsonObject& obj) { DeleteLeaf(obj); };
    handler_obj_[WsKey::kPartnerLeafDelete] = [this](const QJsonObject& obj) { DeletePartnerLeaf(obj); };
    handler_obj_[WsKey::kOrderLeafDelete] = [this](const QJsonObject& obj) { DeleteOrderLeaf(obj); };
    handler_obj_[WsKey::kBranchDelete] = [this](const QJsonObject& obj) { CommitDeleteNode(obj); };
    handler_obj_[WsKey::kLeafReplace] = [this](const QJsonObject& obj) { ReplaceLeaf(obj); };
    handler_obj_[WsKey::kNodeUpdate] = [this](const QJsonObject& obj) { UpdateNode(obj); };
    handler_obj_[WsKey::kEntryInsert] = [this](const QJsonObject& obj) { InsertEntry(obj); };
    handler_obj_[WsKey::kEntryDescriptionSearch] = [this](const QJsonObject& obj) { SearchEntry(obj); };
    handler_obj_[WsKey::kEntryTagSearch] = [this](const QJsonObject& obj) { SearchEntry(obj); };
    handler_obj_[WsKey::kOrderNameSearch] = [this](const QJsonObject& obj) { SearchOrder(obj); };
    handler_obj_[WsKey::kOrderTagSearch] = [this](const QJsonObject& obj) { SearchOrder(obj); };
    handler_obj_[WsKey::kEntryUpdate] = [this](const QJsonObject& obj) { UpdateEntry(obj); };
    handler_obj_[WsKey::kEntryDelete] = [this](const QJsonObject& obj) { DeleteEntry(obj); };
    handler_obj_[WsKey::kNodeDirectionRuleUpdate] = [this](const QJsonObject& obj) { UpdateNodeDirectionRule(obj); };
    handler_obj_[WsKey::kNodeStatus] = [this](const QJsonObject& obj) { UpdateNodeStatus(obj); };
    handler_obj_[WsKey::kLeafDeleteDeny] = [this](const QJsonObject& obj) { DenyLeafDelete(obj); };
    handler_obj_[WsKey::kNodeDrag] = [this](const QJsonObject& obj) { DragNode(obj); };
    handler_obj_[WsKey::kEntriesMark] = [this](const QJsonObject& obj) { MarkEntries(obj); };

    handler_obj_[WsKey::kNodeNameUpdate] = [this](const QJsonObject& obj) { UpdateNodeName(obj); };
    handler_obj_[WsKey::kEntryLinkedNodeUpdate] = [this](const QJsonObject& obj) { UpdateEntryLinkedNode(obj); };
    handler_obj_[WsKey::kEntryRateUpdate] = [this](const QJsonObject& obj) { UpdateEntryRate(obj); };
    handler_obj_[WsKey::kEntryNumericUpdate] = [this](const QJsonObject& obj) { UpdateEntryNumeric(obj); };
    handler_obj_[WsKey::kLeafDeleteAllow] = [this](const QJsonObject& obj) { CommitDeleteNode(obj); };
    handler_obj_[WsKey::kOrderInsertSave] = [this](const QJsonObject& obj) { InsertOrder(obj, false); };
    handler_obj_[WsKey::kUnreleasedOrderSave] = [this](const QJsonObject& obj) { SaveOrder(obj); };
    handler_obj_[WsKey::kOrderInsertRelease] = [this](const QJsonObject& obj) { InsertOrder(obj, true); };
    handler_obj_[WsKey::kUnreleasedOrderRelease] = [this](const QJsonObject& obj) { ReleaseOrder(obj); };
    handler_obj_[WsKey::kOrderRecall] = [this](const QJsonObject& obj) { RecallOrder(obj); };
    handler_obj_[WsKey::kRecalledOrderRelease] = [this](const QJsonObject& obj) { ReleaseOrder(obj); };
    handler_obj_[WsKey::kRecalledOrderSave] = [this](const QJsonObject& obj) { SaveOrder(obj); };

    handler_obj_[WsKey::kAccountNameUpdate] = [this](const QJsonObject& obj) { UpdateAccountName(obj); };
    handler_obj_[WsKey::kAccountUsernameUpdate] = [this](const QJsonObject& obj) { UpdateAccountUsername(obj); };
    handler_obj_[WsKey::kPeriodClose] = [this](const QJsonObject& obj) { PeriodClose(obj); };

    handler_obj_[WsKey::kWorkspaceMemberAck] = [this](const QJsonObject& obj) { AckWorkspaceMember(obj); };
    handler_obj_[WsKey::kAccountRoleUpdate] = [this](const QJsonObject& obj) { UpdateAccountRole(obj); };
    handler_obj_[WsKey::kAuditLogAck] = [this](const QJsonObject& obj) { AckAuditLog(obj); };
    handler_obj_[WsKey::kInventoryHeatAck] = [this](const QJsonObject& obj) { AckInventoryHeat(obj); };
    handler_obj_[WsKey::kPartnerHeatAck] = [this](const QJsonObject& obj) { AckPartnerHeat(obj); };
    handler_obj_[WsKey::kBalanceSheetAck] = [this](const QJsonObject& obj) { AckBalanceSheet(obj); };
    handler_obj_[WsKey::kIncomeStatementAck] = [this](const QJsonObject& obj) { AckIncomeStatement(obj); };
    handler_obj_[WsKey::kCashFlowStatementAck] = [this](const QJsonObject& obj) { AckCashFlowStatement(obj); };

    handler_obj_[WsKey::kPartnerEntryDelete] = [this](const QJsonObject& obj) { DeletePartnerEntry(obj); };
    handler_obj_[WsKey::kPartnerEntryInsert] = [this](const QJsonObject& obj) { InsertPartnerEntry(obj); };
    handler_obj_[WsKey::kPartnerNodeInsert] = [this](const QJsonObject& obj) { InsertNode(obj); };
}

void WebSocket::InitConnect()
{
    connect(&socket_, &QWebSocket::connected, this, &WebSocket::RConnected);
    connect(&socket_, &QWebSocket::disconnected, this, &WebSocket::RDisconnected);
    connect(&socket_, &QWebSocket::errorOccurred, this, &WebSocket::RErrorOccurred);
    connect(&socket_, &QWebSocket::binaryMessageReceived, this, &WebSocket::RBinaryMessageReceived);
    connect(&socket_, &QWebSocket::pong, this, &WebSocket::RPong);
}

void WebSocket::InitTimer()
{
    if (ping_timer_ && ping_timer_->isActive()) {
        return;
    }

    if (!ping_timer_) {
        ping_timer_ = new QTimer(this);
        ping_timer_->setInterval(time_const::kHeartbeatIntervalMs);
        connect(ping_timer_, &QTimer::timeout, this, &WebSocket::RSendPing);
    }

    if (!timeout_timer_) {
        timeout_timer_ = new QTimer(this);
        timeout_timer_->setSingleShot(true);
        connect(timeout_timer_, &QTimer::timeout, this, &WebSocket::RTimeout);
    }

    ping_timer_->start();
    timeout_timer_->start(time_const::kTimeoutThresholdMs);
}

void WebSocket::SendMessage(WsKey key, const QJsonObject& value)
{
    if (socket_.state() != QAbstractSocket::ConnectedState) {
        qWarning() << "[WS] Cannot send message: WebSocket is not connected.";
        return;
    }

    const QByteArray json { QJsonValue(value).toJson(QJsonValue::JsonFormat::Compact) };
    const QByteArray compressed { utils::ZstdCompress(json) };
    const uint16_t key_value { std::to_underlying(key) };

    QByteArray message {};
    message.reserve(2 + compressed.size());

    // same as Rust u16::to_be_bytes()
    message.append(static_cast<char>((key_value >> 8) & 0xff));
    message.append(static_cast<char>(key_value & 0xff));

    message.append(compressed);

    socket_.sendBinaryMessage(message);
}

void WebSocket::HandleMessage(WsKey key, const QByteArray& payload)
{
    QJsonParseError err {};
    const QJsonValue value { QJsonValue::fromJson(payload, &err) };
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "[WS] Invalid message received from server:" << WsMsgToString(key);
        return;
    }

    const bool is_object { value.isObject() };
    const bool is_array { value.isArray() };

    if (!is_object && !is_array) {
        qWarning() << "[WS] Unsupported value type for key:" << WsMsgToString(key);
        return;
    }

    if (is_object) {
        qDebug() << "[WS] Object key:" << WsMsgToString(key);
        if (const auto it { handler_obj_.constFind(key) }; it != handler_obj_.constEnd()) {
            it.value()(value.toObject());
            return;
        }
    }

    if (is_array) {
        qDebug() << "[WS] Array key:" << WsMsgToString(key);

        if (const auto it { handler_arr_.constFind(key) }; it != handler_arr_.constEnd()) {
            it.value()(value.toArray());
            return;
        }
    }

    qWarning() << "[WS] Unsupported server message:" << WsMsgToString(key);
}

void WebSocket::RBinaryMessageReceived(const QByteArray& data)
{
    if (data.size() < 2) {
        qWarning() << "[WS] Invalid binary message length";
        return;
    }

    timeout_timer_->start(time_const::kTimeoutThresholdMs);

    const uint16_t high { static_cast<uint8_t>(data.at(0)) };
    const uint16_t low { static_cast<uint8_t>(data.at(1)) };

    const auto key_value { static_cast<uint16_t>((high << 8) | low) };
    const auto key { static_cast<WsKey>(key_value) };

    const QByteArray payload { utils::ZstdDecompress(data.sliced(2)) };
    HandleMessage(key, payload);
}

void WebSocket::NotifyLoginOutcome(const QJsonObject& obj)
{
    Q_ASSERT(obj.contains(kResult));
    Q_ASSERT(obj.contains(kCode));

    const bool result { obj[kResult].toBool() };
    const int code { obj[kCode].toInt() };

    if (result) {
        session_id_ = QUuid(obj[kSessionId].toString());

        const auto expire_time { QDateTime::fromString(obj[kExpireTime].toString(), Qt::ISODate) };
        const auto expire_date { expire_time.date().toString(datetime_format::kDashedDate) };

        const auto username { obj[kUsername].toString() };
        const auto name { obj[kName].toString() };
        const workspace::Role role { obj[kWorkspaceRole].toInt() };
        const section::Permissions permissions { obj[kSectionPermissions].toInt() };

        UserProfile& profile { UserProfile::Instance() };
        profile.SetUsername(username);
        profile.SetName(name);
        profile.SetWorkspaceRole(role);
        profile.SetSectionPermissions(permissions);

        emit SLoginAllow(name, expire_date);
    } else {
        emit SLoginDeny(code);
    }
}

void WebSocket::DenyOperation() { emit SOperationDeny(); }

void WebSocket::FinishTreeSync() { emit STreeSyncFinish(); }

void WebSocket::NotifyRegisterOutcome(const QJsonObject& obj)
{
    Q_ASSERT(obj.contains(kResult));
    Q_ASSERT(obj.contains(kCode));

    const bool result { obj[kResult].toBool() };
    const int code { obj[kCode].toInt() };
    emit SRegisterResult(result, code);
}

void WebSocket::InsertNode(const QJsonObject& obj)
{
    Q_ASSERT(obj.contains(kSection));
    Q_ASSERT(obj.contains(kSessionId));
    Q_ASSERT(obj.contains(kNode));
    Q_ASSERT(obj.contains(kPath));

    const Section section { obj.value(kSection).toInt() };
    const auto session_id { QUuid(obj[kSessionId].toString()) };
    const QJsonObject node_obj { obj.value(kNode).toObject() };
    const QJsonObject path_obj { obj.value(kPath).toObject() };

    Q_ASSERT(path_obj.contains(kDescendant));
    Q_ASSERT(path_obj.contains(kAncestor));

    const auto descendant { QUuid(path_obj.value(kDescendant).toString()) };
    const auto ancestor { QUuid(path_obj.value(kAncestor).toString()) };

    auto tree_model { tree_model_hash_.value(section) };

    tree_model->ApplyNode(ancestor, node_obj);

    if (session_id_ == session_id)
        emit SNodeSelect(section, descendant);
}

void WebSocket::UpdateNode(const QJsonObject& obj)
{
    Q_ASSERT(obj.contains(kSection));
    Q_ASSERT(obj.contains(kSessionId));
    Q_ASSERT(obj.contains(kNodeId));
    Q_ASSERT(obj.contains(kUpdate));

    const Section section { obj.value(kSection).toInt() };
    const auto session_id { QUuid(obj[kSessionId].toString()) };
    const auto node_id { QUuid(obj.value(kNodeId).toString()) };
    const QJsonObject update { obj.value(kUpdate).toObject() };
    const int version { obj.value(kVersion).toInt() };

    auto tree_model { tree_model_hash_.value(section) };
    Q_ASSERT(tree_model != nullptr);

    if (session_id == session_id_) {
        tree_model->SyncVersion(node_id, version);
        return;
    }

    tree_model->ApplyUpdate(node_id, update, version);
}

void WebSocket::DragNode(const QJsonObject& obj)
{
    Q_ASSERT(obj.contains(kSection));
    Q_ASSERT(obj.contains(kSessionId));
    Q_ASSERT(obj.contains(kPath));

    const Section section { obj.value(kSection).toInt() };
    const auto session_id { QUuid(obj[kSessionId].toString()) };
    const QJsonObject path { obj.value(kPath).toObject() };

    const QUuid ancestor { path.value(kAncestor).toString() };
    const QUuid descendant { path.value(kDescendant).toString() };

    if (ancestor.isNull() || descendant.isNull()) {
        return;
    }

    auto tree_model { tree_model_hash_.value(section) };

    if (session_id != session_id_)
        tree_model->ApplyDrag(ancestor, descendant);
}

void WebSocket::ApplyTree(const QJsonObject& obj)
{
    Q_ASSERT(obj.contains(kSection));

    const Section section { obj.value(kSection).toInt() };
    auto tree_model { tree_model_hash_.value(section) };
    tree_model->ApplyTree(obj);
}

void WebSocket::ApplyTag(const QJsonObject& obj)
{
    Q_ASSERT(obj.contains(kSection));

    emit STagApply(obj);
}

void WebSocket::InsertTag(const QJsonObject& obj)
{
    Q_ASSERT(obj.contains(kSection));
    const auto session_id { QUuid(obj[kSessionId].toString()) };

    if (session_id != session_id_)
        emit STagInsert(obj);
}

void WebSocket::UpdateTag(const QJsonObject& obj)
{
    Q_ASSERT(obj.contains(kSection));
    emit STagUpdate(obj);
}

void WebSocket::DeleteTag(const QJsonObject& obj)
{
    Q_ASSERT(obj.contains(kSection));
    emit STagDelete(obj);
}

void WebSocket::PeriodClose(const QJsonObject& obj)
{
    const Section section { obj.value(kSection).toInt() };
    const QJsonArray entry_array { obj.value(kEntryArray).toArray() };
    const QJsonArray total_array { obj.value(kTotalArray).toArray() };
    const QUuid summary_node_id { obj.value(kSummaryNodeId).toString() };

    {
        auto entry_hub { entry_hub_hash_.value(section) };
        entry_hub->AckTable(summary_node_id, entry_array);
        entry_hub->PeriodClose(entry_array);
    }

    {
        auto tree_model { tree_model_hash_.value(section) };
        tree_model->SyncTotalArray(total_array);
    }
}

void WebSocket::ApplyPartnerEntry(const QJsonArray& arr)
{
    auto* entry_hub { static_cast<EntryHubP*>(entry_hub_hash_.value(Section::kPartner).data()) };
    entry_hub->ApplyPartnerEntry(arr);
}

void WebSocket::AckOrderTree(const QJsonObject& obj)
{
    Q_ASSERT(obj.contains(kSection));

    const Section section { obj.value(kSection).toInt() };
    auto tree_model { tree_model_hash_.value(section) };
    tree_model->ApplyTree(obj);
}

void WebSocket::AckTable(const QJsonObject& obj)
{
    Q_ASSERT(obj.contains(kSection));
    Q_ASSERT(obj.contains(kNodeId));
    Q_ASSERT(obj.contains(kEntryArray));
    Q_ASSERT(obj.contains(kEntryId));

    const Section section { obj.value(kSection).toInt() };
    const auto node_id { QUuid(obj.value(kNodeId).toString()) };
    const auto entry_id { QUuid(obj.value(kEntryId).toString()) };
    const QJsonArray array { obj.value(kEntryArray).toArray() };

    auto entry_hub { entry_hub_hash_.value(section) };
    entry_hub->AckTable(node_id, array);

    if (!entry_id.isNull())
        emit SEntrySelect(node_id, entry_id);
}

void WebSocket::AckOrderNode(const QJsonObject& obj)
{
    const Section section { obj.value(kSection).toInt() };
    const QUuid node_id(obj.value(kNodeId).toString());

    if (!obj.contains(kNode)) {
        qInfo() << "Order node no longer exists";
        return;
    }

    const QJsonObject leaf_obj { obj.value(kNode).toObject() };
    const QUuid ancestor { QUuid(obj.value(kAncestor).toString()) };

    auto tree_model { tree_model_hash_.value(section) };
    tree_model->ApplyNode(ancestor, leaf_obj);

    emit SNodeLocate(section, node_id);
}

void WebSocket::OnOrderHistory(const QJsonObject& obj)
{
    const Section section { obj.value(kSection).toInt() };
    const QUuid widget_id { QUuid(obj.value(kWidgetId).toString()) };
    const QJsonArray array { obj.value(kArray).toArray() };

    emit SOrderHistory(section, widget_id, array);
}

void WebSocket::OnStatementPrimary(const QJsonObject& obj)
{
    const Section section { obj.value(kSection).toInt() };
    const QUuid widget_id { QUuid(obj.value(kWidgetId).toString()) };
    const QJsonArray array { obj.value(kArray).toArray() };

    emit SStatementPrimary(section, widget_id, array);
}

void WebSocket::OnStatementSecondary(const QJsonObject& obj)
{
    const Section section { obj.value(kSection).toInt() };
    const QUuid widget_id { QUuid(obj.value(kWidgetId).toString()) };
    const QJsonArray array { obj.value(kArray).toArray() };

    emit SStatementSecondary(section, widget_id, array);
}

void WebSocket::OnStatementTertiary(const QJsonObject& obj)
{
    const Section section { obj.value(kSection).toInt() };
    const QUuid widget_id { QUuid(obj.value(kWidgetId).toString()) };
    const QJsonArray array { obj.value(kArray).toArray() };

    emit SStatementTertiary(section, widget_id, array);
}

void WebSocket::OnSettlementPrimary(const QJsonObject& obj)
{
    const Section section { obj.value(kSection).toInt() };
    const QUuid widget_id { QUuid(obj.value(kWidgetId).toString()) };
    const QJsonArray array { obj.value(kArray).toArray() };

    emit SSettlementPrimary(section, widget_id, array);
}

void WebSocket::OnSettlementSecondary(const QJsonObject& obj)
{
    const Section section { obj.value(kSection).toInt() };
    const QUuid widget_id { QUuid(obj.value(kWidgetId).toString()) };
    const QJsonArray array { obj.value(kArray).toArray() };

    emit SSettlementSecondary(section, widget_id, array);
}

void WebSocket::AckWorkspaceMember(const QJsonObject& obj)
{
    const QUuid widget_id { QUuid(obj.value(kWidgetId).toString()) };
    const QJsonArray array { obj.value(kArray).toArray() };

    emit SWorkspaceMemberAck(widget_id, array);
}

void WebSocket::AckAuditLog(const QJsonObject& obj)
{
    const QUuid widget_id { QUuid(obj.value(kWidgetId).toString()) };
    const QJsonArray array { obj.value(kArray).toArray() };

    emit SAuditLogAck(widget_id, array);
}

void WebSocket::AckInventoryHeat(const QJsonObject& obj)
{
    const auto session_id { QUuid(obj[kSessionId].toString()) };
    if (session_id != session_id_)
        return;

    const QUuid widget_id { QUuid(obj.value(kWidgetId).toString()) };
    if (widget_id.isNull()) {
        qWarning() << Q_FUNC_INFO << "widget_id is null";
        return;
    }

    const QJsonArray array { obj.value(kArray).toArray() };

    emit SInventoryHeatAck(widget_id, array);
}

void WebSocket::AckPartnerHeat(const QJsonObject& obj)
{
    const auto session_id { QUuid(obj[kSessionId].toString()) };
    if (session_id != session_id_)
        return;

    const QUuid widget_id { QUuid(obj.value(kWidgetId).toString()) };
    if (widget_id.isNull()) {
        qWarning() << Q_FUNC_INFO << "widget_id is null";
        return;
    }

    const QJsonArray array { obj.value(kArray).toArray() };

    emit SPartnerHeatAck(widget_id, array);
}

void WebSocket::AckBalanceSheet(const QJsonObject& obj)
{
    const auto session_id { QUuid(obj[kSessionId].toString()) };
    if (session_id != session_id_)
        return;

    const QUuid widget_id { QUuid(obj.value(kWidgetId).toString()) };
    if (widget_id.isNull()) {
        qWarning() << Q_FUNC_INFO << "widget_id is null";
        return;
    }

    const QJsonArray node_array { obj.value(kNodeArray).toArray() };
    const QJsonArray path_array { obj.value(kPathArray).toArray() };

    emit SBalanceSheetAck(widget_id, node_array, path_array);
}

void WebSocket::AckIncomeStatement(const QJsonObject& obj)
{
    const auto session_id { QUuid(obj[kSessionId].toString()) };
    if (session_id != session_id_)
        return;

    const QUuid widget_id { QUuid(obj.value(kWidgetId).toString()) };
    if (widget_id.isNull()) {
        qWarning() << Q_FUNC_INFO << "widget_id is null";
        return;
    }

    emit SIncomeStatementAck(widget_id, obj);
}

void WebSocket::AckCashFlowStatement(const QJsonObject& obj)
{
    const auto session_id { QUuid(obj[kSessionId].toString()) };
    if (session_id != session_id_)
        return;

    const QUuid widget_id { QUuid(obj.value(kWidgetId).toString()) };
    if (widget_id.isNull()) {
        qWarning() << Q_FUNC_INFO << "widget_id is null";
        return;
    }

    emit SCashFlowStatementAck(widget_id, obj);
}

void WebSocket::DeleteLeaf(const QJsonObject& obj)
{
    const Section section { obj.value(kSection).toInt() };
    const auto node_id { QUuid(obj.value(kNodeId).toString()) };

    const QJsonObject linked_entry_obj { obj.value(kLinkedEntry).toObject() };
    const QJsonArray total_array { obj.value(kTotalArray).toArray() };

    const auto leaf_entry { ParseLinkedEntry(linked_entry_obj) };

    auto entry_hub { entry_hub_hash_.value(section) };
    auto tree_model { tree_model_hash_.value(section) };

    entry_hub->DeleteDoubleLeaf(leaf_entry);
    tree_model->SyncTotalArray(total_array);
    tree_model->ApplyDelete(node_id);
}

void WebSocket::DeleteOrderLeaf(const QJsonObject& obj)
{
    const Section section { obj.value(kSection).toInt() };
    const auto node_id { QUuid(obj.value(kNodeId).toString()) };

    auto entry_hub { entry_hub_hash_.value(section) };
    auto tree_model { tree_model_hash_.value(section) };

    tree_model->ApplyDelete(node_id);
}

void WebSocket::DeletePartnerLeaf(const QJsonObject& obj)
{
    const Section section { obj.value(kSection).toInt() };
    const auto node_id { QUuid(obj.value(kNodeId).toString()) };

    const auto linked_entry_array { obj.value(kLinkedEntry).toArray() };
    const auto leaf_entry { ParseLinkedEntryP(linked_entry_array) };

    auto* entry_hub { static_cast<EntryHubP*>(entry_hub_hash_.value(section).data()) };
    auto tree_model { tree_model_hash_.value(section) };

    entry_hub->DeleteSingleLeaf(leaf_entry);
    tree_model->ApplyDelete(node_id);
}

void WebSocket::CommitDeleteNode(const QJsonObject& obj)
{
    const Section section { obj.value(kSection).toInt() };
    const auto node_id { QUuid(obj.value(kNodeId).toString()) };

    auto tree_model { tree_model_hash_.value(section) };

    tree_model->ApplyDelete(node_id);
}

void WebSocket::DenyLeafDelete(const QJsonObject& obj) { emit SLeafDeleteDeny(obj); }

void WebSocket::ReplaceLeaf(const QJsonObject& obj)
{
    Q_ASSERT(obj.contains(kSection));
    Q_ASSERT(obj.contains(kSessionId));
    Q_ASSERT(obj.contains(kResult));
    Q_ASSERT(obj.contains(node_ref::kPartner));
    Q_ASSERT(obj.contains(kOldNodeId));
    Q_ASSERT(obj.contains(kNewNodeId));

    const Section section { obj.value(kSection).toInt() };
    const auto session_id { QUuid(obj[kSessionId].toString()) };
    const bool result { obj.value(kResult).toBool() };
    const bool partner { obj.value(node_ref::kPartner).toBool() };
    const QUuid old_node_id(obj.value(kOldNodeId).toString());
    const QUuid new_node_id(obj.value(kNewNodeId).toString());

    Q_ASSERT(section == Section::kInventory);

    if (session_id == session_id_) {
        emit SReplaceResult(result);
    }

    if (!result)
        return;

    {
        auto* entry_hub { static_cast<EntryHubI*>(entry_hub_hash_.value(section).data()) };
        entry_hub->ReplaceLeaf(old_node_id, new_node_id);
    }

    {
        auto entry_hub_p { static_cast<EntryHubP*>(entry_hub_hash_.value(Section::kPartner).data()) };

        if (partner)
            entry_hub_p->ReplaceInternalInventoryRef(old_node_id, new_node_id);
    }

    auto tree_model { tree_model_hash_.value(section) };
    tree_model->ApplyReplace(old_node_id, new_node_id);
}

void WebSocket::UpdateEntry(const QJsonObject& obj)
{
    const Section section { obj.value(kSection).toInt() };
    const auto session_id { QUuid(obj[kSessionId].toString()) };
    const auto entry_id { QUuid(obj.value(kEntryId).toString()) };
    const QJsonObject update { obj.value(kUpdate).toObject() };
    const int version { obj.value(kVersion).toInt() };

    auto entry_hub { entry_hub_hash_.value(section) };

    Q_ASSERT(entry_hub);
    Q_ASSERT(!entry_id.isNull());

    if (session_id == session_id_) {
        entry_hub->UpdateVersion(entry_id, version);
        return;
    }

    entry_hub->UpdateEntry(entry_id, update, version);
}

void WebSocket::UpdateEntryLinkedNode(const QJsonObject& obj)
{
    Q_ASSERT(obj.contains(kLhsTotal));
    Q_ASSERT(obj.contains(kRhsTotal));

    const Section section { obj.value(kSection).toInt() };
    const auto session_id { QUuid(obj[kSessionId].toString()) };
    const QUuid entry_id { obj.value(kEntryId).toString() };
    const InputSide input_side { obj.value(kInputSide).toInt() };
    const QJsonObject update { obj.value(kUpdate).toObject() };

    auto entry_hub { entry_hub_hash_.value(section) };
    auto tree_model { tree_model_hash_.value(section) };

    const QJsonObject lhs_total { obj.value(kLhsTotal).toObject() };
    const QJsonObject rhs_total { obj.value(kRhsTotal).toObject() };

    const QJsonArray total_array { lhs_total, rhs_total };
    tree_model->SyncTotalArray(total_array);

    if (session_id == session_id_) {
        const int version { update.value(kVersion).toInt() };
        entry_hub->UpdateVersion(entry_id, version);
        return;
    }

    entry_hub->UpdateEntryLinkedNode(entry_id, update, input_side);
}

void WebSocket::UpdateEntryRate(const QJsonObject& obj)
{
    Q_ASSERT(obj.contains(kLhsTotal));
    Q_ASSERT(obj.contains(kRhsTotal));

    const Section section { obj.value(kSection).toInt() };
    const auto session_id { QUuid(obj[kSessionId].toString()) };

    const auto entry_id { QUuid(obj.value(kEntryId).toString()) };
    const QJsonObject update { obj.value(kUpdate).toObject() };
    const InputSide input_side { obj.value(kInputSide).toInt() };
    const int version { obj.value(kVersion).toInt() };

    auto entry_hub { entry_hub_hash_.value(section) };
    auto tree_model { tree_model_hash_.value(section) };

    Q_ASSERT(entry_hub);
    Q_ASSERT(tree_model);

    const QJsonObject lhs_total { obj.value(kLhsTotal).toObject() };
    const QJsonObject rhs_total { obj.value(kRhsTotal).toObject() };

    const QJsonArray total_array { lhs_total, rhs_total };
    tree_model->SyncTotalArray(total_array);

    if (session_id == session_id_) {
        entry_hub->UpdateVersion(entry_id, version);
        return;
    }

    entry_hub->UpdateEntryRate(entry_id, update, input_side, version);
}

void WebSocket::UpdateEntryNumeric(const QJsonObject& obj)
{
    Q_ASSERT(obj.contains(kLhsTotal));
    Q_ASSERT(obj.contains(kRhsTotal));

    const Section section { obj.value(kSection).toInt() };
    const auto session_id { QUuid(obj[kSessionId].toString()) };

    const auto entry_id { QUuid(obj.value(kEntryId).toString()) };
    const QJsonObject update { obj.value(kUpdate).toObject() };
    const int version { obj.value(kVersion).toInt() };

    auto entry_hub { entry_hub_hash_.value(section) };
    auto tree_model { tree_model_hash_.value(section) };

    Q_ASSERT(entry_hub);
    Q_ASSERT(tree_model);

    const QJsonObject lhs_total { obj.value(kLhsTotal).toObject() };
    const QJsonObject rhs_total { obj.value(kRhsTotal).toObject() };

    const QJsonArray total_array { lhs_total, rhs_total };
    tree_model->SyncTotalArray(total_array);

    if (session_id == session_id_) {
        entry_hub->UpdateVersion(entry_id, version);
        return;
    }

    entry_hub->UpdateEntryNumeric(entry_id, update, version);
}

void WebSocket::SearchEntry(const QJsonObject& obj)
{
    const Section section { obj.value(kSection).toInt() };
    const QJsonArray array { obj.value(kEntryArray).toArray() };

    auto entry_hub { entry_hub_hash_.value(section) };
    entry_hub->SearchEntry(array);
}

void WebSocket::SearchOrder(const QJsonObject& obj) { emit SOrderSearch(obj); }

void WebSocket::InsertEntry(const QJsonObject& obj)
{
    Q_ASSERT(obj.contains(kLhsTotal));
    Q_ASSERT(obj.contains(kRhsTotal));

    const Section section { obj.value(kSection).toInt() };
    const auto session_id { QUuid(obj[kSessionId].toString()) };

    const QJsonObject entry { obj.value(kEntry).toObject() };

    auto entry_hub { entry_hub_hash_.value(section) };
    auto tree_model { tree_model_hash_.value(section) };

    Q_ASSERT(entry_hub);
    Q_ASSERT(tree_model);

    const QJsonObject lhs_total { obj.value(kLhsTotal).toObject() };
    const QJsonObject rhs_total { obj.value(kRhsTotal).toObject() };

    const QJsonArray total_array { lhs_total, rhs_total };
    tree_model->SyncTotalArray(total_array);

    if (session_id == session_id_) {
        const QUuid id { entry.value(kId).toString() };
        const int version { entry.value(kVersion).toInt() };
        entry_hub->UpdateVersion(id, version);
        return;
    }

    entry_hub->InsertEntry(entry);
}

void WebSocket::InsertPartnerEntry(const QJsonObject& obj)
{
    const Section section { obj.value(kSection).toInt() };
    const auto session_id { QUuid(obj[kSessionId].toString()) };

    const QJsonObject entry { obj.value(kEntry).toObject() };

    auto entry_hub { entry_hub_hash_.value(section) };
    auto tree_model { tree_model_hash_.value(section) };

    Q_ASSERT(entry_hub);
    Q_ASSERT(tree_model);

    if (session_id == session_id_) {
        const QUuid id { entry.value(kId).toString() };
        const int version { entry.value(kVersion).toInt() };
        entry_hub->UpdateVersion(id, version);
        return;
    }

    entry_hub->InsertEntry(entry);
}

void WebSocket::DeleteEntry(const QJsonObject& obj)
{
    Q_ASSERT(obj.contains(kLhsTotal));
    Q_ASSERT(obj.contains(kRhsTotal));

    const Section section { obj.value(kSection).toInt() };
    const auto entry_id { QUuid(obj.value(kEntryId).toString()) };

    auto entry_hub { entry_hub_hash_.value(section) };
    auto tree_model { tree_model_hash_.value(section) };

    Q_ASSERT(entry_hub);
    Q_ASSERT(tree_model);

    const QJsonObject lhs_total { obj.value(kLhsTotal).toObject() };
    const QJsonObject rhs_total { obj.value(kRhsTotal).toObject() };

    const QJsonArray total_array { lhs_total, rhs_total };
    tree_model->SyncTotalArray(total_array);

    entry_hub->DeleteEntry(entry_id);
}

void WebSocket::DeletePartnerEntry(const QJsonObject& obj)
{
    const Section section { obj.value(kSection).toInt() };
    const auto entry_id { QUuid(obj.value(kEntryId).toString()) };

    auto entry_hub { entry_hub_hash_.value(section) };

    Q_ASSERT(entry_hub);

    entry_hub->DeleteEntry(entry_id);
}

void WebSocket::UpdateNodeDirectionRule(const QJsonObject& obj)
{
    const Section section { obj.value(kSection).toInt() };
    const auto node_id { QUuid(obj.value(kNodeId).toString()) };
    const bool direction_rule { obj.value(kDirectionRule).toBool() };
    const int version { obj.value(kVersion).toInt() };

    auto tree_model { tree_model_hash_.value(section) };
    Q_ASSERT(tree_model);

    tree_model->ApplyDirectionRule(node_id, direction_rule, version);
}

void WebSocket::UpdateNodeStatus(const QJsonObject& obj)
{
    const Section section { obj.value(kSection).toInt() };
    const auto node_id { QUuid(obj.value(kNodeId).toString()) };
    const int status { obj.value(kStatus).toInt() };
    const int version { obj.value(kVersion).toInt() };

    auto tree_model { tree_model_hash_.value(section) };
    Q_ASSERT(tree_model);

    tree_model->ApplyStatus(node_id, status, version);
}

void WebSocket::UpdateNodeName(const QJsonObject& obj)
{
    const Section section { obj.value(kSection).toInt() };
    const auto node_id { QUuid(obj.value(kNodeId).toString()) };
    const auto name { obj.value(kName).toString() };
    const int version { obj.value(kVersion).toInt() };

    auto tree_model { tree_model_hash_.value(section) };
    Q_ASSERT(tree_model);

    tree_model->ApplyName(node_id, name, version);
}

void WebSocket::UpdateAccountName(const QJsonObject& obj)
{
    const auto name { obj.value(kName).toString() };
    emit SAccountName(name);
}

void WebSocket::UpdateAccountUsername(const QJsonObject& obj) { emit SAccountUsername(obj); }

void WebSocket::UpdateAccountRole(const QJsonObject& /*obj*/) { emit SAccountRoleUpdate(); }

void WebSocket::ReleaseOrder(const QJsonObject& obj)
{
    const Section section { obj.value(kSection).toInt() };
    const auto session_id { QUuid(obj[kSessionId].toString()) };
    const auto node_id { QUuid(obj.value(kNodeId).toString()) };
    const auto node_update { obj.value(WsField::kNodeUpdate).toObject() };
    const int version { obj.value(kVersion).toInt() };

    auto* order_model { static_cast<TreeModelO*>(tree_model_hash_.value(section).data()) };
    Q_ASSERT(order_model != nullptr);

    {
        const NodeUnit unit { node_update.value(kUnit).toInt() };
        if (unit == NodeUnit::OMonthly) {
            auto* partner_model { static_cast<TreeModelP*>(tree_model_hash_.value(Section::kPartner).data()) };
            Q_ASSERT(partner_model != nullptr);

            const auto partner_id { QUuid(obj.value(kPartnerId).toString()) };
            const double initial_total { node_update.value(kInitialTotal).toString().toDouble() };

            partner_model->UpdateAmount(partner_id, initial_total);
        }
    }

    if (!order_model->Contains(node_id))
        return;

    if (session_id == session_id_) {
        emit SOrderRelease(section, node_id, version);
    }

    order_model->ApplyUpdate(node_id, node_update, version);
    order_model->HandleStatusChanged(node_id, OrderStatus::kReleased);
}

void WebSocket::SaveOrder(const QJsonObject& obj)
{
    const Section section { obj.value(kSection).toInt() };
    const auto session_id { QUuid(obj[kSessionId].toString()) };
    const auto node_id { QUuid(obj.value(kNodeId).toString()) };
    const auto node_update { obj.value(WsField::kNodeUpdate).toObject() };
    const int version { obj.value(kVersion).toInt() };

    auto* order_model { static_cast<TreeModelO*>(tree_model_hash_.value(section).data()) };
    Q_ASSERT(order_model != nullptr);

    if (!order_model->Contains(node_id))
        return;

    if (session_id == session_id_) {
        emit SOrderSave(section, node_id, version);
    }

    order_model->ApplyUpdate(node_id, node_update, version);
}

void WebSocket::InsertOrder(const QJsonObject& obj, bool is_released)
{
    const Section section { obj.value(kSection).toInt() };
    const auto session_id { QUuid(obj[kSessionId].toString()) };
    const QJsonObject node_obj { obj.value(kNode).toObject() };
    const QJsonObject path_obj { obj.value(kPath).toObject() };

    const auto descendant { QUuid(path_obj.value(kDescendant).toString()) };
    const auto ancestor { QUuid(path_obj.value(kAncestor).toString()) };
    const auto node_id { QUuid(node_obj.value(kId).toString()) };

    auto* order_model { static_cast<TreeModelO*>(tree_model_hash_.value(section).data()) };
    Q_ASSERT(order_model != nullptr);

    order_model->ApplyNode(ancestor, node_obj);
    const int version { node_obj.value(kVersion).toInt() };

    if (session_id == session_id_) {
        emit SNodeSelect(section, descendant);

        if (is_released)
            emit SOrderRelease(section, node_id, version);
        else
            emit SOrderSave(section, node_id, version);
    }

    if (is_released) {
        const NodeUnit unit { node_obj.value(kUnit).toInt() };

        if (unit == NodeUnit::OMonthly) {
            auto* partner_model { static_cast<TreeModelP*>(tree_model_hash_.value(Section::kPartner).data()) };
            Q_ASSERT(partner_model != nullptr);

            const auto partner_id { QUuid(node_obj.value(kPartnerId).toString()) };
            const double initial_total { node_obj.value(kInitialTotal).toString().toDouble() };

            partner_model->UpdateAmount(partner_id, initial_total);
        }

        order_model->HandleStatusChanged(node_id, OrderStatus::kReleased);
    }
}

void WebSocket::RecallOrder(const QJsonObject& obj)
{
    const Section section { obj.value(kSection).toInt() };
    const auto session_id { QUuid(obj[kSessionId].toString()) };
    const auto node_update { obj.value(WsField::kNodeUpdate).toObject() };
    const auto node_id { QUuid(obj.value(kNodeId).toString()) };
    const int version { obj.value(kVersion).toInt() };
    const NodeUnit unit { node_update.value(kUnit).toInt() };

    auto* order_model = static_cast<TreeModelO*>(tree_model_hash_.value(section).data());
    Q_ASSERT(order_model != nullptr);

    {
        if (unit == NodeUnit::OMonthly) {
            auto* partner_model { static_cast<TreeModelP*>(tree_model_hash_.value(Section::kPartner).data()) };
            Q_ASSERT(partner_model != nullptr);

            const auto partner_id { QUuid(node_update.value(kPartnerId).toString()) };
            const double initial_total { node_update.value(kInitialTotal).toString().toDouble() };

            partner_model->UpdateAmount(partner_id, -initial_total);
        }
    }

    if (!order_model->Contains(node_id))
        return;

    if (session_id == session_id_) {
        emit SOrderRecall(section, node_id, version);
    }

    order_model->ApplyUpdate(node_id, node_update, version);
    order_model->HandleStatusChanged(node_id, OrderStatus::kRecalled);
}

void WebSocket::InsertSettlement(const QJsonObject& obj)
{
    const Section section { obj.value(kSection).toInt() };
    const auto session_id { QUuid(obj[kSessionId].toString()) };
    const QUuid settlement_id { QUuid(obj.value(kSettlementId).toString()) };
    const QJsonArray selected_array { obj.value(kSettlementItemSelected).toArray() };
    const auto settlement { obj.value(kSettlement).toObject() };

    auto* order_model = static_cast<TreeModelO*>(tree_model_hash_.value(section).data());
    Q_ASSERT(order_model != nullptr);

    if (session_id == session_id_) {
        emit SSettlementInsert(obj);
    }

    {
        QSet<QUuid> settled_set {};

        for (const auto& v : selected_array) {
            const QUuid node_id { v.toString() };
            settled_set.insert(node_id);
        }

        order_model->InsertSettlement(settled_set, settlement_id);
    }

    {
        auto* partner_model { static_cast<TreeModelP*>(tree_model_hash_.value(Section::kPartner).data()) };
        Q_ASSERT(partner_model != nullptr);

        const auto partner_id { QUuid(settlement.value(kPartnerId).toString()) };
        const double initial_delta { settlement.value(kAmount).toString().toDouble() };

        partner_model->UpdateAmount(partner_id, -initial_delta);
    }
}

void WebSocket::UpdateSettlement(const QJsonObject& obj)
{
    const Section section { obj.value(kSection).toInt() };
    const auto session_id { QUuid(obj[kSessionId].toString()) };
    const QUuid settlement_id { QUuid(obj.value(kSettlementId).toString()) };
    const QJsonArray selected_array { obj.value(kSettlementItemSelected).toArray() };
    const auto update { obj.value(kUpdate).toObject() };

    auto* base_model { tree_model_hash_.value(section).data() };

    auto* order_model = static_cast<TreeModelO*>(base_model);
    Q_ASSERT(order_model != nullptr);

    if (session_id == session_id_) {
        emit SSettlementUpdate(obj);
    }

    {
        QSet<QUuid> settled_set {};

        for (const auto& v : selected_array) {
            const QUuid node_id { v.toString() };
            settled_set.insert(node_id);
        }

        order_model->InsertSettlement(settled_set, settlement_id);
    }

    {
        auto* partner_model { static_cast<TreeModelP*>(tree_model_hash_.value(Section::kPartner).data()) };
        Q_ASSERT(partner_model != nullptr);

        const auto partner_id { QUuid(obj.value(kPartnerId).toString()) };
        const double initial_delta { update.value(kAmount).toString().toDouble() };

        partner_model->UpdateAmount(partner_id, -initial_delta);
    }
}

void WebSocket::RecallSettlement(const QJsonObject& obj)
{
    const Section section { obj.value(kSection).toInt() };
    const auto session_id { QUuid(obj[kSessionId].toString()) };
    const QUuid settlement_id { QUuid(obj.value(kSettlementId).toString()) };

    auto* base_model { tree_model_hash_.value(section).data() };

    auto* order_model = static_cast<TreeModelO*>(base_model);
    Q_ASSERT(order_model != nullptr);

    if (session_id == session_id_) {
        emit SSettlementRecall(obj);
    }

    {
        order_model->RecallSettlement(settlement_id);
    }

    {
        auto* partner_model { static_cast<TreeModelP*>(tree_model_hash_.value(Section::kPartner).data()) };
        Q_ASSERT(partner_model != nullptr);

        const auto partner_id { QUuid(obj.value(kPartnerId).toString()) };
        const double initial_delta { obj.value(kAmount).toString().toDouble() };

        partner_model->UpdateAmount(partner_id, initial_delta);
    }
}

void WebSocket::OnSettlementView(const QJsonObject& obj)
{
    const Section section { obj.value(kSection).toInt() };
    const QUuid widget_id { QUuid(obj.value(kWidgetId).toString()) };
    const QJsonArray array { obj.value(kArray).toArray() };

    emit SSettlementView(section, widget_id, array);
}

void WebSocket::MarkEntries(const QJsonObject& obj)
{
    const Section section { obj.value(kSection).toInt() };
    const auto session_id { QUuid(obj[kSessionId].toString()) };

    const auto node_id { QUuid(obj.value(kNodeId).toString()) };
    const MarkOperation operation { obj.value(WsField::kMarkOperation).toInt() };

    auto entry_hub { entry_hub_hash_.value(section) };

    if (session_id != session_id_) {
        entry_hub->MarkEntries(node_id, operation);
    }
}

void WebSocket::UpdateDocumentDir(const QJsonObject& obj)
{
    const Section section { obj.value(kSection).toInt() };
    const auto session_id { QUuid(obj[kSessionId].toString()) };
    CString document_dir { obj.value(WsField::kDocumentDir).toString() };

    if (session_id != session_id_) {
        emit SDocumentDirUpdate(section, document_dir);
    }
}

void WebSocket::UpdateDefaultUnit(const QJsonObject& obj)
{
    const Section section { obj.value(kSection).toInt() };
    const int default_unit { obj.value(WsField::kDefaultUnit).toInt() };

    emit SDefaultUnitUpdate(section, default_unit);
}

void WebSocket::DenyDefaultUnit(const QJsonObject& /*obj*/) { emit SDefaultUnitDeny(kFinance); }

void WebSocket::ApplySharedConfig(const QJsonArray& arr) { emit SSharedConfigApply(arr); }

QHash<QUuid, QSet<QUuid>> WebSocket::ParseLinkedEntry(const QJsonObject& obj) const
{
    QHash<QUuid, QSet<QUuid>> map {};

    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
        const QUuid key { QUuid(it.key()) };
        QSet<QUuid>& set = map[key];

        const QJsonArray arr { it.value().toArray() };
        for (const auto& val : arr) {
            set.insert(QUuid(val.toString()));
        }
    }

    return map;
}

QSet<QUuid> WebSocket::ParseLinkedEntryP(const QJsonArray& arr) const
{
    QSet<QUuid> set {};

    for (const auto& val : arr) {
        const QUuid uuid { val.toString() };
        if (!uuid.isNull())
            set.insert(uuid);
    }

    return set;
}
