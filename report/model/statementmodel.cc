#include "statementmodel.h"

#include "component/enumclass.h"
#include "global/resourcepool.h"

StatementModel::StatementModel(Sql* sql, CInfo& info, QObject* parent)
    : QAbstractItemModel { parent }
    , sql_ { qobject_cast<SqlO*>(sql) }
    , info_ { info }
{
}

StatementModel::~StatementModel() { ResourcePool<Trans>::Instance().Recycle(trans_list_); }

QModelIndex StatementModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex StatementModel::parent(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

int StatementModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return trans_list_.size();
}

int StatementModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return info_.statement_header.size();
}

QVariant StatementModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    const StatementEnum kColumn { index.column() };
    auto* trans { trans_list_.at(index.row()) };

    switch (kColumn) {
    case StatementEnum::kParty:
        return trans->id;
    case StatementEnum::kPBalance:
        return trans->lhs_ratio == 0 ? QVariant() : trans->lhs_ratio;
    case StatementEnum::kCGrossAmount:
        return trans->rhs_debit == 0 ? QVariant() : trans->rhs_debit;
    case StatementEnum::kCSettlement:
        return trans->rhs_credit == 0 ? QVariant() : trans->rhs_credit;
    case StatementEnum::kCBalance:
        return trans->rhs_ratio == 0 ? QVariant() : trans->rhs_ratio;
    case StatementEnum::kCFirst:
        return trans->lhs_debit == 0 ? QVariant() : trans->lhs_debit;
    case StatementEnum::kCSecond:
        return trans->lhs_credit == 0 ? QVariant() : trans->lhs_credit;
    default:
        return QVariant();
    }
}

QVariant StatementModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return info_.statement_header.at(section);

    return QVariant();
}

void StatementModel::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.statement_header.size())
        return;

    auto Compare = [column, order](const Trans* lhs, const Trans* rhs) -> bool {
        const StatementEnum kColumn { column };

        switch (kColumn) {
        case StatementEnum::kParty:
            return (order == Qt::AscendingOrder) ? (lhs->id < rhs->id) : (lhs->id > rhs->id);
        case StatementEnum::kPBalance:
            return (order == Qt::AscendingOrder) ? (lhs->lhs_ratio < rhs->lhs_ratio) : (lhs->lhs_ratio > rhs->lhs_ratio);
        case StatementEnum::kCGrossAmount:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_debit < rhs->rhs_debit) : (lhs->rhs_debit > rhs->rhs_debit);
        case StatementEnum::kCSettlement:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_credit < rhs->rhs_credit) : (lhs->rhs_credit > rhs->rhs_credit);
        case StatementEnum::kCBalance:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_ratio < rhs->rhs_ratio) : (lhs->rhs_ratio > rhs->rhs_ratio);
        case StatementEnum::kCFirst:
            return (order == Qt::AscendingOrder) ? (lhs->lhs_debit < rhs->lhs_debit) : (lhs->lhs_debit > rhs->lhs_debit);
        case StatementEnum::kCSecond:
            return (order == Qt::AscendingOrder) ? (lhs->lhs_credit < rhs->lhs_credit) : (lhs->lhs_credit > rhs->lhs_credit);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(trans_list_.begin(), trans_list_.end(), Compare);
    emit layoutChanged();
}

void StatementModel::RResetModel(int unit, const QDateTime& start, const QDateTime& end)
{
    if (!start.isValid() || !end.isValid())
        return;

    beginResetModel();
    if (!trans_list_.isEmpty())
        ResourcePool<Trans>::Instance().Recycle(trans_list_);

    sql_->ReadStatement(trans_list_, unit, start, end);
    endResetModel();
}
