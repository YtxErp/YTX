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

#ifndef SQLITEORDER_H
#define SQLITEORDER_H

#include "sqlite.h"

class SqliteOrder final : public Sqlite {
    Q_OBJECT

public:
    SqliteOrder(CInfo& info, QObject* parent = nullptr);
    ~SqliteOrder();

    bool ReadNode(NodeHash& node_hash, const QDate& start_date, const QDate& end_date);
    bool SearchNode(QList<const Node*>& node_list, const QList<int>& party_id_list);
    bool RetriveNode(NodeHash& node_hash, int node_id);

public slots:
    void RRemoveNode(int node_id, int node_type) override;

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
    void ReadTransFunction(TransShadowList& trans_shadow_list, int node_id, QSqlQuery& query, bool is_support = false) override;
    void ReadTransQuery(Trans* trans, const QSqlQuery& query) const override;
    void UpdateProductReferenceSO(int old_node_id, int new_node_id) const override;
    void UpdateStakeholderReferenceO(int old_node_id, int new_node_id) const override;
    void WriteTransValueBindFPTO(const TransShadow* trans_shadow, QSqlQuery& query) const override;

    QString QSWriteLeafValueFPTO() const override;
    void WriteLeafValueBindFPTO(const Node* node, QSqlQuery& query) const override;

    QString QSReadNodeTrans() const override;
    QString QSWriteNodeTrans() const override;
    QString QSUpdateProductReferenceSO() const override;
    QString QSUpdateStakeholderReferenceO() const override;
    QString QSSearchTrans() const override;
    QString QSWriteTransValueFPTO() const override;
    QString QSNodeTransToRemove() const override;

private:
    QString SearchNodeQS(CString& in_list) const;

private:
    NodeHash node_hash_buffer_ {};
};

#endif // SQLITEORDER_H
