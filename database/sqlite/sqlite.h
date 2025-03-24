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

#ifndef SQLITE_H
#define SQLITE_H

#include <QObject>
#include <QSqlDatabase>

#include "component/enumclass.h"
#include "component/info.h"
#include "component/using.h"
#include "database/sqlite/prices.h"
#include "table/trans.h"
#include "tree/node.h"

class Sqlite : public QObject {
    Q_OBJECT

public:
    virtual ~Sqlite();

protected:
    Sqlite(CInfo& info, QObject* parent = nullptr);

signals:
    // send to TreeModel
    void SUpdateMultiLeafTotal(const QList<int>& node_id_list);
    void SRemoveNode(int node_id);
    // send to Mainwindow
    void SFreeWidget(int node_id, int node_type);
    // send to sql itsself
    void SUpdateProduct(int old_node_id, int new_node_id);
    // send to sql itsself and treemodel
    void SUpdateStakeholder(int old_node_id, int new_node_id);
    // send to sql itsself
    void SPriceSList(QList<PriceS>& list);
    // send to LeafSStation, Stakeholder transactions are removed directly
    void SRemoveMultiTransL(Section section, const QMultiHash<int, int>& leaf_trans);
    void SMoveMultiTransL(Section section, int old_node_id, int new_node_id, const QSet<int>& trans_id_set);
    void SAppendMultiTrans(Section section, int node_id, const TransShadowList& trans_shadow);
    // send to SupportSStation
    void SRemoveMultiTransS(Section section, const QMultiHash<int, int>& support_trans);
    void SMoveMultiTransS(Section section, int old_node_id, int new_node_id, const QSet<int>& trans_id_set);

public slots:
    // receive from remove node dialog
    virtual void RRemoveNode(int node_id, int node_type);
    virtual void RReplaceNode(int old_node_id, int new_node_id, int node_type, int node_unit);
    // receive from sql
    virtual void RUpdateProduct(int old_node_id, int new_node_id)
    {
        Q_UNUSED(old_node_id);
        Q_UNUSED(new_node_id);
    };
    virtual void RUpdateStakeholder(int old_node_id, int new_node_id) const
    {
        Q_UNUSED(old_node_id);
        Q_UNUSED(new_node_id);
    };
    // receive from sql
    virtual void RPriceSList(QList<PriceS>& list) { Q_UNUSED(list) };

public:
    // tree
    bool ReadNode(NodeHash& node_hash);
    bool RemoveNode(int node_id, int node_type) const;
    bool WriteNode(int parent_id, Node* node) const;
    bool DragNode(int destination_node_id, int node_id) const;
    bool InternalReference(int node_id) const;
    bool ExternalReference(int node_id) const;
    bool SupportReference(int support_id) const;
    bool ReadLeafTotal(Node* node) const;
    bool SyncLeafValue(const Node* node) const;
    bool SearchNodeName(QSet<int>& node_id_set, CString& text) const;
    bool ReadStatementPrimary(QList<Node*>& node_list, int party_id, int unit, const QDateTime& start, const QDateTime& end) const;

    bool SyncPriceS(int node_id);
    bool InvertTransValue(int node_id) const;

    // table
    bool ReadTrans(TransShadowList& trans_shadow_list, int node_id);
    bool RetrieveTransRange(TransShadowList& trans_shadow_list, int node_id, const QSet<int>& trans_id_set);
    bool WriteTrans(TransShadow* trans_shadow);
    bool WriteTransRange(const QList<TransShadow*>& list) const;
    bool SyncTransValue(const TransShadow* trans_shadow) const;
    TransShadow* AllocateTransShadow();

    bool WriteState(Check state) const;

    bool ReadSupportTrans(TransList& trans_list, int support_id);
    bool RetrieveTransRange(TransList& trans_shadow_list, const QSet<int>& trans_id_set);
    bool SearchTrans(TransList& trans_list, CString& text);
    bool RemoveTrans(int trans_id);

    bool ReadTransRef(TransList& trans_list, int node_id, int unit, const QDateTime& start, const QDateTime& end) const;
    bool ReadStatement(TransList& trans_list, int unit, const QDateTime& start, const QDateTime& end) const;
    bool ReadBalance(double& pbalance, double& cdelta, int party_id, int unit, const QDateTime& start, const QDateTime& end) const;
    bool ReadStatementSecondary(TransList& trans_list, int party_id, int unit, const QDateTime& start, const QDateTime& end) const;

    // common
    bool WriteField(CString& table, CString& field, CVariant& value, int id) const;

protected:
    // QS means QueryString
    // tree
    virtual QString QSReadNode() const = 0;
    virtual QString QSWriteNode() const = 0;
    virtual QString QSRemoveNodeSecond() const = 0;
    virtual QString QSInternalReference() const = 0;
    virtual QString QSSearchTransValue() const = 0;
    virtual QString QSSearchTransText() const = 0;
    virtual QString QSSyncLeafValue() const = 0;

