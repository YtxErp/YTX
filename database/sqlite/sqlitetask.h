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

#ifndef SQLITETASK_H
#define SQLITETASK_H

#include "sqlite.h"

class SqliteTask final : public Sqlite {
public:
    SqliteTask(CInfo& info, QObject* parent = nullptr);

protected:
    QString QSReadNode() const override;
    QString QSWriteNode() const override;
    QString QSRemoveNodeSecond() const override;
    QString QSInternalReference() const override;
    QString QSSupportReference() const override;
    QString QSReplaceSupportTransFPTS() const override;
    QString QSRemoveSupport() const override;
    QString QSLeafTotalFPT() const override;
    QString QSFreeViewFPT() const override;
    QString QSSupportTransToMove() const override;

    QString QSTransToRemove() const override;
    QString QSSupportTransToRemove() const override;

    QString QSReadTrans() const override;
    QString QSReadSupportTransFPTS() const override;
    QString QSWriteTrans() const override;
    QString QSReadTransRangeFPTS(CString& in_list) const override;
    QString QSReplaceNodeTransFPTS() const override;
    QString QSWriteTransValueFPTO() const override;
    QString QSSearchTrans() const override;

    void ReadTransQuery(Trans* trans, const QSqlQuery& query) const override;
    void WriteTransBind(TransShadow* trans_shadow, QSqlQuery& query) const override;
    void WriteTransValueBindFPTO(const TransShadow* trans_shadow, QSqlQuery& query) const override;

    QString QSWriteLeafValueFPTO() const override;
    void WriteLeafValueBindFPTO(const Node* node, QSqlQuery& query) const override;

    void WriteNodeBind(Node* node, QSqlQuery& query) const override;
    void ReadNodeQuery(Node* node, const QSqlQuery& query) const override;
};

#endif // SQLITETASK_H
