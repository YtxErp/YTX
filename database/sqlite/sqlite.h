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

#ifndef SQLITE_H
#define SQLITE_H

#include <QObject>
#include <QSqlDatabase>

#include "component/enumclass.h"
#include "component/info.h"
#include "component/using.h"
#include "table/trans.h"
#include "tree/node.h"

class Sqlite : public QObject {
    Q_OBJECT

public:
    virtual ~Sqlite();

protected:
    Sqlite(CInfo& info, QObject* parent = nullptr);

signals:
    // send to all TableModel
    void SRemoveMultiTrans(const QMultiHash<int, int>& leaf_trans);
    void SMoveMultiTrans(int old_node_id, int new_node_id, const QList<int>& trans_id_list);
    // send to SupportStation
    void SMoveMultiSupportTrans(Section section, int new_support_id, const QList<int>& trans_id_list);
    void SRemoveMultiSupportTrans(const QMultiHash<int, int>& support_trans);
    // send to TreeModel
    void SUpdateMultiLeafTotal(const QList<int>& node_id_list);
    void SRemoveNode(int node_id);
    // send to Mainwindow
    void SFreeWidget(int node_id);
    // send to sql itsself
    void SUpdateProduct(int old_node_id, int new_node_id);
    // send to sql itsself and treemodel
    void SUpdateStakeholder(int old_node_id, int new_node_id);

public slots:
    // receive from remove node dialog
    virtual void RRemoveNode(int node_id, int node_type);
    virtual void RReplaceNode(int old_node_id, int new_node_id, int node_type);
    // receive from sql
    void RUpdateProduct(int old_node_id, int new_node_id);
    void RUpdateStakeholder(int old_node_id, int new_node_id);

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
    bool WriteLeafValue(const Node* node) const;
    QList<int> SearchNodeName(CString& text) const;

    // table
    bool ReadTrans(TransShadowList& trans_shadow_list, int node_id);
    bool ReadSupportTrans(TransShadowList& trans_shadow_list, int support_id);
    bool ReadTransRange(TransShadowList& trans_shadow_list, int node_id, const QList<int>& trans_id_list);
    bool WriteTrans(TransShadow* trans_shadow);
    bool WriteTransRange(const QList<TransShadow*>& list) const;
    bool WriteTransValue(const TransShadow* trans_shadow) const;
    TransShadow* AllocateTransShadow();

    bool RemoveTrans(int trans_id);
    bool WriteState(Check state) const;
    bool SearchTrans(TransList& trans_list, CString& text) const;
    bool ReadTransRef(TransList& trans_list, int node_id) const;
    bool ReadStatement(TransList& trans_list, const QDateTime& start, const QDateTime& end, UnitO unit = UnitO::kMS) const;

    // common
    bool WriteField(CString& table, CString& field, CVariant& value, int id) const;

protected:
    // QS means QueryString
    // tree
    virtual QString QSReadNode() const = 0;
    virtual QString QSWriteNode() const = 0;
    virtual QString QSRemoveNodeSecond() const = 0;
    virtual QString QSInternalReference() const = 0;
    virtual QString QSSearchTrans() const = 0;

    virtual QString QSExternalReferencePS() const { return {}; }
    virtual QString QSSupportReference() const { return {}; }
    virtual QString QSRemoveSupport() const { return {}; }
    virtual QString QSLeafTotalFPT() const { return {}; }
    virtual QString QSWriteLeafValueFPTO() const { return {}; }
    virtual QString QSSupportTransToMove() const { return {}; }
    virtual QString QSRemoveNodeFirst() const;
    virtual QString QSReadTransRef() const { return {}; };
    virtual QString QSReadStatement(UnitO unit) const
    {
        Q_UNUSED(unit);
        return {};
    }

    virtual void ReadNodeQuery(Node* node, const QSqlQuery& query) const = 0;
    virtual void WriteNodeBind(Node* node, QSqlQuery& query) const = 0;

    virtual void WriteLeafValueBindFPTO(const Node* node, QSqlQuery& query) const
    {
        Q_UNUSED(node);
        Q_UNUSED(query);
    };

    //
    QString QSRemoveBranch() const;
    QString QSRemoveNodeThird() const;
    QString QSDragNodeFirst() const;
    QString QSDragNodeSecond() const;

    //
    void CalculateLeafTotal(Node* node, QSqlQuery& query) const;
    bool DBTransaction(std::function<bool()> function) const;
    bool ReadRelationship(const NodeHash& node_hash, QSqlQuery& query) const;
    bool WriteRelationship(int node_id, int parent_id, QSqlQuery& query) const;

    // table
    virtual QString QSReadTrans() const = 0;
    virtual QString QSWriteTrans() const = 0;
    virtual QString QSTransToRemove() const = 0;

    virtual QString QSReadSupportTransFPTS() const { return {}; }
    virtual QString QSSupportTransToRemove() const { return {}; }
    virtual QString QSReplaceNodeTransFPTS() const { return {}; }
    virtual QString QSReplaceSupportTransFPTS() const { return {}; }
    virtual QString QSReadTransRangeFPTS(CString& in_list) const
    {
        Q_UNUSED(in_list);
        return {};
    }

    virtual QString QSWriteTransValueFPTO() const { return {}; }
    virtual QString QSFreeViewFPT() const { return {}; }
    virtual QString QSUpdateProductReferenceSO() const { return {}; }
    virtual QString QSUpdateStakeholderReferenceO() const { return {}; }

    virtual void ReadTransQuery(Trans* trans, const QSqlQuery& query) const = 0;
    virtual void WriteTransBind(TransShadow* trans_shadow, QSqlQuery& query) const = 0;
    virtual void WriteTransValueBindFPTO(const TransShadow* trans_shadow, QSqlQuery& query) const
    {
        Q_UNUSED(trans_shadow);
        Q_UNUSED(query);
    }
    virtual void UpdateProductReferenceSO(int old_node_id, int new_node_id) const
    {
        Q_UNUSED(old_node_id);
        Q_UNUSED(new_node_id);
    }
    virtual void UpdateStakeholderReferenceO(int old_node_id, int new_node_id) const
    {
        Q_UNUSED(old_node_id);
        Q_UNUSED(new_node_id);
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

    virtual void WriteTransRangeFunction(const QList<TransShadow*>& list, QSqlQuery& query) const
    {
        Q_UNUSED(list);
        Q_UNUSED(query);
    };

    //

    virtual void ReadTransFunction(TransShadowList& trans_shadow_list, int node_id, QSqlQuery& query, bool is_support = false);
    virtual QMultiHash<int, int> ReplaceNodeFunction(int old_node_id, int new_node_id) const;

    //
    void ConvertTrans(Trans* trans, TransShadow* trans_shadow, bool left) const;
    QMultiHash<int, int> TransToRemove(int node_id, int node_type) const;
    QList<int> SupportTransToMove(int support_id) const;
    void RemoveSupportFunction(int support_id) const;
    void ReplaceSupportFunction(int old_support_id, int new_support_id);
    bool FreeView(int old_node_id, int new_node_id) const;

protected:
    QHash<int, Trans*> trans_hash_ {};
    Trans* last_trans_ {};

    QSqlDatabase* db_ {};
    CInfo& info_;
};

#endif // SQLITE_H