    virtual QString QSExternalReferencePS() const { return {}; }
    virtual QString QSSupportReference() const { return {}; }
    virtual QString QSRemoveSupport() const { return {}; }
    virtual QString QSRemoveNodeFirst() const;
    virtual QString QSReadTransRef(int unit) const
    {
        Q_UNUSED(unit);
        return {};
    };

    virtual QString QSLeafTotal(int unit = 0) const
    {
        Q_UNUSED(unit);
        return {};
    }
    virtual QString QSReadStatement(int unit) const
    {
        Q_UNUSED(unit);
        return {};
    }
    virtual QString QSReadBalance(int unit) const
    {
        Q_UNUSED(unit);
        return {};
    }
    virtual QString QSReadStatementPrimary(int unit) const
    {
        Q_UNUSED(unit);
        return {};
    }

    virtual QString QSReadStatementSecondary(int unit) const
    {
        Q_UNUSED(unit);
        return {};
    }

    virtual void ReadNodeQuery(Node* node, const QSqlQuery& query) const = 0;
    virtual void WriteNodeBind(Node* node, QSqlQuery& query) const = 0;
    virtual void SyncLeafValueBind(const Node* node, QSqlQuery& query) const = 0;

    //
    QString QSRemoveBranch() const;
    QString QSRemoveNodeThird() const;
    QString QSDragNodeFirst() const;
    QString QSDragNodeSecond() const;
    QString QSFreeView() const;

    //
    bool DBTransaction(std::function<bool()> function) const;
    bool ReadRelationship(const NodeHash& node_hash, QSqlQuery& query) const;
    bool WriteRelationship(int node_id, int parent_id, QSqlQuery& query) const;
    bool RemoveNode(int old_node_id) const;

    virtual bool ReplaceLeaf(int old_node_id, int new_node_id, int node_unit) const;
    bool ReplaceSupport(int old_node_id, int new_node_id);
    void ReplaceLeafFunction(QSet<int>& trans_id_set, int old_node_id, int new_node_id) const;

    // table
    virtual QString QSReadTrans() const = 0;
    virtual QString QSWriteTrans() const = 0;
    virtual QString QSTransToRemove() const = 0;

    virtual QString QSReadSupportTrans() const { return {}; }
    virtual QString QSRemoveTrans() const;
    virtual QString QSReplaceLeaf() const { return {}; }
    virtual QString QSReplaceSupport() const { return {}; }

    virtual QString QSSyncPriceSFirst() const { return {}; };
    virtual QString QSSyncPriceSSecond() const { return {}; };
    virtual QString QSSyncTransValue() const { return {}; }
    virtual QString QSInvertTransValue() const { return {}; }

    virtual void ReadTransQuery(Trans* trans, const QSqlQuery& query) const = 0;
    virtual void WriteTransBind(TransShadow* trans_shadow, QSqlQuery& query) const = 0;

    virtual void SyncTransValueBind(const TransShadow* trans_shadow, QSqlQuery& query) const
    {
        Q_UNUSED(trans_shadow);
        Q_UNUSED(query);
    }

    virtual void ReadTransRefQuery(TransList& trans_list, QSqlQuery& query) const
    {
        Q_UNUSED(trans_list);
        Q_UNUSED(query);
    };

    virtual void ReadStatementQuery(TransList& trans_list, QSqlQuery& query) const
    {
        Q_UNUSED(trans_list);
        Q_UNUSED(query);
    };

    virtual void ReadStatementPrimaryQuery(QList<Node*>& node_list, QSqlQuery& query) const
    {
        Q_UNUSED(node_list);
        Q_UNUSED(query);
    };

    virtual void ReadStatementSecondaryQuery(TransList& trans_list, QSqlQuery& query) const
    {
        Q_UNUSED(trans_list);
        Q_UNUSED(query);
    };

    virtual void WriteTransRangeFunction(const QList<TransShadow*>& list, QSqlQuery& query) const
    {
        Q_UNUSED(list);
        Q_UNUSED(query);
    };

    //

    virtual void ReadTransFunction(TransShadowList& trans_shadow_list, int node_id, QSqlQuery& query);
    virtual void CalculateLeafTotal(Node* node, QSqlQuery& query) const;

    //
    void ConvertTrans(Trans* trans, TransShadow* trans_shadow, bool left) const;
    void TransToRemove(QMultiHash<int, int>& leaf_trans, QMultiHash<int, int>& support_trans, int node_id, int node_type) const;
    void RemoveSupportFunction(int support_id) const;
    void RemoveLeafFunction(const QMultiHash<int, int>& leaf_trans);
    void ReplaceSupportFunction(QSet<int>& trans_id_set, int old_support_id, int new_support_id);
    bool FreeView(int old_node_id, int new_node_id) const;
    void ReadTransFunction(TransList& trans_list, QSqlQuery& query);

protected:
    QHash<int, Trans*> trans_hash_ {};
    Trans* last_trans_ {};
    const Section section_ {};

    QSqlDatabase* db_ {};
    CInfo& info_;
};

#endif // SQLITE_H
