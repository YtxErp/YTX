#include "global/leafsstation.h"

#include "table/model/transmodels.h"

LeafSStation& LeafSStation::Instance()
{
    static LeafSStation instance {};
    return instance;
}

void LeafSStation::RegisterModel(Section section, int node_id, const TransModel* model) { model_hash_.insert({ section, node_id }, model); }

void LeafSStation::DeregisterModel(Section section, int node_id) { model_hash_.remove({ section, node_id }); }

void LeafSStation::RAppendOneTransL(Section section, const TransShadow* trans_shadow)
{
    assert(trans_shadow && "trans_shadow must be non-null");

    int rhs_node_id { *trans_shadow->rhs_node };
    const auto* model { FindModel(section, rhs_node_id) };

    if (!model)
        return;

    connect(this, &LeafSStation::SAppendOneTransL, model, &TransModel::RAppendOneTransL, Qt::SingleShotConnection);
    emit SAppendOneTransL(trans_shadow);
}

void LeafSStation::RRemoveOneTransL(Section section, int node_id, int trans_id)
{
    const auto* model { FindModel(section, node_id) };
    if (!model)
        return;

    connect(this, &LeafSStation::SRemoveOneTransL, model, &TransModel::RRemoveOneTransL, Qt::SingleShotConnection);
    emit SRemoveOneTransL(node_id, trans_id);
}

void LeafSStation::RSyncBalance(Section section, int node_id, int trans_id)
{
    const auto* model { FindModel(section, node_id) };
    if (!model)
        return;

    connect(this, &LeafSStation::SSyncBalance, model, &TransModel::RSyncBalance, Qt::SingleShotConnection);
    emit SSyncBalance(node_id, trans_id);
}

void LeafSStation::RAppendMultiTrans(Section section, int node_id, const TransShadowList& trans_shadow_list)
{
    if (trans_shadow_list.isEmpty())
        return;

    const auto* model { FindModel(section, node_id) };
    if (!model)
        return;

    connect(this, &LeafSStation::SAppendMultiTrans, model, &TransModel::RAppendMultiTrans, Qt::SingleShotConnection);
    emit SAppendMultiTrans(node_id, trans_shadow_list);
}

void LeafSStation::RSyncRule(Section section, int node_id, bool rule)
{
    const auto* model { FindModel(section, node_id) };
    if (!model)
        return;

    connect(this, &LeafSStation::SSyncRule, model, &TransModel::RSyncRule, Qt::SingleShotConnection);
    emit SSyncRule(node_id, rule);
}

void LeafSStation::RRemoveMultiTransL(Section section, const QMultiHash<int, int>& leaf_trans)
{
    const auto keys { leaf_trans.uniqueKeys() };

    for (int node_id : keys) {
        const auto* model { FindModel(section, node_id) };
        if (!model)
            continue;

        const auto trans_id_list { leaf_trans.values(node_id) };
        const QSet<int> trans_id_set { trans_id_list.cbegin(), trans_id_list.cend() };

        connect(this, &LeafSStation::SRemoveMultiTransL, model, &TransModel::RRemoveMultiTransL, Qt::SingleShotConnection);
        emit SRemoveMultiTransL(node_id, trans_id_set);
    }
}

void LeafSStation::RMoveMultiTransL(Section section, int old_node_id, int new_node_id, const QSet<int>& trans_id_set)
{
    const auto* old_model { FindModel(section, old_node_id) };
    if (old_model) {
        connect(this, &LeafSStation::SRemoveMultiTransL, old_model, &TransModel::RRemoveMultiTransL, Qt::SingleShotConnection);
        emit SRemoveMultiTransL(old_node_id, trans_id_set);
    }

    const auto* new_model { FindModel(section, new_node_id) };
    if (new_model) {
        connect(this, &LeafSStation::SAppendMultiTransL, new_model, &TransModel::RAppendMultiTransL, Qt::SingleShotConnection);
        emit SAppendMultiTransL(new_node_id, trans_id_set);
    }
}
