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

#ifndef SQLITES_H
#define SQLITES_H

#include "sqlite.h"

class SqliteS final : public Sqlite {
    Q_OBJECT

public:
    SqliteS(CInfo& info, QObject* parent = nullptr);

public slots:
    void RReplaceNode(int old_node_id, int new_node_id, int node_type, int node_unit) override;
    void RRemoveNode(int node_id, int node_type) override;
    void RPriceSList(QList<PriceS>& list) override;
    void RUpdateProduct(int old_node_id, int new_node_id) override;

public:
    bool CrossSearch(TransShadow* order_trans_shadow, int party_id, int product_id, bool is_inside) const;
    bool ReadTrans(int node_id);

protected:
    // tree
    void ReadNodeQuery(Node* node, const QSqlQuery& query) const override;
    void WriteNodeBind(Node* node, QSqlQuery& query) const override;

    QString QSReadNode() const override;
    QString QSWriteNode() const override;
    QString QSRemoveNodeSecond() const override;
    QString QSInternalReference() const override;
    QString QSExternalReferencePS() const override;
    QString QSSupportReference() const override;
    QString QSRemoveSupport() const override;

    // table
    void ReadTransQuery(Trans* trans, const QSqlQuery& query) const override;
    void WriteTransBind(TransShadow* trans_shadow, QSqlQuery& query) const override;
    void ReadTransRefQuery(TransList& trans_list, QSqlQuery& query) const override;
    void CalculateLeafTotal(Node* node, QSqlQuery& query) const override;
    bool ReplaceLeaf(int old_node_id, int new_node_id, int node_unit) const override;

    void SyncLeafValueBind(const Node* node, QSqlQuery& query) const override;

    QString QSReadTrans() const override;
    QString QSReadSupportTrans() const override;
    QString QSWriteTrans() const override;
    QString QSSearchTransValue() const override;
    QString QSSearchTransText() const override;
    QString QSRemoveNodeFirst() const override;
    QString QSTransToRemove() const override;
    QString QSReadTransRef(int unit) const override;
    QString QSRemoveTrans() const override;
    QString QSLeafTotal(int unit) const override;
    QString QSReplaceSupport() const override;
    QString QSSyncLeafValue() const override;

private:
    void ReadTransS(QSqlQuery& query);
    bool ReadTransRange(const QSet<int>& set);

    bool ReplaceLeafC(QSqlQuery& query, int old_node_id, int new_node_id) const;
    bool ReplaceLeafE(QSqlQuery& query, int old_node_id, int new_node_id) const;
    bool ReplaceLeafV(QSqlQuery& query, int old_node_id, int new_node_id) const;

    QString QSReplaceLeafSE() const; // stakeholder employee
    QString QSReplaceLeafOSE() const; // order sales employee
    QString QSReplaceLeafOPE() const; // order purchase employee

    QString QSReplaceLeafOSP() const; // order sales party
    QString QSReplaceLeafOPP() const; // order purchase party
};

#endif // SQLITES_H
