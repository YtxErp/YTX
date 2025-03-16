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

#ifndef SQLITEF_H
#define SQLITEF_H

#include "sqlite.h"

class SqliteF final : public Sqlite {
public:
    SqliteF(CInfo& info, QObject* parent = nullptr);

protected:
    void WriteNodeBind(Node* node, QSqlQuery& query) const override;
    void ReadNodeQuery(Node* node, const QSqlQuery& query) const override;

    void ReadTransQuery(Trans* trans, const QSqlQuery& query) const override;
    void WriteTransBind(TransShadow* trans_shadow, QSqlQuery& query) const override;
    void SyncTransValueBind(const TransShadow* trans_shadow, QSqlQuery& query) const override;

    QString QSReadNode() const override;
    QString QSWriteNode() const override;
    QString QSRemoveNodeSecond() const override;
    QString QSInternalReference() const override;
    QString QSSupportReference() const override;
    QString QSReplaceSupport() const override;
    QString QSRemoveSupport() const override;
    QString QSLeafTotal(int unit) const override;
    QString QSTransToRemove() const override;

    QString QSSyncLeafValue() const override;
    void SyncLeafValueBind(const Node* node, QSqlQuery& query) const override;

    QString QSReadTrans() const override;
    QString QSReadSupportTrans() const override;
    QString QSWriteTrans() const override;
    QString QSReplaceLeaf() const override;
    QString QSSyncTransValue() const override;
    QString QSSearchTransValue() const override;
    QString QSSearchTransText() const override;
};

#endif // SQLITEF_H
