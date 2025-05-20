#include "settlementprimarymodel.h"

#include "component/enumclass.h"
#include "global/resourcepool.h"

SettlementPrimaryModel::SettlementPrimaryModel(Sql* sql, CInfo& info, QObject* parent)
    : QAbstractItemModel { parent }
    , sql_ { qobject_cast<SqlO*>(sql) }
    , info_ { info }
{
}

SettlementPrimaryModel::~SettlementPrimaryModel() { ResourcePool<Node>::Instance().Recycle(node_list_); }

QModelIndex SettlementPrimaryModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex SettlementPrimaryModel::parent(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

int SettlementPrimaryModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return node_list_.size();
}

int SettlementPrimaryModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return info_.settlement_primary_header.size();
}

QVariant SettlementPrimaryModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    const SettlementEnum kColumn { index.column() };
    auto* node { node_list_.at(index.row()) };

    switch (kColumn) {
    case SettlementEnum::kID:
        return node->id;
    case SettlementEnum::kDateTime:
        return node->date_time;
    case SettlementEnum::kDescription:
        return node->description;
    case SettlementEnum::kFinished:
        return node->finished ? node->finished : QVariant();
    case SettlementEnum::kGrossAmount:
        return node->initial_total == 0 ? QVariant() : node->initial_total;
    case SettlementEnum::kParty:
        return node->employee ? node->employee : QVariant();
    default:
        return QVariant();
    }
}

bool SettlementPrimaryModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || index.column() != std::to_underlying(SettlementEnum::kFinished) || role != Qt::EditRole || settlement_finished_)
        return false;

    auto* node { node_list_.at(index.row()) };
    const bool check { value.toBool() };

    check ? sql_->AddSettlementPrimary(node->id, settlement_id_) : sql_->RemoveSettlementPrimary(node->id);

    node->finished = check;

    emit SSyncDouble(settlement_id_, std::to_underlying(SettlementEnum::kGrossAmount), check ? node->initial_total : -node->initial_total);
    return true;
}

QVariant SettlementPrimaryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return info_.settlement_primary_header.at(section);

    return QVariant();
}

void SettlementPrimaryModel::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.settlement_primary_header.size())
        return;

    auto Compare = [column, order](const Node* lhs, const Node* rhs) -> bool {
        const SettlementEnum kColumn { column };

        switch (kColumn) {
        case SettlementEnum::kParty:
            return (order == Qt::AscendingOrder) ? (lhs->employee < rhs->employee) : (lhs->employee > rhs->employee);
        case SettlementEnum::kDateTime:
            return (order == Qt::AscendingOrder) ? (lhs->date_time < rhs->date_time) : (lhs->date_time > rhs->date_time);
        case SettlementEnum::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case SettlementEnum::kFinished:
            return (order == Qt::AscendingOrder) ? (lhs->finished < rhs->finished) : (lhs->finished > rhs->finished);
        case SettlementEnum::kGrossAmount:
            return (order == Qt::AscendingOrder) ? (lhs->initial_total < rhs->initial_total) : (lhs->initial_total > rhs->initial_total);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(node_list_.begin(), node_list_.end(), Compare);
    emit layoutChanged();
}

void SettlementPrimaryModel::RemoveUnfinishedNode()
{
    if (node_list_.isEmpty()) {
        return;
    }

    for (int i = node_list_.size() - 1; i >= 0; --i) {
        if (!node_list_[i]->finished) {
            beginRemoveRows(QModelIndex(), i, i);
            ResourcePool<Node>::Instance().Recycle(node_list_.takeAt(i));
            endRemoveRows();
        }
    }
}

void SettlementPrimaryModel::UpdateSettlementInfo(int party_id, int settlement_id, bool settlement_finished)
{
    party_id_ = party_id;
    settlement_id_ = settlement_id;
    settlement_finished_ = settlement_finished;
}

void SettlementPrimaryModel::RSyncFinished(int party_id, int settlement_id, bool settlement_finished)
{
    UpdateSettlementInfo(party_id, settlement_id, settlement_finished);

    if (settlement_finished) {
        RemoveUnfinishedNode();
    } else {
        const bool is_empty { node_list_.isEmpty() };

        const long long first_add { node_list_.size() };
        sql_->ReadSettlementPrimary(node_list_, party_id_, 0, true);
        const long long last_add { node_list_.size() - 1 };

        if (last_add >= first_add) {
            beginInsertRows(QModelIndex(), first_add, last_add);
            endInsertRows();

            if (is_empty)
                sort(std::to_underlying(SettlementEnum::kDateTime), Qt::AscendingOrder);
        }
    }
}

void SettlementPrimaryModel::RResetModel(int party_id, int settlement_id, bool settlement_finished)
{
    UpdateSettlementInfo(party_id, settlement_id, settlement_finished);

    beginResetModel();
    if (!node_list_.isEmpty()) {
        ResourcePool<Node>::Instance().Recycle(node_list_);
        node_list_.clear();
    }

    if (party_id != 0)
        sql_->ReadSettlementPrimary(node_list_, party_id, settlement_id, settlement_finished);

    endResetModel();
}
