#include "transmodel.h"

#include <QtConcurrent>

#include "global/resourcepool.h"
#include "transmodelutils.h"

TransModel::TransModel(CTransModelArg& arg, QObject* parent)
    : QAbstractItemModel(parent)
    , sql_ { arg.sql }
    , info_ { arg.info }
    , node_rule_ { arg.rule }
    , node_id_ { arg.node_id }
    , section_ { arg.info.section }
{
}

TransModel::~TransModel() { ResourcePool<TransShadow>::Instance().Recycle(trans_shadow_list_); }

void TransModel::RSyncRule(int node_id, bool rule)
{
    assert(node_id_ == node_id && node_rule_ != rule && "node_id must match and node_rule must be different from rule");

    for (auto* trans_shadow : std::as_const(trans_shadow_list_))
        trans_shadow->subtotal = -trans_shadow->subtotal;

    node_rule_ = rule;
}

void TransModel::RAppendOneTransL(const TransShadow* trans_shadow)
{
    assert(node_id_ == *trans_shadow->rhs_node && "node_id_ must match *trans_shadow->rhs_node");

    auto* new_trans_shadow { ResourcePool<TransShadow>::Instance().Allocate() };
    new_trans_shadow->date_time = trans_shadow->date_time;
    new_trans_shadow->id = trans_shadow->id;
    new_trans_shadow->description = trans_shadow->description;
    new_trans_shadow->code = trans_shadow->code;
    new_trans_shadow->document = trans_shadow->document;
    new_trans_shadow->state = trans_shadow->state;
    new_trans_shadow->discount = trans_shadow->discount;
    new_trans_shadow->support_id = trans_shadow->support_id;

    new_trans_shadow->rhs_ratio = trans_shadow->lhs_ratio;
    new_trans_shadow->rhs_debit = trans_shadow->lhs_debit;
    new_trans_shadow->rhs_credit = trans_shadow->lhs_credit;
    new_trans_shadow->rhs_node = trans_shadow->lhs_node;

    new_trans_shadow->lhs_node = trans_shadow->rhs_node;
    new_trans_shadow->lhs_ratio = trans_shadow->rhs_ratio;
    new_trans_shadow->lhs_debit = trans_shadow->rhs_debit;
    new_trans_shadow->lhs_credit = trans_shadow->rhs_credit;

    auto row { trans_shadow_list_.size() };

    beginInsertRows(QModelIndex(), row, row);
    trans_shadow_list_.emplaceBack(new_trans_shadow);
    endInsertRows();

    const double previous_balance { row >= 1 ? trans_shadow_list_.at(row - 1)->subtotal : 0.0 };
    new_trans_shadow->subtotal = TransModelUtils::Balance(node_rule_, *new_trans_shadow->lhs_debit, *new_trans_shadow->lhs_credit) + previous_balance;
}

void TransModel::RRemoveOneTransL(int node_id, int trans_id)
{
    assert(node_id_ == node_id && "node_id must match");

    auto idx { GetIndex(trans_id) };
    if (!idx.isValid())
        return;

    int row { idx.row() };
    beginRemoveRows(QModelIndex(), row, row);
    ResourcePool<TransShadow>::Instance().Recycle(trans_shadow_list_.takeAt(row));
    endRemoveRows();

    TransModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, row, node_rule_);
}

void TransModel::RSyncBalance(int node_id, int trans_id)
{
    assert(node_id_ == node_id && "node_id must match");

    auto index { GetIndex(trans_id) };
    if (index.isValid())
        TransModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, index.row(), node_rule_);
}

bool TransModel::removeRows(int row, int /*count*/, const QModelIndex& parent)
{
    assert(row >= 0 && row <= rowCount(parent) - 1 && "Row must be in the valid range [0, rowCount(parent) - 1]");

    auto* trans_shadow { trans_shadow_list_.at(row) };
    const int rhs_node_id { *trans_shadow->rhs_node };

    beginRemoveRows(parent, row, row);
    trans_shadow_list_.removeAt(row);
    endRemoveRows();

    if (rhs_node_id != 0) {
        const double lhs_ratio { *trans_shadow->lhs_ratio };
        const double lhs_debit { *trans_shadow->lhs_debit };
        const double lhs_credit { *trans_shadow->lhs_credit };
        emit SSyncLeafValue(node_id_, -lhs_debit, -lhs_credit, -lhs_ratio * lhs_debit, -lhs_ratio * lhs_credit);

        const double rhs_ratio { *trans_shadow->rhs_ratio };
        const double rhs_debit { *trans_shadow->rhs_debit };
        const double rhs_credit { *trans_shadow->rhs_credit };
        emit SSyncLeafValue(*trans_shadow->rhs_node, -rhs_debit, -rhs_credit, -rhs_ratio * rhs_debit, -rhs_ratio * rhs_credit);

        const int trans_id { *trans_shadow->id };
        emit SRemoveOneTransL(section_, rhs_node_id, trans_id);
        TransModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, row, node_rule_);

        UpdateUnitCost(node_id_, *trans_shadow->rhs_node, -lhs_ratio);

        if (const int support_id = *trans_shadow->support_id; support_id != 0)
            emit SRemoveOneTransS(section_, support_id, *trans_shadow->id);

        sql_->RemoveTrans(trans_id);
    }

    ResourcePool<TransShadow>::Instance().Recycle(trans_shadow);
    return true;
}

