#include "settlementsecondarymodel.h"

#include <QJsonArray>
#include <QTimer>

#include "component/constant.h"
#include "global/resourcepool.h"
#include "settlementenum.h"
#include "utils/templateutils.h"

namespace settlement {

SecondaryModel::SecondaryModel(const QStringList& header, SettlementStatus status, QObject* parent)
    : QAbstractItemModel { parent }
    , header_ { header }
    , status_ { status }
{
}

SecondaryModel::~SecondaryModel() { ResourcePool<SecondaryRow>::Instance().Recycle(list_cache_); }

QModelIndex SecondaryModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column, list_.at(row));
}

QModelIndex SecondaryModel::parent(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

int SecondaryModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return list_.size();
}

int SecondaryModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return header_.size();
}

QVariant SecondaryModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    const SecondaryField column { index.column() };
    auto* settlement_node { static_cast<SecondaryRow*>(index.internalPointer()) };

    switch (column) {
    case SecondaryField::kIssuedTime:
        return settlement_node->issued_time;
    case SecondaryField::kDescription:
        return settlement_node->description;
    case SecondaryField::kAmount:
        return settlement_node->amount;
    case SecondaryField::kEmployee:
        return settlement_node->employee_id;
    case SecondaryField::kIsSettled:
        return settlement_node->is_settled;
    }
}

bool SecondaryModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || index.column() != std::to_underlying(SecondaryField::kIsSettled) || role != Qt::EditRole)
        return false;

    if (status_ == SettlementStatus::kReleased)
        return false;

    if (data(index, role) == value)
        return false;

    auto* settlement_node { static_cast<SecondaryRow*>(index.internalPointer()) };

    const bool is_selected { value.toBool() };

    settlement_node->is_settled = is_selected;

    if (is_selected)
        pending_selected_.insert(settlement_node->id);
    else
        pending_selected_.remove(settlement_node->id);

    emit SSyncAmount(settlement_node->amount * (is_selected ? 1 : -1));

    return true;
}

QVariant SecondaryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return header_.at(section);

    return QVariant();
}

void SecondaryModel::sort(int column, Qt::SortOrder order)
{
    const SecondaryField e_column { column };

    auto Compare = [e_column, order](const SecondaryRow* lhs, const SecondaryRow* rhs) -> bool {
        switch (e_column) {
        case SecondaryField::kEmployee:
            return utils::CompareMember(lhs, rhs, &SecondaryRow::employee_id, order);
        case SecondaryField::kIssuedTime:
            return utils::CompareMember(lhs, rhs, &SecondaryRow::issued_time, order);
        case SecondaryField::kDescription:
            return utils::CompareMember(lhs, rhs, &SecondaryRow::description, order);
        case SecondaryField::kAmount:
            return utils::CompareMember(lhs, rhs, &SecondaryRow::amount, order);
        case SecondaryField::kIsSettled:
            return utils::CompareMember(lhs, rhs, &SecondaryRow::is_settled, order);
        }
    };

    emit layoutAboutToBeChanged();
    std::ranges::sort(list_, Compare);
    emit layoutChanged();
}

void SecondaryModel::Rebuild(const QJsonArray& array)
{
    if (array.isEmpty()) {
        qDebug() << Q_FUNC_INFO << "Received empty array";
    }

    QList<SecondaryRow*> new_cache {};
    new_cache.reserve(array.size());

    for (const auto& value : array) {
        Q_ASSERT(value.isObject());

        auto* settlement { ResourcePool<SecondaryRow>::Instance().Allocate() };
        settlement->ReadJson(value.toObject());

        new_cache.emplaceBack(settlement);
    }

    std::ranges::sort(
        new_cache, [](const auto* lhs, const auto* rhs) { return utils::CompareMember(lhs, rhs, &SecondaryRow::issued_time, Qt::AscendingOrder); });

    beginResetModel();

    ResourcePool<SecondaryRow>::Instance().Recycle(list_cache_);
    list_cache_ = std::move(new_cache);

    list_.clear();

    for (auto* entry : std::as_const(list_cache_)) {
        switch (status_) {
        case SettlementStatus::kReleased:
            if (entry->is_settled)
                list_.emplaceBack(entry);
            break;
        case SettlementStatus::kRecalled:
            list_.emplaceBack(entry);
            break;
        }
    }

    endResetModel();
}

void SecondaryModel::UpdateStatus(SettlementStatus status)
{
    if (status_ == status)
        return;

    status_ = status;

    beginResetModel();

    if (status == SettlementStatus::kReleased) {
        list_.erase(std::remove_if(list_.begin(), list_.end(), [](const SecondaryRow* node) { return !node->is_settled; }), list_.end());
    }

    if (status == SettlementStatus::kRecalled) {
        for (auto* node : std::as_const(list_cache_))
            node->is_settled = false;

        list_ = list_cache_;
    }

    endResetModel();
}

void SecondaryModel::Finalize(QJsonObject& message)
{
    {
        QJsonArray selected_array {};
        for (const auto& id : std::as_const(pending_selected_)) {
            selected_array.append(id.toString(QUuid::WithoutBraces));
        }

        message.insert(kSettlementItemSelected, selected_array);
    }

    pending_selected_.clear();
}
}
