#include "tablemodel.h"

#include <QtConcurrent>

#include "global/resourcepool.h"
#include "tablemodelutils.h"

TableModel::TableModel(Sqlite* sql, bool rule, int node_id, CInfo& info, QObject* parent)
    : QAbstractItemModel(parent)
    , sql_ { sql }
    , rule_ { rule }
    , info_ { info }
    , node_id_ { node_id }
{
}

TableModel::~TableModel() { ResourcePool<TransShadow>::Instance().Recycle(trans_shadow_list_); }

void TableModel::RRemoveMultiTrans(const QMultiHash<int, int>& node_trans)
{
    if (!node_trans.contains(node_id_))
        return;

    RemoveMultiTrans(node_trans.values(node_id_));
}

void TableModel::RMoveMultiTrans(int old_node_id, int new_node_id, const QList<int>& trans_id_list)
{
    if (node_id_ == old_node_id)
        RemoveMultiTrans(trans_id_list);

    if (node_id_ == new_node_id)
        AppendMultiTrans(node_id_, trans_id_list);
}

void TableModel::RRule(int node_id, bool rule)
{
    if (node_id_ != node_id || rule_ == rule)
        return;

    for (auto* trans_shadow : trans_shadow_list_)
        trans_shadow->subtotal = -trans_shadow->subtotal;

    rule_ = rule;
}

void TableModel::RAppendOneTrans(const TransShadow* trans_shadow)
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
    new_trans_shadow->unit_price = trans_shadow->unit_price;
    new_trans_shadow->discount_price = trans_shadow->discount_price;
    new_trans_shadow->settled = trans_shadow->settled;
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
    new_trans_shadow->subtotal = TableModelUtils::Balance(rule_, *new_trans_shadow->lhs_debit, *new_trans_shadow->lhs_credit) + previous_balance;
}

void TableModel::RRemoveOneTrans(int node_id, int trans_id)
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

    TableModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, row, rule_);
}

void TableModel::RUpdateBalance(int node_id, int trans_id)
{
    if (node_id_ != node_id)
        return;

    auto index { GetIndex(trans_id) };
    if (index.isValid())
        TableModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, index.row(), rule_);
}

bool TableModel::removeRows(int row, int /*count*/, const QModelIndex& parent)
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
        TableModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, row, rule_);

        if (int support_id = *trans_shadow->support_id; support_id != 0)
            emit SRemoveSupportTrans(info_.section, support_id, *trans_shadow->id);

        sql_->RemoveTrans(trans_id);
    }

    ResourcePool<TransShadow>::Instance().Recycle(trans_shadow);
    return true;
}

void TableModel::UpdateAllState(Check state)
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
        sql_->UpdateState(state);

        // 刷新视图
        int column { std::to_underlying(TableEnum::kState) };
        emit dataChanged(index(0, column), index(rowCount() - 1, column));

        // 释放 QFutureWatcher
        watcher->deleteLater();
    });

    // 开始监听任务
    watcher->setFuture(future);
}

bool TableModel::UpdateDebit(TransShadow* trans_shadow, double value)
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

    double lhs_debit_diff { *trans_shadow->lhs_debit - lhs_debit };
    double lhs_credit_diff { *trans_shadow->lhs_credit - lhs_credit };
    emit SUpdateLeafValue(node_id_, lhs_debit_diff, lhs_credit_diff, lhs_debit_diff * lhs_ratio, lhs_credit_diff * lhs_ratio);

    double rhs_debit_diff { *trans_shadow->rhs_debit - rhs_debit };
    double rhs_credit_diff { *trans_shadow->rhs_credit - rhs_credit };
    emit SUpdateLeafValue(*trans_shadow->rhs_node, rhs_debit_diff, rhs_credit_diff, rhs_debit_diff * rhs_ratio, rhs_credit_diff * rhs_ratio);

    return true;
}

bool TableModel::UpdateCredit(TransShadow* trans_shadow, double value)
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

    double lhs_debit_diff { *trans_shadow->lhs_debit - lhs_debit };
    double lhs_credit_diff { *trans_shadow->lhs_credit - lhs_credit };
    emit SUpdateLeafValue(node_id_, lhs_debit_diff, lhs_credit_diff, lhs_debit_diff * lhs_ratio, lhs_credit_diff * lhs_ratio);

    double rhs_debit_diff { *trans_shadow->rhs_debit - rhs_debit };
    double rhs_credit_diff { *trans_shadow->rhs_credit - rhs_credit };
    emit SUpdateLeafValue(*trans_shadow->rhs_node, rhs_debit_diff, rhs_credit_diff, rhs_debit_diff * rhs_ratio, rhs_credit_diff * rhs_ratio);

    return true;
}

bool TableModel::UpdateRatio(TransShadow* trans_shadow, double value)
{
    double lhs_ratio { *trans_shadow->lhs_ratio };

    if (std::abs(lhs_ratio - value) < kTolerance || value <= 0)
        return false;

    double diff { value - lhs_ratio };
    double proportion { value / *trans_shadow->lhs_ratio };

    *trans_shadow->lhs_ratio = value;

    double rhs_debit { *trans_shadow->rhs_debit };
    double rhs_credit { *trans_shadow->rhs_credit };
    double rhs_ratio { *trans_shadow->rhs_ratio };

    *trans_shadow->rhs_debit *= proportion;
    *trans_shadow->rhs_credit *= proportion;

    if (*trans_shadow->rhs_node == 0)
        return false;

    emit SUpdateLeafValue(node_id_, 0, 0, *trans_shadow->lhs_debit * diff, *trans_shadow->lhs_credit * diff);

    double rhs_debit_diff { *trans_shadow->rhs_debit - rhs_debit };
    double rhs_credit_diff { *trans_shadow->rhs_credit - rhs_credit };
    emit SUpdateLeafValue(*trans_shadow->rhs_node, rhs_debit_diff, rhs_credit_diff, rhs_debit_diff * rhs_ratio, rhs_credit_diff * rhs_ratio);

    return true;
}

QModelIndex TableModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex TableModel::parent(const QModelIndex& /*index*/) const { return QModelIndex(); }

int TableModel::rowCount(const QModelIndex& /*parent*/) const { return trans_shadow_list_.size(); }

int TableModel::columnCount(const QModelIndex& /*parent*/) const { return info_.table_header.size(); }

QVariant TableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return info_.table_header.at(section);

    return QVariant();
}

int TableModel::GetNodeRow(int rhs_node_id) const
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

QModelIndex TableModel::GetIndex(int trans_id) const
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

QStringList* TableModel::GetDocumentPointer(const QModelIndex& index) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= trans_shadow_list_.size()) {
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

bool TableModel::insertRows(int row, int /*count*/, const QModelIndex& parent)
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

bool TableModel::RemoveMultiTrans(const QList<int>& trans_id_list)
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
        TableModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, min_row, rule_);

    return true;
}

bool TableModel::AppendMultiTrans(int node_id, const QList<int>& trans_id_list)
{
    auto row { trans_shadow_list_.size() };
    TransShadowList trans_shadow_list {};

    sql_->ReadTransRange(trans_shadow_list, node_id, trans_id_list);
    beginInsertRows(QModelIndex(), row, row + trans_shadow_list.size() - 1);
    trans_shadow_list_.append(trans_shadow_list);
    endInsertRows();

    TableModelUtils::AccumulateSubtotal(mutex_, trans_shadow_list_, row, rule_);

    return true;
}
