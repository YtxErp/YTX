#include "supportsignalstation.h"

SupportSignalStation& SupportSignalStation::Instance()
{
    static SupportSignalStation instance {};
    return instance;
}

void SupportSignalStation::RegisterModel(Section section, int node_id, const SupportModel* model) { model_hash_.insert({ section, node_id }, model); }

void SupportSignalStation::DeregisterModel(Section section, int node_id) { model_hash_.remove({ section, node_id }); }

void SupportSignalStation::RAppendSupportTrans(Section section, const TransShadow* trans_shadow)
{
    if (!trans_shadow)
        return;

    const auto* model { FindModel(section, *trans_shadow->support_id) };
    if (!model)
        return;

    connect(this, &SupportSignalStation::SAppendSupportTrans, model, &SupportModel::RAppendSupportTrans, Qt::SingleShotConnection);
    emit SAppendSupportTrans(trans_shadow);
}

void SupportSignalStation::RRemoveSupportTrans(Section section, int support_id, int trans_id)
{
    const auto* model { FindModel(section, support_id) };
    if (!model)
        return;

    connect(this, &SupportSignalStation::SRemoveSupportTrans, model, &SupportModel::RRemoveSupportTrans, Qt::SingleShotConnection);
    emit SRemoveSupportTrans(support_id, trans_id);
}

void SupportSignalStation::RMoveMultiSupportTransFPTS(Section section, int new_support_id, const QList<int>& trans_id_list)
{
    const auto* model { FindModel(section, new_support_id) };
    if (!model)
        return;

    connect(this, &SupportSignalStation::SAppendMultiSupportTransFPTS, model, &SupportModel::RAppendMultiSupportTransFPTS, Qt::SingleShotConnection);
    emit SAppendMultiSupportTransFPTS(new_support_id, trans_id_list);
}
