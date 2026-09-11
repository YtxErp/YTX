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

#include <QDateTime>
#include <QJsonObject>
#include <QTimer>
#include <QUuid>

#include "enum/stateenum.h"
#include "finance/finance_role.h"

struct Entry {
    QUuid id {};
    QDateTime issued_time {};
    QString code {};
    QUuid lhs_node {};
    QString description {};
    QStringList document {};
    QStringList tag {};
    int status {};
    QUuid rhs_node {};

    // New local object starts as pending creation
    SyncState sync_state { SyncState::kCreating };

    // NOTE: Following fields are only used by base Entry, not EntryP or EntryO
    double lhs_rate {};
    double rhs_rate {};
    double lhs_debit {};
    double lhs_credit {};
    double rhs_debit {};
    double rhs_credit {};

    int version {};

    virtual ~Entry() = default;
    virtual void Reset();
    virtual void ReadJson(const QJsonObject& object);
    virtual QJsonObject WriteJson() const { return QJsonObject(); }
};

struct EntryF final : Entry {
    finance::CashKind cash_kind {};

    void Reset() override;
    void ReadJson(const QJsonObject& object) override;
};

struct EntryP final : Entry {
    double unit_price {};
    QString external_sku {};

    void Reset() override;
    void ReadJson(const QJsonObject& object) override;
    QJsonObject WriteJson() const override;
};

struct EntryO final : Entry {
    double unit_price {};
    double count {};
    double measure {};

    double initial {};

    void Reset() override;
    void ReadJson(const QJsonObject& object) override;
    QJsonObject WriteJson() const override;
};

struct PendingEntryUpdate {
    QTimer* timer {};
    Entry* entry {};
    QJsonObject changes {};
};

using EntryList = QList<Entry*>;
