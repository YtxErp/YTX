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

#include <QJsonObject>
#include <QTimer>
#include <QUuid>

#include "enum/nodeenum.h"
#include "enum/stateenum.h"
#include "enum/statusenum.h"
#include "finance/finance_role.h"

struct Node {
    QString name {};
    QUuid id {};
    QString code {};
    QString description {};
    NodeKind kind {};
    NodeStatus status {};
    bool direction_rule {};
    NodeUnit unit {};
    QString color {};
    QStringList tag {};
    QStringList document {};

    // New local object starts as pending creation
    SyncState sync_state { SyncState::kCreating };

    double final_total {};
    double initial_total {};

    int version {};

    Node* parent {};
    QList<Node*> children {};

    bool IsValid() const { return !id.isNull(); }

    virtual void Reset();
    virtual void InvertTotal();

    virtual void ReadJson(const QJsonObject& object);
    virtual QJsonObject WriteJson() const;

    virtual ~Node() = default;
};

struct NodeF final : Node {
    finance::Roles roles {};

    void Reset() override;

    void ReadJson(const QJsonObject& object) override;
    QJsonObject WriteJson() const override;
};

struct NodeI final : Node {
    double unit_price {};
    double commission {};

    void Reset() override;

    void ReadJson(const QJsonObject& object) override;
    QJsonObject WriteJson() const override;
};

struct NodeT final : Node { };

struct NodeP final : Node {
    void Reset() override;

    void ReadJson(const QJsonObject& object) override;
    QJsonObject WriteJson() const override;
};

struct NodeO final : Node {
    QUuid employee_id {};
    QUuid partner_id {};

    // NOTE: settlement_id are managed by the server
    QUuid settlement_id {};

    QDateTime issued_time {};
    double count_total {};
    double measure_total {};
    double discount_total {};
    OrderStatus order_status {};

    void Reset() override;
    void InvertTotal() override;

    void ReadJson(const QJsonObject& object) override;
    QJsonObject WriteJson() const override;
};

struct PendingNodeUpdate {
    QTimer* timer {};
    Node* node {};
    QJsonObject changes {};
};

using NodeHash = QHash<QUuid, Node*>;
using CNodeHash = const QHash<QUuid, Node*>;
using NodeList = QList<Node*>;

using NodeListO = QList<NodeO*>;