void TransModel::UpdateAllState(Check state)
{
    auto UpdateState = [state](TransShadow* trans_shadow) {
        switch (state) {
        case Check::kAll:
            *trans_shadow->state = true;
            break;
        case Check::kNone:
            *trans_shadow->state = false;
            break;
        case Check::kReverse:
            *trans_shadow->state = !*trans_shadow->state;
            break;
        default:
            break;
        }
    };

    // 使用 QtConcurrent::map() 并行处理 trans_shadow_list_
    auto future { QtConcurrent::map(trans_shadow_list_, UpdateState) };

    // 使用 QFutureWatcher 监听并行任务的完成状态
    auto* watcher { new QFutureWatcher<void>(this) };

    // 连接信号槽，任务完成时刷新视图
    connect(watcher, &QFutureWatcher<void>::finished, this, [this, state, watcher]() {
        // 更新数据库
        sql_->WriteState(state);

        // 刷新视图
        const int column { std::to_underlying(TransEnum::kState) };
        emit dataChanged(index(0, column), index(rowCount() - 1, column));

        // 释放 QFutureWatcher
        watcher->deleteLater();
    });

    // 开始监听任务
    watcher->setFuture(future);
}

bool TransModel::UpdateDebit(TransShadow* trans_shadow, double value)
{
    const double lhs_debit { *trans_shadow->lhs_debit };
    if (std::abs(lhs_debit - value) < kTolerance)
        return false;

    const double lhs_credit { *trans_shadow->lhs_credit };
    const double lhs_ratio { *trans_shadow->lhs_ratio };

    const double abs { qAbs(value - lhs_credit) };
    *trans_shadow->lhs_debit = (value > lhs_credit) ? abs : 0;
    *trans_shadow->lhs_credit = (value <= lhs_credit) ? abs : 0;

    const double rhs_debit { *trans_shadow->rhs_debit };
    const double rhs_credit { *trans_shadow->rhs_credit };
    const double rhs_ratio { *trans_shadow->rhs_ratio };

    *trans_shadow->rhs_debit = (*trans_shadow->lhs_credit) * lhs_ratio / rhs_ratio;
    *trans_shadow->rhs_credit = (*trans_shadow->lhs_debit) * lhs_ratio / rhs_ratio;

    if (*trans_shadow->rhs_node == 0)
        return false;

    const double lhs_debit_delta { *trans_shadow->lhs_debit - lhs_debit };
    const double lhs_credit_delta { *trans_shadow->lhs_credit - lhs_credit };
    emit SSyncLeafValue(node_id_, lhs_debit_delta, lhs_credit_delta, lhs_debit_delta * lhs_ratio, lhs_credit_delta * lhs_ratio);

    const double rhs_debit_delta { *trans_shadow->rhs_debit - rhs_debit };
    const double rhs_credit_delta { *trans_shadow->rhs_credit - rhs_credit };
    emit SSyncLeafValue(*trans_shadow->rhs_node, rhs_debit_delta, rhs_credit_delta, rhs_debit_delta * rhs_ratio, rhs_credit_delta * rhs_ratio);

    return true;
}

bool TransModel::UpdateCredit(TransShadow* trans_shadow, double value)
{
    const double lhs_credit { *trans_shadow->lhs_credit };
    if (std::abs(lhs_credit - value) < kTolerance)
        return false;

    const double lhs_debit { *trans_shadow->lhs_debit };
    const double lhs_ratio { *trans_shadow->lhs_ratio };

    const double abs { qAbs(value - lhs_debit) };
    *trans_shadow->lhs_debit = (value > lhs_debit) ? 0 : abs;
    *trans_shadow->lhs_credit = (value <= lhs_debit) ? 0 : abs;

    const double rhs_debit { *trans_shadow->rhs_debit };
    const double rhs_credit { *trans_shadow->rhs_credit };
    const double rhs_ratio { *trans_shadow->rhs_ratio };

    *trans_shadow->rhs_debit = (*trans_shadow->lhs_credit) * lhs_ratio / rhs_ratio;
    *trans_shadow->rhs_credit = (*trans_shadow->lhs_debit) * lhs_ratio / rhs_ratio;

    if (*trans_shadow->rhs_node == 0)
        return false;

    const double lhs_debit_delta { *trans_shadow->lhs_debit - lhs_debit };
    const double lhs_credit_delta { *trans_shadow->lhs_credit - lhs_credit };
    emit SSyncLeafValue(node_id_, lhs_debit_delta, lhs_credit_delta, lhs_debit_delta * lhs_ratio, lhs_credit_delta * lhs_ratio);

    const double rhs_debit_delta { *trans_shadow->rhs_debit - rhs_debit };
    const double rhs_credit_delta { *trans_shadow->rhs_credit - rhs_credit };
    emit SSyncLeafValue(*trans_shadow->rhs_node, rhs_debit_delta, rhs_credit_delta, rhs_debit_delta * rhs_ratio, rhs_credit_delta * rhs_ratio);

    return true;
}

