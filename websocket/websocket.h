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

#include <QObject>
#include <QPointer>
#include <QSettings>
#include <QWebSocket>

#include "component/constantint.h"
#include "component/constantwebsocket.h"
#include "entryhub/entryhub.h"
#include "tree/model/treemodel.h"

class WebSocket final : public QObject {
    Q_OBJECT

public:
    static WebSocket* Instance();

    void ReadConfig(const QSharedPointer<QSettings>& local_settings);
    void Connect();
    void SendMessage(WsKey key, const QJsonObject& value);
    void Reset();

    void RegisterTreeModel(Section section, QPointer<TreeModel> node) { tree_model_hash_.insert(section, node); }
    void RegisterEntryHub(Section section, QPointer<EntryHub> entry_hub) { entry_hub_hash_.insert(section, entry_hub); }

    WebSocket(const WebSocket&) = delete;
    WebSocket& operator=(const WebSocket&) = delete;
    WebSocket(WebSocket&&) = delete;
    WebSocket& operator=(WebSocket&&) = delete;

signals:
    void SConnectionAllow();
    void SConnectionDeny();

    void SLoginAllow(const QString& name, const QString& expire_date);
    void SLoginDeny(int code);

    void SRegisterResult(bool result, int code);
    void SRemoteHostClosed();
    void SEntrySelect(const QUuid& node_id, const QUuid& entry_id);

    void SLeafDeleteDeny(const QJsonObject& obj);
    void SSharedConfigApply(const QJsonArray& arr);
    void SDocumentDirUpdate(Section section, const QString& document_dir);
    void SDefaultUnitUpdate(Section section, int unit);
    void SDefaultUnitDeny(const QString& section);

    void STagApply(const QJsonObject& obj);
    void STagInsert(const QJsonObject& obj);
    void STagUpdate(const QJsonObject& obj);
    void STagDelete(const QJsonObject& obj);

    void SReplaceResult(bool result);
    void SOrderHistory(Section section, const QUuid& widget_id, const QJsonArray& array);
    void SStatementPrimary(Section section, const QUuid& widget_id, const QJsonArray& array);
    void SStatementSecondary(Section section, const QUuid& widget_id, const QJsonArray& array);
    void SStatementTertiary(Section section, const QUuid& widget_id, const QJsonArray& array);
    void SSettlementPrimary(Section section, const QUuid& widget_id, const QJsonArray& array);
    void SSettlementSecondary(Section section, const QUuid& widget_id, const QJsonArray& array);
    void SSettlementInsert(const QJsonObject& obj);
    void SSettlementRecall(const QJsonObject& obj);
    void SSettlementUpdate(const QJsonObject& obj);
    void SSettlementView(Section section, const QUuid& widget_id, const QJsonArray& array);
    void SOrderRelease(Section section, const QUuid& node_id, int version);
    void SOrderRecall(Section section, const QUuid& node_id, int version);
    void SOrderSave(Section section, const QUuid& node_id, int version);
    void SOperationDeny();
    void SNodeSelect(Section section, const QUuid& node_id);
    void SNodeLocate(Section section, const QUuid& node_id);
    void STreeSyncFinish();
    void SAccountName(const QString& name);
    void SAccountUsername(const QJsonObject& obj);
    void SWorkspaceMemberAck(const QUuid& widget_id, const QJsonArray& array);
    void SAuditLogAck(const QUuid& widget_id, const QJsonArray& array);
    void SAccountRoleUpdate();
    void SInventoryHeatAck(const QUuid& widget_id, const QJsonArray& array);
    void SPartnerHeatAck(const QUuid& widget_id, const QJsonArray& array);
    void SBalanceSheetAck(const QUuid& widget_id, const QJsonArray& node_array, const QJsonArray& path_array);
    void SIncomeStatementAck(const QUuid& widget_id, const QJsonObject& obj);
    void SCashFlowStatementAck(const QUuid& widget_id, const QJsonObject& obj);

    // send to SearchNodeModel
    void SOrderSearch(const QJsonObject& obj);

private slots:
    void RConnected();
    void RDisconnected();
    void RBinaryMessageReceived(const QByteArray& data);
    void RErrorOccurred(QAbstractSocket::SocketError error);
    void RPong() { timeout_timer_->start(time_const::kTimeoutThresholdMs); }

    void RSendPing()
    {
        if (socket_.state() == QAbstractSocket::ConnectedState) {
            socket_.ping();
        }
    }

    void RTimeout()
    {
        qWarning() << "Heartbeat timeout, closing connection";
        socket_.close(QWebSocketProtocol::CloseCodeGoingAway, "Heartbeat timeout");
        ping_timer_->stop();
    }

private:
    explicit WebSocket(QObject* parent = nullptr);
    ~WebSocket() override;

    void InitHandler();
    void InitConnect();
    void InitTimer();

