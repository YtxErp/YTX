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
#include <QUuid>

namespace audit {

// Mirrors the server-side `audit` table exactly.
// Field order follows the same memory-layout grouping as the schema:
//   1. UUIDs (16 bytes each)
//   2. Timestamps (8 bytes)
//   3. Integers (4 bytes)
//   4. Variable-length (TEXT / JSONB)
struct Row {
    // -- 1. UUIDs ---------------------------------------------------------------
    QUuid id {}; // PRIMARY KEY — generated client-side before shipping
    QUuid target_id {}; // The entity being audited
    QString username {}; // Who triggered the action
    QUuid lhs_node {}; // Default: null UUID (00000000-…)
    QUuid rhs_node {}; // Default: null UUID (00000000-…)

    // -- 2. Timestamp -----------------------------------------------------------
    QDateTime created_time {}; // UTC — maps to TIMESTAMPTZ

    // -- 3. Integers ------------------------------------------------------------
    int target_operation {}; // Workspace key
    int target_type {}; // Discriminator for the audited entity type
    int level {}; // Default: 0
    int target_field {};

    // -- 4. Variable-length -----------------------------------------------------
    QString target_code {}; // Default: ""
    QJsonValue before {}; // State before the action — maps to JSONB
    QJsonValue after {}; // State after the action  — maps to JSONB

    void Reset();
    void ReadJson(const QJsonObject& object);
};
}
