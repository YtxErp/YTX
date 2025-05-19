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

#ifndef SQLITEO_H
#define SQLITEO_H

#include "database/sqlite/prices.h"
#include "sqlite.h"

class SqliteO final : public Sqlite {
    Q_OBJECT

public:
    SqliteO(CInfo& info, QObject* parent = nullptr);
    ~SqliteO();

signals:
    // send to sqlite stakeholder
    void SSyncPrice(const QList<PriceS>& list);

public slots:
    void RRemoveNode(int node_id, int node_type) override;

    void RSyncProduct(int old_node_id, int new_node_id) const override;
    void RSyncStakeholder(int old_node_id, int new_node_id) const override;

public:
    bool ReadNode(NodeHash& node_hash, const QDateTime& start, const QDateTime& end);
    bool SearchNode(QList<const Node*>& node_list, const QList<int>& party_id_list);
    Node* ReadNode(int node_id);

    bool SettlementReference(int settlement_id) const;
    int SettlementID(int node_id) const;

    bool ReadSettlement(NodeList& node_list, const QDateTime& start, const QDateTime& end) const;
    bool WriteSettlement(Node* node) const;
    bool RemoveSettlement(int settlement_id);

    bool ReadSettlementPrimary(NodeList& node_list, int party_id, int settlement_id, bool finished);
    bool AddSettlementPrimary(int node_id, int settlement_id) const;
    bool RemoveSettlementPrimary(int node_id) const;

    bool SyncPrice(int node_id);
    bool InvertTransValue(int node_id) const;

    bool ReadStatement(TransList& trans_list, int unit, const QDateTime& start, const QDateTime& end) const;
    bool ReadBalance(double& pbalance, double& cdelta, int party_id, int unit, const QDateTime& start, const QDateTime& end) const;
    bool ReadStatementPrimary(NodeList& node_list, int party_id, int unit, const QDateTime& start, const QDateTime& end) const;
    bool ReadStatementSecondary(TransList& trans_list, int party_id, int unit, const QDateTime& start, const QDateTime& end) const;

    bool WriteTransRange(const QList<TransShadow*>& list);

protected:
    // tree
    void ReadNodeQuery(Node* node, const QSqlQuery& query) const override;
    void WriteNodeBind(Node* node, QSqlQuery& query) const override;

    QString QSReadNode() const override;
    QString QSWriteNode() const override;
    QString QSRemoveNodeSecond() const override;
    QString QSInternalReference() const override;
    QString QSExternalReference() const override;

    // table
    void WriteTransBind(TransShadow* trans_shadow, QSqlQuery& query) const override;
    void ReadTransFunction(TransShadowList& trans_shadow_list, int node_id, QSqlQuery& query) override;
    void ReadTransQuery(Trans* trans, const QSqlQuery& query) const override;
    void UpdateTransValueBind(const TransShadow* trans_shadow, QSqlQuery& query) const override;

    QString QSUpdateLeafValue() const override;
    void UpdateLeafValueBind(const Node* node, QSqlQuery& query) const override;

    QString QSReadTrans() const override;
    QString QSWriteTrans() const override;
    QString QSSearchTransValue() const override;
    QString QSSearchTransText() const override;
    QString QSUpdateTransValue() const override;
    QString QSTransToRemove() const override;

private:
    QString QSSearchNode(CString& in_list) const;

    void ReadNodeFunction(NodeHash& node_hash, QSqlQuery& query);
    void SearchNodeFunction(QList<const Node*>& node_list, QSqlQuery& query);

    QString QSReadSettlement() const;
    void ReadSettlementQuery(NodeList& node_list, QSqlQuery& query) const;
    QString QSWriteSettlement() const;
    void WriteSettlementBind(Node* node, QSqlQuery& query) const;

    QString QSRemoveSettlementFirst() const;
    QString QSRemoveSettlementSecond() const;

    QString QSReadSettlementPrimary(bool finished) const;

    void ReadSettlementPrimaryQuery(NodeList& node_list, QSqlQuery& query);

    QString QSSyncPriceFirst() const;
    QString QSSyncPriceSecond() const;
    QString QSInvertTransValue() const;

    QString QSReadStatement(int unit) const;
    QString QSReadBalance(int unit) const;
    QString QSReadStatementPrimary(int unit) const;
    QString QSReadStatementSecondary(int unit) const;

    void ReadStatementQuery(TransList& trans_list, QSqlQuery& query) const;
    void ReadStatementPrimaryQuery(NodeList& node_list, QSqlQuery& query) const;
    void ReadStatementSecondaryQuery(TransList& trans_list, QSqlQuery& query) const;

    void WriteTransRangeFunction(const QList<TransShadow*>& list, QSqlQuery& query) const;

private:
    NodeHash node_hash_ {};
};

#endif // SQLITEO_H
