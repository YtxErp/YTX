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
#include "table/supportmodel.h"
#include "table/trans.h"

// support node signal station

class SupportSStation final : public QObject {
    Q_OBJECT

public:
    static SupportSStation& Instance();
    void RegisterModel(Section section, int node_id, const SupportModel* model);
    void DeregisterModel(Section section, int node_id);

signals:
    // send to SupportModel
    void SAppendSupportTrans(const TransShadow* trans_shadow);
    void SRemoveSupportTrans(int support_id, int trans_id);
    void SAppendMultiSupportTransFPTS(int new_support_id, const QList<int>& trans_id_list);

public slots:
    // receive from TableModel
    void RAppendSupportTrans(Section section, const TransShadow* trans_shadow);
    void RRemoveSupportTrans(Section section, int support_id, int trans_id);

    // receive from sqlite
    void RMoveMultiSupportTransFPTS(Section section, int new_support_id, const QList<int>& trans_id_list);

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
