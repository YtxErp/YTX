#include "global/leafsstation.h"

#include "table/model/tablemodelstakeholder.h"
#include "table/model/tablemodelsupport.h"

LeafSStation& LeafSStation::Instance()
{
    static LeafSStation instance {};
    return instance;
}

void LeafSStation::RegisterModel(Section section, int node_id, const TableModel* model)
{
    if (!model_hash_.contains(section))
        model_hash_[section] = QHash<int, const TableModel*>();

    model_hash_[section].insert(node_id, model);
}

void LeafSStation::DeregisterModel(Section section, int node_id) { model_hash_[section].remove(node_id); }

void LeafSStation::RAppendOneTrans(Section section, const TransShadow* trans_shadow)
{
    if (!trans_shadow)
        return;

    int rhs_node_id { *trans_shadow->rhs_node };
    const auto* model { FindModel(section, rhs_node_id) };
    if (!model)
        return;

    connect(this, &LeafSStation::SAppendOneTrans, model, &TableModel::RAppendOneTrans, Qt::SingleShotConnection);
    emit SAppendOneTrans(trans_shadow);
}

void LeafSStation::RRemoveOneTrans(Section section, int node_id, int trans_id)
{
    const auto* model { FindModel(section, node_id) };
    if (!model)
        return;

    connect(this, &LeafSStation::SRemoveOneTrans, model, &TableModel::RRemoveOneTrans, Qt::SingleShotConnection);
    emit SRemoveOneTrans(node_id, trans_id);
}

void LeafSStation::RUpdateBalance(Section section, int node_id, int trans_id)
{
    const auto* model { FindModel(section, node_id) };
    if (!model)
        return;

    connect(this, &LeafSStation::SUpdateBalance, model, &TableModel::RUpdateBalance, Qt::SingleShotConnection);
    emit SUpdateBalance(node_id, trans_id);
}

void LeafSStation::RAppendSupportTrans(Section section, const TransShadow* trans_shadow)
{
    if (!trans_shadow)
        return;

    const auto* model { FindModel(section, *trans_shadow->support_id) };
    if (!model)
        return;

    const auto* cast_model { static_cast<const TableModelSupport*>(model) };
    connect(this, &LeafSStation::SAppendSupportTrans, cast_model, &TableModelSupport::RAppendSupportTrans, Qt::SingleShotConnection);
    emit SAppendSupportTrans(trans_shadow);
}

void LeafSStation::RRemoveSupportTrans(Section section, int support_id, int trans_id)
{
    const auto* model { FindModel(section, support_id) };
    if (!model)
        return;

    const auto* cast_model { static_cast<const TableModelSupport*>(model) };
    connect(this, &LeafSStation::SRemoveSupportTrans, cast_model, &TableModelSupport::RRemoveSupportTrans, Qt::SingleShotConnection);
    emit SRemoveSupportTrans(support_id, trans_id);
}

void LeafSStation::RAppendPrice(Section section, TransShadow* trans_shadow)
{
    if (!trans_shadow)
        return;

    int lhs_node { *trans_shadow->lhs_node };
    const auto* model { FindModel(section, lhs_node) };
    if (!model)
        return;

    const auto* cast_model { static_cast<const TableModelStakeholder*>(model) };
    connect(this, &LeafSStation::SAppendPrice, cast_model, &TableModelStakeholder::RAppendPrice, Qt::SingleShotConnection);
    emit SAppendPrice(trans_shadow);
}

void LeafSStation::RRule(Section section, int node_id, bool rule)
{
    const auto* model { FindModel(section, node_id) };
    if (!model)
        return;

    connect(this, &LeafSStation::SRule, model, &TableModel::RRule, Qt::SingleShotConnection);
    emit SRule(node_id, rule);
}

void LeafSStation::RMoveMultiSupportTransFPTS(Section section, int new_support_id, const QList<int>& trans_id_list)
{
    const auto* model { FindModel(section, new_support_id) };
    if (!model)
        return;

    const auto* cast_model { static_cast<const TableModelSupport*>(model) };
    connect(this, &LeafSStation::SAppendMultiSupportTransFPTS, cast_model, &TableModelSupport::RAppendMultiSupportTransFPTS, Qt::SingleShotConnection);
    emit SAppendMultiSupportTransFPTS(new_support_id, trans_id_list);
}
