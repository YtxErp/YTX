#include "statementprimarymodel.h"

#include <QJsonArray>
#include <QJsonObject>

#include "global/resourcepool.h"
#include "statementenum.h"
#include "utils/templateutils.h"

namespace statement {

PrimaryModel::PrimaryModel(const QStringList& header, QObject* parent)
    : QAbstractItemModel { parent }
    , header_ { header }
{
}

PrimaryModel::~PrimaryModel() { ResourcePool<PrimaryRow>::Instance().Recycle(list_); }

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
    const auto* statement { static_cast<PrimaryRow*>(index.internalPointer()) };

    if (statement->type == RowType::kSpacer)
        return {};

    if (statement->type == RowType::kTotal && column == PrimaryField::kPartner)
        return tr("Total");

    switch (column) {
    case PrimaryField::kPartner:
        return statement->partner_id;
    case PrimaryField::kAmount:
        return statement->amount;
    case PrimaryField::kCount:
        return statement->count;
    case PrimaryField::kMeasure:
        return statement->measure;
    case PrimaryField::kPlaceholder:
        return QVariant();
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
    if (column < 0 || column >= columnCount())
        return;

    const PrimaryField e_column { column };

    auto Compare = [e_column, order](const PrimaryRow* lhs, const PrimaryRow* rhs) -> bool {
        switch (e_column) {
        case PrimaryField::kPartner:
            return utils::CompareMember(lhs, rhs, &PrimaryRow::partner_id, order);
        case PrimaryField::kAmount:
            return utils::CompareMember(lhs, rhs, &PrimaryRow::amount, order);
        case PrimaryField::kCount:
            return utils::CompareMember(lhs, rhs, &PrimaryRow::count, order);
        case PrimaryField::kMeasure:
            return utils::CompareMember(lhs, rhs, &PrimaryRow::measure, order);
        case PrimaryField::kPlaceholder:
            return false;
        }
    };

    if (list_.size() <= 2)
        return;

    emit layoutAboutToBeChanged();
    std::ranges::sort(list_.begin(), list_.end() - 2, Compare);
    emit layoutChanged();
}

void PrimaryModel::Rebuild(const QJsonArray& array)
{
    if (array.isEmpty()) {
        qDebug() << Q_FUNC_INFO << "Received empty array";
    }

    QList<PrimaryRow*> new_list {};
    new_list.reserve(array.size() + 2);

    auto* total { ResourcePool<PrimaryRow>::Instance().Allocate() };
    total->type = RowType::kTotal;

    for (const auto& value : array) {
        Q_ASSERT(value.isObject());

        auto* statement { ResourcePool<PrimaryRow>::Instance().Allocate() };
        statement->ReadJson(value.toObject());

        total->Accumulate(*statement);
        new_list.emplaceBack(statement);
    }

    if (!new_list.isEmpty()) {
        std::ranges::sort(new_list, [](const auto* lhs, const auto* rhs) { return utils::CompareMember(lhs, rhs, &PrimaryRow::amount, Qt::DescendingOrder); });

        auto* spacer { ResourcePool<PrimaryRow>::Instance().Allocate() };
        spacer->type = RowType::kSpacer;

        new_list.emplaceBack(spacer);
        new_list.emplaceBack(total);
    } else {
        ResourcePool<PrimaryRow>::Instance().Recycle(total);
    }

    beginResetModel();

    ResourcePool<PrimaryRow>::Instance().Recycle(list_);
    list_ = std::move(new_list);

    endResetModel();
}

}
