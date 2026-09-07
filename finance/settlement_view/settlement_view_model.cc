#include "settlement_view_model.h"

#include "utils/templateutils.h"

namespace settlement_view {

Model::Model(const QHash<QUuid, QString>& partner_leaf_path, QObject* parent)
    : QAbstractItemModel(parent)
    , partner_leaf_path_ { partner_leaf_path }
{
}

QVariant Model::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    if (section < 0 || section >= columns_.size())
        return {};

    return columns_.at(section).title;
}

QModelIndex Model::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex Model::parent(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

int Model::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return rows_.size();
}

int Model::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return columns_.size();
}

QVariant Model::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return {};

    const Row& row { rows_.at(index.row()) };
    const Column& column { columns_.at(index.column()) };

    switch (column.type) {
    case ColumnType::kPartner:
        return partner_leaf_path_.value(row.partner_id);

    case ColumnType::kPreviousBalance:
        return row.previous_balance;

    case ColumnType::kMonth:
        return row.months.at(column.month_index);

    case ColumnType::kCurrentAmount:
        return row.current_amount;

    case ColumnType::kCurrentSettled:
        return row.current_settled;

    case ColumnType::kCurrentUnsettled:
        return row.current_unsettled;

    case ColumnType::kCurrentBalance:
        return row.current_balance;
    }

    return {};
}

void Model::sort(int column, Qt::SortOrder order)
{
    if (column < 0 || column >= columns_.size())
        return;

    const Column& e_column { columns_.at(column) };

    auto Compare = [this, e_column, order](const Row& lhs, const Row& rhs) -> bool {
        switch (e_column.type) {
        case ColumnType::kPartner:
            return utils::CompareString(partner_leaf_path_.value(lhs.partner_id), partner_leaf_path_.value(rhs.partner_id), order);

        case ColumnType::kPreviousBalance:
            return utils::CompareValue(lhs.previous_balance, rhs.previous_balance, order);

        case ColumnType::kMonth:
            return utils::CompareValue(lhs.months.at(e_column.month_index), rhs.months.at(e_column.month_index), order);

        case ColumnType::kCurrentAmount:
            return utils::CompareValue(lhs.current_amount, rhs.current_amount, order);

        case ColumnType::kCurrentSettled:
            return utils::CompareValue(lhs.current_settled, rhs.current_settled, order);

        case ColumnType::kCurrentUnsettled:
            return utils::CompareValue(lhs.current_unsettled, rhs.current_unsettled, order);

        case ColumnType::kCurrentBalance:
            return utils::CompareValue(lhs.current_balance, rhs.current_balance, order);
        }

        return false;
    };

    emit layoutAboutToBeChanged();
    std::ranges::sort(rows_, Compare);
    emit layoutChanged();
}

void Model::Rebuild(const QJsonArray& array)
{
    if (array.isEmpty())
        qDebug() << Q_FUNC_INFO << "Received empty array";

    QList<Row> new_rows {};
    new_rows.reserve(array.size());

    for (const auto& value : array) {
        if (!value.isObject()) {
            qWarning() << Q_FUNC_INFO << "Invalid data, expected object:" << value;
            continue;
        }

        Row row {};
        row.ReadJson(value.toObject());

        new_rows.emplaceBack(std::move(row));
    }

    std::ranges::sort(new_rows, [](const Row& lhs, const Row& rhs) { return utils::CompareValue(lhs.months.back(), rhs.months.back(), Qt::DescendingOrder); });

    beginResetModel();

    rows_ = std::move(new_rows);

    endResetModel();
}

void Model::RebuildHeader(const utils::DateRange& date_range)
{
    Q_ASSERT(date_range.IsValid());

    beginResetModel();

    columns_.clear();
    rows_.clear();

    columns_.append({ ColumnType::kPartner, tr("Partner") });
    columns_.append({ ColumnType::kPreviousBalance, tr("Previous Balance") });

    int month_index { 0 };

    QDate date { date_range.start.year(), date_range.start.month(), 1 };
    const QDate end { date_range.end.year(), date_range.end.month(), 1 };

    while (date <= end) {
        columns_.append({ ColumnType::kMonth, date.toString(QStringLiteral("yyyy-MM")), month_index++ });

        date = date.addMonths(1);
    }

    columns_.append({ ColumnType::kCurrentAmount, tr("Current Amount") });
    columns_.append({ ColumnType::kCurrentSettled, tr("Current Settled") });
    columns_.append({ ColumnType::kCurrentUnsettled, tr("Current Unsettled") });
    columns_.append({ ColumnType::kCurrentBalance, tr("Current Balance") });

    endResetModel();
}
}