bool TransModel::UpdateRatio(TransShadow* trans_shadow, double value)
{
    const double lhs_ratio { *trans_shadow->lhs_ratio };

    if (std::abs(lhs_ratio - value) < kTolerance || value <= 0)
        return false;

    const double delta { value - lhs_ratio };
    const double proportion { value / *trans_shadow->lhs_ratio };

    *trans_shadow->lhs_ratio = value;

    const double rhs_debit { *trans_shadow->rhs_debit };
    const double rhs_credit { *trans_shadow->rhs_credit };
    const double rhs_ratio { *trans_shadow->rhs_ratio };

    *trans_shadow->rhs_debit *= proportion;
    *trans_shadow->rhs_credit *= proportion;

    if (*trans_shadow->rhs_node == 0)
        return false;

    emit SSyncLeafValue(node_id_, 0, 0, *trans_shadow->lhs_debit * delta, *trans_shadow->lhs_credit * delta);

    const double rhs_debit_delta { *trans_shadow->rhs_debit - rhs_debit };
    const double rhs_credit_delta { *trans_shadow->rhs_credit - rhs_credit };
    emit SSyncLeafValue(*trans_shadow->rhs_node, rhs_debit_delta, rhs_credit_delta, rhs_debit_delta * rhs_ratio, rhs_credit_delta * rhs_ratio);

    return true;
}

QModelIndex TransModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column);
}

QVariant TransModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return info_.trans_header.at(section);

    return QVariant();
}

int TransModel::GetNodeRow(int rhs_node_id) const
{
    int row { 0 };

    for (const auto* trans_shadow : trans_shadow_list_) {
        if (*trans_shadow->rhs_node == rhs_node_id) {
            return row;
        }
        ++row;
    }
    return -1;
}

QModelIndex TransModel::GetIndex(int trans_id) const
{
    int row { 0 };

    for (const auto* trans_shadow : trans_shadow_list_) {
        if (*trans_shadow->id == trans_id) {
            return index(row, 0);
        }
        ++row;
    }
    return QModelIndex();
}

QStringList* TransModel::GetDocumentPointer(const QModelIndex& index) const
{
    if (!index.isValid()) {
        qWarning() << "Invalid QModelIndex provided.";
        return nullptr;
    }

    auto* trans_shadow { trans_shadow_list_[index.row()] };

    if (!trans_shadow || !trans_shadow->document) {
        qWarning() << "Null pointer encountered in trans_list_ or document.";
        return nullptr;
    }

    return trans_shadow->document;
}

bool TransModel::insertRows(int row, int /*count*/, const QModelIndex& parent)
{
    assert(row >= 0 && row <= rowCount(parent) && "Row must be in the valid range [0, rowCount(parent)]");

    // just register trans_shadow in this function
    // while set rhs node in setData function, register trans to sql_'s trans_hash_
    auto* trans_shadow { sql_->AllocateTransShadow() };

    *trans_shadow->lhs_node = node_id_;
    IniRatio(trans_shadow);

    beginInsertRows(parent, row, row);
    trans_shadow_list_.emplaceBack(trans_shadow);
    endInsertRows();

    return true;
}

void TransModel::RRemoveMultiTransL(int node_id, const QSet<int>& trans_id_set)
{
    assert(node_id_ == node_id && "Node ID mismatch detected!");

    int min_row { std::numeric_limits<int>::max() };

    for (int i = trans_shadow_list_.size() - 1; i >= 0; --i) {
        const int trans_id { *trans_shadow_list_[i]->id };

        if (trans_id_set.contains(trans_id)) {
            min_row = i;

            beginRemoveRows(QModelIndex(), i, i);
            ResourcePool<TransShadow>::Instance().Recycle(trans_shadow_list_.takeAt(i));
            endRemoveRows();
        }
    }

    if (min_row != std::numeric_limits<int>::max())
        TransModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, min_row, node_rule_);
}

void TransModel::RAppendMultiTransL(int node_id, const QSet<int>& trans_id_set)
{
    assert(node_id_ == node_id && "Node ID mismatch detected!");

    auto row { trans_shadow_list_.size() };
    TransShadowList trans_shadow_list {};

    sql_->RetrieveTransRange(trans_shadow_list, node_id, trans_id_set);
    beginInsertRows(QModelIndex(), row, row + trans_shadow_list.size() - 1);
    trans_shadow_list_.append(trans_shadow_list);
    endInsertRows();

    TransModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, row, node_rule_);
}

void TransModel::RAppendMultiTrans(int node_id, const TransShadowList& trans_shadow_list)
{
    assert(node_id_ == node_id && "Node ID mismatch detected!");

    auto row { trans_shadow_list_.size() };
    beginInsertRows(QModelIndex(), row, row + trans_shadow_list.size() - 1);
    trans_shadow_list_.append(trans_shadow_list);
    endInsertRows();
}
