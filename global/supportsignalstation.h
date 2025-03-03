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

#ifndef SUPPORTSIGNALSTATION_H
#define SUPPORTSIGNALSTATION_H

#include "component/enumclass.h"
#include "table/supportmodel.h"
#include "table/trans.h"

class SupportSignalStation final : public QObject {
    Q_OBJECT

public:
    static SupportSignalStation& Instance();
    void RegisterModel(Section section, int node_id, const SupportModel* model);
    void DeregisterModel(Section section, int node_id);

signals:
    // send to SupportModelSupport
    void SAppendSupportTrans(const TransShadow* trans_shadow);
    void SRemoveSupportTrans(int support_id, int trans_id);
    void SAppendMultiSupportTransFPTS(int new_support_id, const QList<int>& trans_id_list);

public slots:
    // receive from SupportModel
    void RAppendSupportTrans(Section section, const TransShadow* trans_shadow);
    void RRemoveSupportTrans(Section section, int support_id, int trans_id);

    // receive from sqlite
    void RMoveMultiSupportTransFPTS(Section section, int new_support_id, const QList<int>& trans_id_list);

private:
    SupportSignalStation() = default;
    ~SupportSignalStation() { };

    SupportSignalStation(const SupportSignalStation&) = delete;
    SupportSignalStation& operator=(const SupportSignalStation&) = delete;
    SupportSignalStation(SupportSignalStation&&) = delete;
    SupportSignalStation& operator=(SupportSignalStation&&) = delete;

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

#endif // SUPPORTSIGNALSTATION_H
