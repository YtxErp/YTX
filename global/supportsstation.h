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

#ifndef SUPPORTSSTATION_H
#define SUPPORTSSTATION_H

#include "component/enumclass.h"
#include "support/model/supportmodel.h"

// support node signal station

class SupportSStation final : public QObject {
    Q_OBJECT

public:
    static SupportSStation& Instance();
    void RegisterModel(Section section, int support_id, const SupportModel* model);
    void DeregisterModel(Section section, int support_id);

signals:
    // send to SupportModel
    void SAppendOneTransS(int support_id, int trans_id);
    void SRemoveOneTransS(int support_id, int trans_id);
    void SRemoveMultiTransS(int node_id, const QSet<int>& trans_id_set);
    void SAppendMultiTransS(int node_id, const QSet<int>& trans_id_set);

public slots:
    // receive from TableModel
    void RAppendOneTransS(Section section, int support_id, int trans_id);
    void RRemoveOneTransS(Section section, int support_id, int trans_id);

    // receive from Sqlite
    void RRemoveMultiTransS(Section section, const QMultiHash<int, int>& support_trans);
    void RMoveMultiTransS(Section section, int old_node_id, int new_node_id, const QSet<int>& trans_id_set);

private:
    SupportSStation() = default;
    ~SupportSStation() { };

    SupportSStation(const SupportSStation&) = delete;
    SupportSStation& operator=(const SupportSStation&) = delete;
    SupportSStation(SupportSStation&&) = delete;
    SupportSStation& operator=(SupportSStation&&) = delete;

    const SupportModel* FindModel(Section section, int node_id) const
    {
        auto it = model_hash_.constFind({ section, node_id });
        if (it == model_hash_.constEnd())
            return nullptr;

        return it.value();
    }

private:
    QHash<std::pair<Section, int>, const SupportModel*> model_hash_ {};
};

#endif // SUPPORTSSTATION_H
