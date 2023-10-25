#include "supportsstation.h"

SupportSStation& SupportSStation::Instance()
{
    static SupportSStation instance {};
    return instance;
}

void SupportSStation::RegisterModel(Section section, int node_id, const SupportModel* model) { model_hash_.insert({ section, node_id }, model); }

void SupportSStation::DeregisterModel(Section section, int node_id) { model_hash_.remove({ section, node_id }); }

void SupportSStation::RAppendSupportTrans(Section section, const TransShadow* trans_shadow)
{
    if (!trans_shadow)
        return;

    const auto* model { FindModel(section, *trans_shadow->support_id) };
    if (!model)
        return;

    connect(this, &SupportSStation::SAppendSupportTrans, model, &SupportModel::RAppendSupportTrans, Qt::SingleShotConnection);
    emit SAppendSupportTrans(trans_shadow);
}

void SupportSStation::RRemoveSupportTrans(Section section, int support_id, int trans_id)
{
    const auto* model { FindModel(section, support_id) };
    if (!model)
        return;

    connect(this, &SupportSStation::SRemoveSupportTrans, model, &SupportModel::RRemoveSupportTrans, Qt::SingleShotConnection);
    emit SRemoveSupportTrans(support_id, trans_id);
}

void SupportSStation::RMoveMultiSupportTransFPTS(Section section, int new_support_id, const QList<int>& trans_id_list)
{
    const auto* model { FindModel(section, new_support_id) };
    if (!model)
        return;

    connect(this, &SupportSStation::SAppendMultiSupportTransFPTS, model, &SupportModel::RAppendMultiSupportTrans, Qt::SingleShotConnection);
    emit SAppendMultiSupportTransFPTS(new_support_id, trans_id_list);
}