    void HandleMessage(WsKey key, const QByteArray& payload);

    void StopTimer()
    {
        if (ping_timer_)
            ping_timer_->stop();

        if (timeout_timer_)
            timeout_timer_->stop();
    }

    QHash<QUuid, QSet<QUuid>> ParseLinkedEntry(const QJsonObject& obj) const;
    QSet<QUuid> ParseLinkedEntryP(const QJsonArray& arr) const;

private:
    void NotifyRegisterOutcome(const QJsonObject& obj);
    void NotifyLoginOutcome(const QJsonObject& obj);

    void AckOrderTree(const QJsonObject& obj);
    void AckTable(const QJsonObject& obj);
    void AckOrderNode(const QJsonObject& obj);
    void OnOrderHistory(const QJsonObject& obj);
    void OnStatementPrimary(const QJsonObject& obj);
    void OnStatementSecondary(const QJsonObject& obj);
    void OnStatementTertiary(const QJsonObject& obj);
    void OnSettlementPrimary(const QJsonObject& obj);
    void OnSettlementSecondary(const QJsonObject& obj);
    void AckWorkspaceMember(const QJsonObject& obj);
    void AckAuditLog(const QJsonObject& obj);
    void AckInventoryHeat(const QJsonObject& obj);
    void AckPartnerHeat(const QJsonObject& obj);
    void AckBalanceSheet(const QJsonObject& obj);
    void AckIncomeStatement(const QJsonObject& obj);
    void AckCashFlowStatement(const QJsonObject& obj);

    void SearchEntry(const QJsonObject& obj);
    void SearchOrder(const QJsonObject& obj);

    void DenyLeafDelete(const QJsonObject& obj);
    void DenyDefaultUnit(const QJsonObject& obj);
    void DenyOperation();
    void FinishTreeSync();

private:
    void ApplySharedConfig(const QJsonArray& arr);
    void ApplyTree(const QJsonObject& obj);
    void ApplyTag(const QJsonObject& obj);
    void ApplyPartnerEntry(const QJsonArray& arr);

    void InsertNode(const QJsonObject& obj);
    void UpdateNode(const QJsonObject& obj);
    void DragNode(const QJsonObject& obj);
    void ReplaceLeaf(const QJsonObject& obj);
    void DeleteLeaf(const QJsonObject& obj);
    void DeleteOrderLeaf(const QJsonObject& obj);
    void DeletePartnerLeaf(const QJsonObject& obj);
    void CommitDeleteNode(const QJsonObject& obj);
    void UpdateNodeDirectionRule(const QJsonObject& obj);
    void UpdateNodeStatus(const QJsonObject& obj);
    void UpdateNodeName(const QJsonObject& obj);
    void UpdateAccountName(const QJsonObject& obj);
    void UpdateAccountUsername(const QJsonObject& obj);
    void UpdateAccountRole(const QJsonObject& obj);

    void InsertTag(const QJsonObject& obj);
    void UpdateTag(const QJsonObject& obj);
    void DeleteTag(const QJsonObject& obj);

    void PeriodClose(const QJsonObject& obj);

    void ReleaseOrder(const QJsonObject& obj);
    void SaveOrder(const QJsonObject& obj);
    void InsertOrder(const QJsonObject& obj, bool is_released);
    void RecallOrder(const QJsonObject& obj);

    void InsertSettlement(const QJsonObject& obj);
    void UpdateSettlement(const QJsonObject& obj);
    void RecallSettlement(const QJsonObject& obj);
    void OnSettlementView(const QJsonObject& obj);

    void UpdateDocumentDir(const QJsonObject& obj);
    void UpdateDefaultUnit(const QJsonObject& obj);

    void UpdateEntry(const QJsonObject& obj);
    void InsertEntry(const QJsonObject& obj);
    void InsertPartnerEntry(const QJsonObject& obj);
    void DeleteEntry(const QJsonObject& obj);
    void DeletePartnerEntry(const QJsonObject& obj);
    void MarkEntries(const QJsonObject& obj);
    void UpdateEntryLinkedNode(const QJsonObject& obj);
    void UpdateEntryRate(const QJsonObject& obj);
    void UpdateEntryNumeric(const QJsonObject& obj);

private:
    QWebSocket socket_ {};
    QUuid session_id_ {};

    QUrl server_url_ {};
    bool manual_disconnect_ {};

    QTimer* ping_timer_ {};
    QTimer* timeout_timer_ {};

    QHash<WsKey, std::function<void(const QJsonObject&)>> handler_obj_ {};
    QHash<WsKey, std::function<void(const QJsonArray&)>> handler_arr_ {};
    QHash<Section, QPointer<TreeModel>> tree_model_hash_ {};
    QHash<Section, QPointer<EntryHub>> entry_hub_hash_ {};
};
