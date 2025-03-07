#include "global/leafsstation.h"

#include "table/model/transmodels.h"

LeafSStation& LeafSStation::Instance()
{
    static LeafSStation instance {};
    return instance;
}

void LeafSStation::RegisterModel(Section section, int node_id, const TransModel* model) { model_hash_.insert({ section, node_id }, model); }

void LeafSStation::DeregisterModel(Section section, int node_id) { model_hash_.remove({ section, node_id }); }

void LeafSStation::RAppendOneTrans(Section section, const TransShadow* trans_shadow)
{
    if (!trans_shadow)
        return;

    int rhs_node_id { *trans_shadow->rhs_node };
    const auto* model { FindModel(section, rhs_node_id) };
    if (!model)
        return;

    connect(this, &LeafSStation::SAppendOneTrans, model, &TransModel::RAppendOneTrans, Qt::SingleShotConnection);
    emit SAppendOneTrans(trans_shadow);
}

void LeafSStation::RRemoveOneTrans(Section section, int node_id, int trans_id)
{
    const auto* model { FindModel(section, node_id) };
    if (!model)
        return;

    connect(this, &LeafSStation::SRemoveOneTrans, model, &TransModel::RRemoveOneTrans, Qt::SingleShotConnection);
    emit SRemoveOneTrans(node_id, trans_id);
}

void LeafSStation::RUpdateBalance(Section section, int node_id, int trans_id)
{
    const auto* model { FindModel(section, node_id) };
    if (!model)
        return;

    connect(this, &LeafSStation::SUpdateBalance, model, &TransModel::RUpdateBalance, Qt::SingleShotConnection);
    emit SUpdateBalance(node_id, trans_id);
}

void LeafSStation::RAppendPrice(Section section, TransShadow* trans_shadow)
{
    if (!trans_shadow)
        return;

    int lhs_node { *trans_shadow->lhs_node };
    const auto* model { FindModel(section, lhs_node) };
    if (!model)
        return;

    const auto* cast_model { static_cast<const TransModelS*>(model) };
    connect(this, &LeafSStation::SAppendPrice, cast_model, &TransModelS::RAppendPrice, Qt::SingleShotConnection);
    emit SAppendPrice(trans_shadow);
}

void LeafSStation::RRule(Section section, int node_id, bool rule)
{
    const auto* model { FindModel(section, node_id) };
    if (!model)
        return;

    connect(this, &LeafSStation::SRule, model, &TransModel::RRule, Qt::SingleShotConnection);
    emit SRule(node_id, rule);
}
