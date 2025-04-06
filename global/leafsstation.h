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

#ifndef LEAFSSTATION_H
#define LEAFSSTATION_H

#include "component/enumclass.h"
#include "table/model/transmodel.h"
#include "table/trans.h"

// leaf node signal station

class LeafSStation final : public QObject {
    Q_OBJECT

public:
    static LeafSStation& Instance();
    void RegisterModel(Section section, int node_id, const TransModel* model);
    void DeregisterModel(Section section, int node_id);

signals:
    // send to TableModel
    void SAppendOneTransL(const TransShadow* trans_shadow);
    void SRemoveOneTransL(int node_id, int trans_id);
    void SSyncBalance(int node_id, int trans_id);
    void SSyncRule(int node_id, bool rule);
    void SAppendMultiTrans(int node_id, const TransShadowList& trans_shadow_list);
    void SRemoveMultiTransL(int node_id, const QSet<int>& trans_id_set);
    void SAppendMultiTransL(int node_id, const QSet<int>& trans_id_set);

public slots:
    // receive from TableModel
    void RAppendOneTransL(Section section, const TransShadow* trans_shadow);
    void RRemoveOneTransL(Section section, int node_id, int trans_id);
    void RSyncBalance(Section section, int node_id, int trans_id);

    // receive from SqliteStakeholder

    // receive from TreeModel
    void RSyncRule(Section section, int node_id, bool rule);

    // receive from Sqlite
    void RRemoveMultiTransL(Section section, const QMultiHash<int, int>& leaf_trans);
    void RMoveMultiTransL(Section section, int old_node_id, int new_node_id, const QSet<int>& trans_id_set);
    void RAppendMultiTrans(Section section, int node_id, const TransShadowList& trans_shadow);

private:
    LeafSStation() = default;
    ~LeafSStation() { };

    LeafSStation(const LeafSStation&) = delete;
    LeafSStation& operator=(const LeafSStation&) = delete;
    LeafSStation(LeafSStation&&) = delete;
    LeafSStation& operator=(LeafSStation&&) = delete;

    const TransModel* FindModel(Section section, int node_id) const
    {
        auto it = model_hash_.constFind({ section, node_id });
        if (it == model_hash_.constEnd())
            return nullptr;

        return it.value();
    }

private:
    QHash<std::pair<Section, int>, const TransModel*> model_hash_ {};
};

#endif // LEAFSSTATION_H
