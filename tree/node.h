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

#ifndef NODE_H
#define NODE_H

#include <QList>
#include <QString>

struct Node {
    QString name {};
    int id {};
    QString code {};
    QString description {};
    QString note {};
    QString date_time {};
    QString color {};
    QStringList document {};
    bool direction_rule { false };
    int node_type {};
    int unit {};

    double first {};
    double second {};
    double discount {};
    bool is_finished {};

    // order and stakeholder
    int employee {};
    // order
    int party {};

    double final_total {};
    double initial_total {};

    Node* parent {};
    QList<Node*> children {};

    void Reset()
    {
        name.clear();
        id = 0;
        code.clear();
        party = 0;
        employee = 0;
        second = 0.0;
        discount = 0.0;
        is_finished = false;
        first = 0.0;
        date_time.clear();
        description.clear();
        note.clear();
        color.clear();
        document.clear();
        direction_rule = false;
        node_type = 0;
        unit = 0;
        final_total = 0.0;
        initial_total = 0.0;
        parent = nullptr;
        children.clear();
    }
};

using NodeHash = QHash<int, Node*>;
using CNodeHash = const QHash<int, Node*>;
using NodeList = QList<Node*>;

#endif // NODE_H
