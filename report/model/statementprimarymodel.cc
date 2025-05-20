#include "statementprimarymodel.h"

#include "component/enumclass.h"
#include "global/resourcepool.h"

StatementPrimaryModel::StatementPrimaryModel(Sql* sql, CInfo& info, int party_id, QObject* parent)
    : QAbstractItemModel { parent }
    , sql_ { qobject_cast<SqlO*>(sql) }
    , info_ { info }
    , party_id_ { party_id }
{
}

StatementPrimaryModel::~StatementPrimaryModel() { ResourcePool<Node>::Instance().Recycle(node_list_); }

QModelIndex StatementPrimaryModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex StatementPrimaryModel::parent(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

int StatementPrimaryModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return node_list_.size();
}

int StatementPrimaryModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return info_.statement_primary_header.size();
}

QVariant StatementPrimaryModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* node { node_list_.at(index.row()) };
    const StatementPrimaryEnum kColumn { index.column() };

    switch (kColumn) {
    case StatementPrimaryEnum::kDescription:
        return node->description;
    case StatementPrimaryEnum::kEmployee:
        return node->employee == 0 ? QVariant() : node->employee;
    case StatementPrimaryEnum::kDateTime:
        return node->date_time;
    case StatementPrimaryEnum::kFirst:
        return node->first == 0 ? QVariant() : node->first;
    case StatementPrimaryEnum::kSecond:
        return node->second == 0 ? QVariant() : node->second;
    case StatementPrimaryEnum::kState:
        return node->finished ? node->finished : QVariant();
    case StatementPrimaryEnum::kGrossAmount:
        return node->initial_total == 0 ? QVariant() : node->initial_total;
    case StatementPrimaryEnum::kSettlement:
        return node->final_total == 0 ? QVariant() : node->final_total;
    default:
        return QVariant();
    }
}

bool StatementPrimaryModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    const StatementPrimaryEnum kColumn { index.column() };
    const int kRow { index.row() };

    auto* node { node_list_.at(kRow) };

    switch (kColumn) {
    case StatementPrimaryEnum::kState:
        node->finished = value.toBool();
        break;
    default:
        return false;
    }

    return true;
}

QVariant StatementPrimaryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return info_.statement_primary_header.at(section);

    return QVariant();
}

void StatementPrimaryModel::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.statement_primary_header.size())
        return;

    auto Compare = [column, order](const Node* lhs, const Node* rhs) -> bool {
        const StatementPrimaryEnum kColumn { column };

        switch (kColumn) {
        case StatementPrimaryEnum::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case StatementPrimaryEnum::kEmployee:
            return (order == Qt::AscendingOrder) ? (lhs->employee < rhs->employee) : (lhs->employee > rhs->employee);
        case StatementPrimaryEnum::kDateTime:
            return (order == Qt::AscendingOrder) ? (lhs->date_time < rhs->date_time) : (lhs->date_time > rhs->date_time);
        case StatementPrimaryEnum::kFirst:
            return (order == Qt::AscendingOrder) ? (lhs->first < rhs->first) : (lhs->first > rhs->first);
        case StatementPrimaryEnum::kSecond:
            return (order == Qt::AscendingOrder) ? (lhs->second < rhs->second) : (lhs->second > rhs->second);
        case StatementPrimaryEnum::kState:
            return (order == Qt::AscendingOrder) ? (lhs->finished < rhs->finished) : (lhs->finished > rhs->finished);
        case StatementPrimaryEnum::kSettlement:
            return (order == Qt::AscendingOrder) ? (lhs->final_total < rhs->final_total) : (lhs->final_total > rhs->final_total);
        case StatementPrimaryEnum::kGrossAmount:
            return (order == Qt::AscendingOrder) ? (lhs->initial_total < rhs->initial_total) : (lhs->initial_total > rhs->initial_total);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(node_list_.begin(), node_list_.end(), Compare);
    emit layoutChanged();
}

void StatementPrimaryModel::RResetModel(int unit, const QDateTime& start, const QDateTime& end)
{
    if (party_id_ <= 0 || !start.isValid() || !end.isValid())
        return;

    beginResetModel();

    if (!node_list_.isEmpty())
        ResourcePool<Node>::Instance().Recycle(node_list_);

    sql_->ReadStatementPrimary(node_list_, party_id_, unit, start, end);
    endResetModel();
}
