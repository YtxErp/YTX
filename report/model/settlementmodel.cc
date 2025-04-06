#include "settlementmodel.h"

#include "component/constvalue.h"
#include "component/enumclass.h"
#include "global/resourcepool.h"
#include "tree/model/nodemodelutils.h"

SettlementModel::SettlementModel(Sqlite* sql, CInfo& info, QObject* parent)
    : QAbstractItemModel { parent }
    , sql_ { qobject_cast<SqliteO*>(sql) }
    , info_ { info }
{
}

SettlementModel::~SettlementModel() { ResourcePool<Node>::Instance().Recycle(node_list_); }

void SettlementModel::RSyncDouble(int node_id, int column, double delta1)
{
    if (delta1 == 0.0 || column != std::to_underlying(SettlementEnum::kGrossAmount))
        return;

    int row { 0 };

    for (auto* node : std::as_const(node_list_)) {
        if (node->id == node_id) {
            node->initial_total += delta1;

            emit SSyncDouble(node->party, std::to_underlying(NodeEnumS::kAmount), -delta1); // send to stakeholder
            emit dataChanged(index(row, std::to_underlying(SettlementEnum::kGrossAmount)), index(row, std::to_underlying(SettlementEnum::kGrossAmount)));

            sql_->WriteField(info_.settlement, kGrossAmount, node->initial_total, node->id);
            break;
        }

        ++row;
    }
}

QModelIndex SettlementModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex SettlementModel::parent(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

int SettlementModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return node_list_.size();
}

int SettlementModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return info_.settlement_header.size();
}

QVariant SettlementModel::data(const QModelIndex& index, int role) const
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
        return node->party == 0 ? QVariant() : node->party;
    default:
        return QVariant();
    }
}

bool SettlementModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    const SettlementEnum kColumn { index.column() };
    const int kRow { index.row() };

    auto* node { node_list_.at(kRow) };

    switch (kColumn) {
    case SettlementEnum::kDateTime:
        NodeModelUtils::UpdateField(sql_, node, info_.settlement, kDateTime, value.toString(), &Node::date_time);
        break;
    case SettlementEnum::kDescription:
        NodeModelUtils::UpdateField(sql_, node, info_.settlement, kDescription, value.toString(), &Node::description);
        break;
    case SettlementEnum::kParty:
        UpdateParty(node, value.toInt());
        break;
    case SettlementEnum::kFinished:
        UpdateFinished(node, value.toBool());
        break;
    default:
        return false;
    }

    emit SResizeColumnToContents(index.column());
    return true;
}

QVariant SettlementModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return info_.settlement_header.at(section);

    return QVariant();
}

Qt::ItemFlags SettlementModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const SettlementEnum kColumn { index.column() };

    switch (kColumn) {
    case SettlementEnum::kDateTime:
    case SettlementEnum::kDescription:
    case SettlementEnum::kParty:
        flags |= Qt::ItemIsEditable;
        break;
    default:
        flags &= ~Qt::ItemIsEditable;
        break;
    }

    const bool non_editable { index.siblingAtColumn(std::to_underlying(SettlementEnum::kFinished)).data().toBool() };
    if (non_editable)
        flags &= ~Qt::ItemIsEditable;

    return flags;
}

void SettlementModel::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.settlement_header.size())
        return;

    auto Compare = [column, order](const Node* lhs, const Node* rhs) -> bool {
        const SettlementEnum kColumn { column };

        switch (kColumn) {
        case SettlementEnum::kParty:
            return (order == Qt::AscendingOrder) ? (lhs->party < rhs->party) : (lhs->party > rhs->party);
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

bool SettlementModel::removeRows(int row, int /*count*/, const QModelIndex& parent)
{
    beginRemoveRows(parent, row, row);
    auto* node { node_list_.takeAt(row) };
    endRemoveRows();

    if (node->initial_total != 0.0)
        emit SSyncDouble(node->party, std::to_underlying(NodeEnumS::kAmount), node->initial_total);

    sql_->RemoveSettlement(node->id);
    ResourcePool<Node>::Instance().Recycle(node);

    emit SResetModel(0, 0, false);
    return true;
}

bool SettlementModel::insertRows(int row, int /*count*/, const QModelIndex& parent)
{
    auto* node { ResourcePool<Node>::Instance().Allocate() };
    const bool is_empty { node_list_.isEmpty() };

    node->date_time = QDateTime::currentDateTime().toString(kDateTimeFST);

    beginInsertRows(parent, row, row);
    node_list_.emplaceBack(node);
    endInsertRows();

    if (is_empty)
        emit SResizeColumnToContents(std::to_underlying(SettlementEnum::kDateTime));

    sql_->WriteSettlement(node);
    return true;
}

bool SettlementModel::UpdateParty(Node* node, int party_id)
{
    if (sql_->SettlementReference(node->id) || node->party == party_id)
        return false;

    node->party = party_id;
    sql_->WriteField(info_.settlement, kParty, party_id, node->id);

    emit SResetModel(party_id, node->id, false);
    return true;
}

bool SettlementModel::UpdateFinished(Node* node, bool finished)
{
    if (node->party == 0)
        return false;

    node->finished = finished;
    sql_->WriteField(info_.settlement, kFinished, finished, node->id);

    emit SSyncFinished(node->party, node->id, finished);
    return true;
}

void SettlementModel::ResetModel(const QDateTime& start, const QDateTime& end)
{
    if (!start.isValid() || !end.isValid())
        return;

    beginResetModel();
    if (!node_list_.isEmpty()) {
        ResourcePool<Node>::Instance().Recycle(node_list_);
        node_list_.clear();
    }

    sql_->ReadSettlement(node_list_, start, end);
    endResetModel();
}
