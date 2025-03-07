#include "transmodel.h"

#include <QtConcurrent>

#include "global/resourcepool.h"
#include "transmodelutils.h"

TransModel::TransModel(Sqlite* sql, bool rule, int node_id, CInfo& info, QObject* parent)
    : QAbstractItemModel(parent)
    , sql_ { sql }
    , rule_ { rule }
    , info_ { info }
    , node_id_ { node_id }
{
}

TransModel::~TransModel() { ResourcePool<TransShadow>::Instance().Recycle(trans_shadow_list_); }

void TransModel::RRemoveMultiTrans(const QMultiHash<int, int>& node_trans)
{
    if (!node_trans.contains(node_id_))
        return;

    RemoveMultiTrans(node_trans.values(node_id_));
}

void TransModel::RMoveMultiTrans(int old_node_id, int new_node_id, const QList<int>& trans_id_list)
{
    if (node_id_ == old_node_id)
        RemoveMultiTrans(trans_id_list);

    if (node_id_ == new_node_id)
        AppendMultiTrans(node_id_, trans_id_list);
}

void TransModel::RRule(int node_id, bool rule)
{
    if (node_id_ != node_id || rule_ == rule)
        return;

    for (auto* trans_shadow : std::as_const(trans_shadow_list_))
        trans_shadow->subtotal = -trans_shadow->subtotal;

    rule_ = rule;
}

void TransModel::RAppendOneTrans(const TransShadow* trans_shadow)
{
    if (node_id_ != *trans_shadow->rhs_node)
        return;

    auto* new_trans_shadow { ResourcePool<TransShadow>::Instance().Allocate() };
    new_trans_shadow->date_time = trans_shadow->date_time;
    new_trans_shadow->id = trans_shadow->id;
    new_trans_shadow->description = trans_shadow->description;
    new_trans_shadow->code = trans_shadow->code;
    new_trans_shadow->document = trans_shadow->document;
    new_trans_shadow->state = trans_shadow->state;
    new_trans_shadow->lhs_ratio = trans_shadow->lhs_ratio;
    new_trans_shadow->rhs_ratio = trans_shadow->rhs_ratio;
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

    double previous_balance { row >= 1 ? trans_shadow_list_.at(row - 1)->subtotal : 0.0 };
    new_trans_shadow->subtotal = TransModelUtils::Balance(rule_, *new_trans_shadow->lhs_debit, *new_trans_shadow->lhs_credit) + previous_balance;
}

void TransModel::RRemoveOneTrans(int node_id, int trans_id)
{
    if (node_id_ != node_id)
        return;

    auto idx { GetIndex(trans_id) };
    if (!idx.isValid())
        return;

    int row { idx.row() };
    beginRemoveRows(QModelIndex(), row, row);
    ResourcePool<TransShadow>::Instance().Recycle(trans_shadow_list_.takeAt(row));
    endRemoveRows();

    TransModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, row, rule_);
}

void TransModel::RUpdateBalance(int node_id, int trans_id)
{
    if (node_id_ != node_id)
        return;

    auto index { GetIndex(trans_id) };
    if (index.isValid())
        TransModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, index.row(), rule_);
}

bool TransModel::removeRows(int row, int /*count*/, const QModelIndex& parent)
{
    if (row <= -1)
        return false;

    auto* trans_shadow { trans_shadow_list_.at(row) };
    int rhs_node_id { *trans_shadow->rhs_node };

    beginRemoveRows(parent, row, row);
    trans_shadow_list_.removeAt(row);
    endRemoveRows();

    if (rhs_node_id != 0) {
        auto ratio { *trans_shadow->lhs_ratio };
        auto debit { *trans_shadow->lhs_debit };
        auto credit { *trans_shadow->lhs_credit };
        emit SUpdateLeafValue(node_id_, -debit, -credit, -ratio * debit, -ratio * credit);

        ratio = *trans_shadow->rhs_ratio;
        debit = *trans_shadow->rhs_debit;
        credit = *trans_shadow->rhs_credit;
        emit SUpdateLeafValue(*trans_shadow->rhs_node, -debit, -credit, -ratio * debit, -ratio * credit);

        int trans_id { *trans_shadow->id };
        emit SRemoveOneTrans(info_.section, rhs_node_id, trans_id);
        TransModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, row, rule_);

        if (int support_id = *trans_shadow->support_id; support_id != 0)
            emit SRemoveSupportTrans(info_.section, support_id, *trans_shadow->id);

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
        int column { std::to_underlying(TransEnum::kState) };
        emit dataChanged(index(0, column), index(rowCount() - 1, column));

        // 释放 QFutureWatcher
        watcher->deleteLater();
    });

    // 开始监听任务
    watcher->setFuture(future);
}

bool TransModel::UpdateDebit(TransShadow* trans_shadow, double value)
{
    double lhs_debit { *trans_shadow->lhs_debit };
    if (std::abs(lhs_debit - value) < kTolerance)
        return false;

    double lhs_credit { *trans_shadow->lhs_credit };
    double lhs_ratio { *trans_shadow->lhs_ratio };

    double abs { qAbs(value - lhs_credit) };
    *trans_shadow->lhs_debit = (value > lhs_credit) ? abs : 0;
    *trans_shadow->lhs_credit = (value <= lhs_credit) ? abs : 0;

    double rhs_debit { *trans_shadow->rhs_debit };
    double rhs_credit { *trans_shadow->rhs_credit };
    double rhs_ratio { *trans_shadow->rhs_ratio };

    *trans_shadow->rhs_debit = (*trans_shadow->lhs_credit) * lhs_ratio / rhs_ratio;
    *trans_shadow->rhs_credit = (*trans_shadow->lhs_debit) * lhs_ratio / rhs_ratio;

    if (*trans_shadow->rhs_node == 0)
        return false;

    double lhs_debit_delta { *trans_shadow->lhs_debit - lhs_debit };
    double lhs_credit_delta { *trans_shadow->lhs_credit - lhs_credit };
    emit SUpdateLeafValue(node_id_, lhs_debit_delta, lhs_credit_delta, lhs_debit_delta * lhs_ratio, lhs_credit_delta * lhs_ratio);

    double rhs_debit_delta { *trans_shadow->rhs_debit - rhs_debit };
    double rhs_credit_delta { *trans_shadow->rhs_credit - rhs_credit };
    emit SUpdateLeafValue(*trans_shadow->rhs_node, rhs_debit_delta, rhs_credit_delta, rhs_debit_delta * rhs_ratio, rhs_credit_delta * rhs_ratio);

    return true;
}

