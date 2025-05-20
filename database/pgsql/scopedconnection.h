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

#ifndef SCOPEDCONNECTION_H
#define SCOPEDCONNECTION_H

#include "global/pgconnection.h"

class ScopedConnection {
public:
    ScopedConnection()
        : db_ { PGConnection::Instance().GetConnection() }
    {
        if (db_.isValid()) {
            if (db_.isOpen()) {
                acquired_ = true;
            } else {
                PGConnection::Instance().ReturnConnection(db_);
                db_ = QSqlDatabase();
            }
        }
    }

    ~ScopedConnection()
    {
        if (acquired_) {
            PGConnection::Instance().ReturnConnection(db_);
        }
    }

    bool IsValid() const { return acquired_; }
    QSqlDatabase& Database() { return db_; }

    ScopedConnection(const ScopedConnection&) = delete;
    ScopedConnection& operator=(const ScopedConnection&) = delete;

    ScopedConnection(ScopedConnection&& other) noexcept
        : db_(other.db_)
        , acquired_(other.acquired_)
    {
        other.acquired_ = false;
    }

    ScopedConnection& operator=(ScopedConnection&& other) noexcept
    {
        if (this != &other) {
            if (acquired_) {
                PGConnection::Instance().ReturnConnection(db_);
            }

            db_ = other.db_;
            acquired_ = other.acquired_;
            other.acquired_ = false;
        }

        return *this;
    }

private:
    QSqlDatabase db_ {};
    bool acquired_ { false };
};

#endif // SCOPEDCONNECTION_H
