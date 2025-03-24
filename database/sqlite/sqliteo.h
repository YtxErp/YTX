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

#include "sqlite.h"

class SqliteO final : public Sqlite {
    Q_OBJECT

public:
    SqliteO(CInfo& info, QObject* parent = nullptr);
    ~SqliteO();

    bool ReadNode(NodeHash& node_hash, const QDateTime& start, const QDateTime& end);
    bool SearchNode(QList<const Node*>& node_list, const QList<int>& party_id_list);
    Node* ReadNode(int node_id);

public slots:
    void RRemoveNode(int node_id, int node_type) override;
    void RUpdateStakeholder(int old_node_id, int new_node_id) const override;
    void RUpdateProduct(int old_node_id, int new_node_id) override;

protected:
    // tree
    void ReadNodeQuery(Node* node, const QSqlQuery& query) const override;
    void WriteNodeBind(Node* node, QSqlQuery& query) const override;

    QString QSReadNode() const override;
    QString QSWriteNode() const override;
    QString QSRemoveNodeSecond() const override;
    QString QSInternalReference() const override;

    // table
    void WriteTransBind(TransShadow* trans_shadow, QSqlQuery& query) const override;
    void ReadTransFunction(TransShadowList& trans_shadow_list, int node_id, QSqlQuery& query) override;
    void ReadTransQuery(Trans* trans, const QSqlQuery& query) const override;
    void SyncTransValueBind(const TransShadow* trans_shadow, QSqlQuery& query) const override;
    void WriteTransRangeFunction(const QList<TransShadow*>& list, QSqlQuery& query) const override;
    void ReadStatementQuery(TransList& trans_list, QSqlQuery& query) const override;
    void ReadStatementPrimaryQuery(QList<Node*>& node_list, QSqlQuery& query) const override;
    void ReadStatementSecondaryQuery(TransList& trans_list, QSqlQuery& query) const override;

    QString QSSyncLeafValue() const override;
    void SyncLeafValueBind(const Node* node, QSqlQuery& query) const override;

    QString QSReadTrans() const override;
    QString QSWriteTrans() const override;
    QString QSSearchTransValue() const override;
    QString QSSearchTransText() const override;
    QString QSSyncTransValue() const override;
    QString QSTransToRemove() const override;
    QString QSReadStatement(int unit) const override;
    QString QSReadBalance(int unit) const override;
    QString QSReadStatementPrimary(int unit) const override;
    QString QSReadStatementSecondary(int unit) const override;
    QString QSInvertTransValue() const override;
    QString QSSyncPriceSFirst() const override;
    QString QSSyncPriceSSecond() const override;

private:
    QString SearchNodeQS(CString& in_list) const;

    void ReadNodeFunction(NodeHash& node_hash, QSqlQuery& query);
    void SearchNodeFunction(QList<const Node*>& node_list, QSqlQuery& query);

private:
    NodeHash node_hash_ {};
};

#endif // SQLITEO_H