bool TransModel::UpdateCredit(TransShadow* trans_shadow, double value)
{
    double lhs_credit { *trans_shadow->lhs_credit };
    if (std::abs(lhs_credit - value) < kTolerance)
        return false;

    double lhs_debit { *trans_shadow->lhs_debit };
    double lhs_ratio { *trans_shadow->lhs_ratio };

    double abs { qAbs(value - lhs_debit) };
    *trans_shadow->lhs_debit = (value > lhs_debit) ? 0 : abs;
    *trans_shadow->lhs_credit = (value <= lhs_debit) ? 0 : abs;

    double rhs_debit { *trans_shadow->rhs_debit };
    double rhs_credit { *trans_shadow->rhs_credit };
    double rhs_ratio { *trans_shadow->rhs_ratio };

    *trans_shadow->rhs_debit = (*trans_shadow->lhs_credit) * lhs_ratio / rhs_ratio;
    *trans_shadow->rhs_credit = (*trans_shadow->lhs_debit) * lhs_ratio / rhs_ratio;

    if (*trans_shadow->rhs_node == 0)
        return false;

    double lhs_debit_delta { *trans_shadow->lhs_debit - lhs_debit };
    double lhs_credit_delta { *trans_shadow->lhs_credit - lhs_credit };
    emit SUpdateLeafValue(node_id_, lhs_debit_delta, lhs_credit_delta, lhs_debit_delta * lhs_ratio, lhs_credit_delta * lhs_ratio);

    double rhs_debit_delta { *trans_shadow->rhs_debit - rhs_debit };
    double rhs_credit_delta { *trans_shadow->rhs_credit - rhs_credit };
    emit SUpdateLeafValue(*trans_shadow->rhs_node, rhs_debit_delta, rhs_credit_delta, rhs_debit_delta * rhs_ratio, rhs_credit_delta * rhs_ratio);

    return true;
}

bool TransModel::UpdateRatio(TransShadow* trans_shadow, double value)
{
    double lhs_ratio { *trans_shadow->lhs_ratio };

    if (std::abs(lhs_ratio - value) < kTolerance || value <= 0)
        return false;

    double delta { value - lhs_ratio };
    double proportion { value / *trans_shadow->lhs_ratio };

    *trans_shadow->lhs_ratio = value;

    double rhs_debit { *trans_shadow->rhs_debit };
    double rhs_credit { *trans_shadow->rhs_credit };
    double rhs_ratio { *trans_shadow->rhs_ratio };

    *trans_shadow->rhs_debit *= proportion;
    *trans_shadow->rhs_credit *= proportion;

    if (*trans_shadow->rhs_node == 0)
        return false;

    emit SUpdateLeafValue(node_id_, 0, 0, *trans_shadow->lhs_debit * delta, *trans_shadow->lhs_credit * delta);

    double rhs_debit_delta { *trans_shadow->rhs_debit - rhs_debit };
    double rhs_credit_delta { *trans_shadow->rhs_credit - rhs_credit };
    emit SUpdateLeafValue(*trans_shadow->rhs_node, rhs_debit_delta, rhs_credit_delta, rhs_debit_delta * rhs_ratio, rhs_credit_delta * rhs_ratio);

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
    // just register trans_shadow in this function
    // while set rhs node in setData function, register trans to sql_'s trans_hash_
    auto* trans_shadow { sql_->AllocateTransShadow() };

    *trans_shadow->lhs_node = node_id_;

    beginInsertRows(parent, row, row);
    trans_shadow_list_.emplaceBack(trans_shadow);
    endInsertRows();

    return true;
}

bool TransModel::RemoveMultiTrans(const QList<int>& trans_id_list)
{
    if (trans_id_list.isEmpty())
        return false;

    int min_row { -1 };
    int trans_id {};

    for (int i = trans_shadow_list_.size() - 1; i >= 0; --i) {
        trans_id = *trans_shadow_list_.at(i)->id;

        if (trans_id_list.contains(trans_id)) {
            if (min_row == -1 || i < min_row)
                min_row = i;

            beginRemoveRows(QModelIndex(), i, i);
            ResourcePool<TransShadow>::Instance().Recycle(trans_shadow_list_.takeAt(i));
            endRemoveRows();
        }
    }

    if (min_row != -1)
        TransModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, min_row, rule_);

    return true;
}

bool TransModel::AppendMultiTrans(int node_id, const QList<int>& trans_id_list)
{
    auto row { trans_shadow_list_.size() };
    TransShadowList trans_shadow_list {};

    sql_->ReadTransRange(trans_shadow_list, node_id, trans_id_list);
    beginInsertRows(QModelIndex(), row, row + trans_shadow_list.size() - 1);
    trans_shadow_list_.append(trans_shadow_list);
    endInsertRows();

    TransModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, row, rule_);

    return true;
}
