#include "statementmodel.h"

#include "component/enumclass.h"
#include "global/resourcepool.h"

StatementModel::StatementModel(Sqlite* sql, CInfo& info, QObject* parent)
    : QAbstractItemModel { parent }
    , sql_ { sql }
    , info_ { info }
{
}

StatementModel::~StatementModel() { ResourcePool<Trans>::Instance().Recycle(is_trans_list_); }

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
    return is_trans_list_.size();
}

int StatementModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return info_.search_node_header.size();
}

QVariant StatementModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    const NodeSearchEnum kColumn { index.column() };

    switch (kColumn) {
    default:
        return QVariant();
    }
}

QVariant StatementModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return info_.search_node_header.at(section);

    return QVariant();
}

void StatementModel::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.search_node_header.size())
        return;

    auto Compare = [column, order](const Trans* lhs, const Trans* rhs) -> bool {
        const NodeSearchEnum kColumn { column };

        switch (kColumn) {
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(trans_list_.begin(), trans_list_.end(), Compare);
    emit layoutChanged();
}

void StatementModel::Query(const QString& text)
{
    beginResetModel();
    switch (info_.section) {
    case Section::kSales:
        break;
    case Section::kPurchase:
        break;
    case Section::kFinance:
    case Section::kProduct:
    case Section::kTask:
    case Section::kStakeholder:
        break;
    default:
        break;
    }

    endResetModel();
}
