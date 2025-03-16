#include "statementsecondarymodel.h"

#include "component/enumclass.h"
#include "global/resourcepool.h"

StatementSecondaryModel::StatementSecondaryModel(Sqlite* sql, CInfo& info, int party_id, QObject* parent)
    : QAbstractItemModel { parent }
    , sql_ { sql }
    , info_ { info }
    , party_id_ { party_id }
{
}

StatementSecondaryModel::~StatementSecondaryModel() { ResourcePool<Trans>::Instance().Recycle(trans_list_); }

QModelIndex StatementSecondaryModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex StatementSecondaryModel::parent(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

int StatementSecondaryModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return trans_list_.size();
}

int StatementSecondaryModel::columnCount(const QModelIndex& /*parent*/) const { return info_.statement_secondary_header.size(); }

QVariant StatementSecondaryModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* trans { trans_list_.at(index.row()) };
    const StatementSecondaryEnum kColumn { index.column() };

    switch (kColumn) {
    case StatementSecondaryEnum::kDateTime:
        return trans->date_time;
    case StatementSecondaryEnum::kInsideProduct:
        return trans->rhs_node;
    case StatementSecondaryEnum::kOutsideProduct:
        return trans->support_id == 0 ? QVariant() : trans->support_id;
    case StatementSecondaryEnum::kFirst:
        return trans->lhs_debit == 0 ? QVariant() : trans->lhs_debit;
    case StatementSecondaryEnum::kSecond:
        return trans->lhs_credit == 0 ? QVariant() : trans->lhs_credit;
    case StatementSecondaryEnum::kUnitPrice:
        return trans->lhs_ratio == 0 ? QVariant() : trans->lhs_ratio;
    case StatementSecondaryEnum::kDescription:
        return trans->description;
    case StatementSecondaryEnum::kGrossAmount:
        return trans->rhs_debit == 0 ? QVariant() : trans->rhs_debit;
    case StatementSecondaryEnum::kSettlement:
        return trans->rhs_credit == 0 ? QVariant() : trans->rhs_credit;
    case StatementSecondaryEnum::kState:
        return trans->state ? trans->state : QVariant();
    default:
        return QVariant();
    }
}

bool StatementSecondaryModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    const StatementSecondaryEnum kColumn { index.column() };
    const int kRow { index.row() };

    auto* trans { trans_list_.at(kRow) };

    switch (kColumn) {
    case StatementSecondaryEnum::kState:
        trans->state = value.toBool();
        break;
    default:
        return false;
    }

    return true;
}

QVariant StatementSecondaryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return info_.statement_secondary_header.at(section);

    return QVariant();
}

void StatementSecondaryModel::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.statement_secondary_header.size() - 1)
        return;

    auto Compare = [column, order](const Trans* lhs, const Trans* rhs) -> bool {
        const StatementSecondaryEnum kColumn { column };

        switch (kColumn) {
        case StatementSecondaryEnum::kOutsideProduct:
            return (order == Qt::AscendingOrder) ? (lhs->support_id < rhs->support_id) : (lhs->support_id > rhs->support_id);
        case StatementSecondaryEnum::kDateTime:
            return (order == Qt::AscendingOrder) ? (lhs->date_time < rhs->date_time) : (lhs->date_time > rhs->date_time);
        case StatementSecondaryEnum::kUnitPrice:
            return (order == Qt::AscendingOrder) ? (lhs->lhs_ratio < rhs->lhs_ratio) : (lhs->lhs_ratio > rhs->lhs_ratio);
        case StatementSecondaryEnum::kFirst:
            return (order == Qt::AscendingOrder) ? (lhs->lhs_debit < rhs->lhs_debit) : (lhs->lhs_debit > rhs->lhs_debit);
        case StatementSecondaryEnum::kSecond:
            return (order == Qt::AscendingOrder) ? (lhs->lhs_credit < rhs->lhs_credit) : (lhs->lhs_credit > rhs->lhs_credit);
        case StatementSecondaryEnum::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case StatementSecondaryEnum::kSettlement:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_credit < rhs->rhs_credit) : (lhs->rhs_credit > rhs->rhs_credit);
        case StatementSecondaryEnum::kGrossAmount:
            return (order == Qt::AscendingOrder) ? (lhs->rhs_debit < rhs->rhs_debit) : (lhs->rhs_debit > rhs->rhs_debit);
        case StatementSecondaryEnum::kState:
            return (order == Qt::AscendingOrder) ? (lhs->state < rhs->state) : (lhs->state > rhs->state);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(trans_list_.begin(), trans_list_.end(), Compare);
    emit layoutChanged();
}

void StatementSecondaryModel::RRetrieveData(int unit, const QDateTime& start, const QDateTime& end)
{
    if (party_id_ <= 0 || !start.isValid() || !end.isValid())
        return;

    beginResetModel();
    if (!trans_list_.isEmpty())
        ResourcePool<Trans>::Instance().Recycle(trans_list_);

    sql_->ReadStatementSecondary(trans_list_, party_id_, unit, start, end);
    endResetModel();
}
