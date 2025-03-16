#include "supportsstation.h"

SupportSStation& SupportSStation::Instance()
{
    static SupportSStation instance {};
    return instance;
}

void SupportSStation::RegisterModel(Section section, int support_id, const SupportModel* model) { model_hash_.insert({ section, support_id }, model); }

void SupportSStation::DeregisterModel(Section section, int support_id) { model_hash_.remove({ section, support_id }); }

void SupportSStation::RAppendOneTransS(Section section, int support_id, int trans_id)
{
    const auto* model { FindModel(section, support_id) };
    if (!model)
        return;

    connect(this, &SupportSStation::SAppendOneTransS, model, &SupportModel::RAppendOneTransS, Qt::SingleShotConnection);
    emit SAppendOneTransS(support_id, trans_id);
}

void SupportSStation::RRemoveOneTransS(Section section, int support_id, int trans_id)
{
    const auto* model { FindModel(section, support_id) };
    if (!model)
        return;

    connect(this, &SupportSStation::SRemoveOneTransS, model, &SupportModel::RRemoveOneTransS, Qt::SingleShotConnection);
    emit SRemoveOneTransS(support_id, trans_id);
}

void SupportSStation::RRemoveMultiTransS(Section section, const QMultiHash<int, int>& support_trans)
{
    const auto keys { support_trans.uniqueKeys() };

    for (int support_id : keys) {
        const auto* model { FindModel(section, support_id) };
        if (!model)
            continue;

        const auto trans_id_list { support_trans.values(support_id) };
        const QSet<int> trans_id_set { trans_id_list.cbegin(), trans_id_list.cend() };

        connect(this, &SupportSStation::SRemoveMultiTransS, model, &SupportModel::RRemoveMultiTransS, Qt::SingleShotConnection);
        emit SRemoveMultiTransS(support_id, trans_id_set);
    }
}

void SupportSStation::RMoveMultiTransS(Section section, int /*old_node_id*/, int new_node_id, const QSet<int>& trans_id_set)
{
    const auto* model { FindModel(section, new_node_id) };
    if (model) {
        connect(this, &SupportSStation::SAppendMultiTransS, model, &SupportModel::RAppendMultiTransS, Qt::SingleShotConnection);
        emit SAppendMultiTransS(new_node_id, trans_id_set);
    }
}
