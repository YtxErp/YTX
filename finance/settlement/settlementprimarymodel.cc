#include "settlementprimarymodel.h"

#include <QJsonArray>

#include "component/constantwebsocket.h"
#include "global/resourcepool.h"
#include "settlementenum.h"
#include "websocket/jsongen.h"
#include "websocket/websocket.h"

namespace settlement {

PrimaryModel::PrimaryModel(const QStringList& header, Section section, QObject* parent)
    : QAbstractItemModel { parent }
    , header_ { header }
    , section_ { section }
{
}

PrimaryModel::~PrimaryModel() { ResourcePool<PrimaryRow>::Instance().Recycle(list_); }

void PrimaryModel::RInsertePrimaryRow(const PrimaryRow& row)
{
    auto* settlement { ResourcePool<settlement::PrimaryRow>::Instance().Allocate() };
    *settlement = row;

    const int count { rowCount() };

    beginInsertRows(QModelIndex(), count, count);
    list_.emplaceBack(settlement);
    endInsertRows();
}

void PrimaryModel::RUpdatePrimaryRow(const PrimaryRow& row)
{
    const auto row_index { FindSettlementRow(row.id) };

    if (!row_index)
        return;

    auto* settlement { list_[*row_index] };

    Q_ASSERT(settlement != nullptr);
    Q_ASSERT_X(settlement->partner_id == row.partner_id, "PrimaryModel::RUpdatedPrimaryRow", "Partner id cannot be changed during update");

    if (settlement->partner_id != row.partner_id) {
        qWarning() << Q_FUNC_INFO << "Invalid settlement update: partner id mismatch.";
        return;
    }

    *settlement = row;

    emit dataChanged(index(*row_index, std::to_underlying(PrimaryField::kIssuedTime)), index(*row_index, std::to_underlying(PrimaryField::kAmount)));
}

QModelIndex PrimaryModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column, list_.at(row));
}

QModelIndex PrimaryModel::parent(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

int PrimaryModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return list_.size();
}

int PrimaryModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return header_.size();
}

QVariant PrimaryModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    const PrimaryField column { index.column() };
    auto* settlement { static_cast<PrimaryRow*>(index.internalPointer()) };

    switch (column) {
    case PrimaryField::kIssuedTime:
        return settlement->issued_time;
    case PrimaryField::kDescription:
        return settlement->description;
    case PrimaryField::kStatus:
        return std::to_underlying(settlement->status);
    case PrimaryField::kAmount:
        return settlement->amount;
    case PrimaryField::kPartner:
        return settlement->partner_id;
    }
}

QVariant PrimaryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return header_.at(section);

    return QVariant();
}

void PrimaryModel::sort(int column, Qt::SortOrder order)
{
    const PrimaryField e_column { column };

    auto Compare = [e_column, order](const PrimaryRow* lhs, const PrimaryRow* rhs) -> bool {
        switch (e_column) {
        case PrimaryField::kPartner:
            return utils::CompareMember(lhs, rhs, &PrimaryRow::partner_id, order);
        case PrimaryField::kIssuedTime:
            return utils::CompareMember(lhs, rhs, &PrimaryRow::issued_time, order);
        case PrimaryField::kDescription:
            return utils::CompareMember(lhs, rhs, &PrimaryRow::description, order);
        case PrimaryField::kStatus:
            return utils::CompareMember(lhs, rhs, &PrimaryRow::status, order);
        case PrimaryField::kAmount:
            return utils::CompareMember(lhs, rhs, &PrimaryRow::amount, order);
        }
    };

    emit layoutAboutToBeChanged();
    std::ranges::sort(list_, Compare);
    emit layoutChanged();
}

bool PrimaryModel::removeRows(int row, int /*count*/, const QModelIndex& parent)
{
    beginRemoveRows(parent, row, row);
    auto* settlement { list_.takeAt(row) };
    endRemoveRows();

    QJsonObject message { JsonGen::SettlementDelete(section_, settlement->id, settlement->version) };
    WebSocket::Instance()->SendMessage(WsKey::kSettlementDelete, message);

    ResourcePool<PrimaryRow>::Instance().Recycle(settlement);
    return true;
}

void PrimaryModel::Rebuild(const QJsonArray& array)
{
    if (array.isEmpty()) {
        qDebug() << Q_FUNC_INFO << "Received empty array";
    }

    QList<PrimaryRow*> new_list {};
    new_list.reserve(array.size());

    for (const auto& value : array) {
        Q_ASSERT(value.isObject());

        auto* settlement { ResourcePool<PrimaryRow>::Instance().Allocate() };
        settlement->ReadJson(value.toObject());
        new_list.emplaceBack(settlement);
    }

    std::ranges::sort(new_list, [](const auto* lhs, const auto* rhs) { return utils::CompareMember(lhs, rhs, &PrimaryRow::issued_time, Qt::AscendingOrder); });

    beginResetModel();

    ResourcePool<PrimaryRow>::Instance().Recycle(list_);
    list_ = std::move(new_list);

    endResetModel();
}
}
