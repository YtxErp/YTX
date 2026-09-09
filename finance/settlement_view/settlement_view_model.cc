#include "settlement_view_model.h"

#include "global/resourcepool.h"
#include "utils/templateutils.h"

namespace settlement_view {

Model::Model(const QHash<QUuid, QString>& partner_leaf_path, QObject* parent)
    : QAbstractItemModel(parent)
    , partner_leaf_path_ { partner_leaf_path }
{
}

Model::~Model() { ResourcePool<Row>::Instance().Recycle(rows_); }

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

    return createIndex(row, column, rows_.at(row));
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

    const auto* row { static_cast<const Row*>(index.internalPointer()) };
    const Column& column { columns_.at(index.column()) };

    if (row->type == RowType::kSpacer)
        return {};

    if (row->type == RowType::kTotal && column.type == ColumnType::kPartner)
        return tr("Total");

    switch (column.type) {
    case ColumnType::kPartner:
        return partner_leaf_path_.value(row->partner_id);

    case ColumnType::kPreviousBalance:
        return row->previous_balance;

    case ColumnType::kMonth:
        return row->months.at(column.month_index);

    case ColumnType::kCurrentAmount:
        return row->current_amount;

    case ColumnType::kCurrentSettled:
        return row->current_settled;

    case ColumnType::kCurrentUnsettled:
        return row->current_unsettled;

    case ColumnType::kCurrentBalance:
        return row->current_balance;
    }

    return {};
}

void Model::sort(int column, Qt::SortOrder order)
{
    if (column < 0 || column >= columns_.size())
        return;

    const Column& e_column { columns_.at(column) };

    auto Compare = [this, e_column, order](const Row* lhs, const Row* rhs) -> bool {
        switch (e_column.type) {
        case ColumnType::kPartner:
            return utils::CompareString(partner_leaf_path_.value(lhs->partner_id), partner_leaf_path_.value(rhs->partner_id), order);

        case ColumnType::kPreviousBalance:
            return utils::CompareValue(lhs->previous_balance, rhs->previous_balance, order);

        case ColumnType::kMonth:
            return utils::CompareValue(lhs->months.at(e_column.month_index), rhs->months.at(e_column.month_index), order);

        case ColumnType::kCurrentAmount:
            return utils::CompareValue(lhs->current_amount, rhs->current_amount, order);

        case ColumnType::kCurrentSettled:
            return utils::CompareValue(lhs->current_settled, rhs->current_settled, order);

        case ColumnType::kCurrentUnsettled:
            return utils::CompareValue(lhs->current_unsettled, rhs->current_unsettled, order);

        case ColumnType::kCurrentBalance:
            return utils::CompareValue(lhs->current_balance, rhs->current_balance, order);
        }

        return false;
    };

    if (rows_.size() <= 2)
        return;

    emit layoutAboutToBeChanged();
    std::ranges::sort(rows_.begin(), rows_.end() - 2, Compare);
    emit layoutChanged();
}

// Settlement view presentation:
// - Month columns show order amounts grouped by issued time.
// - Current Amount equals the sum of all month columns.
// - Spacer and Total rows are always kept at the bottom.
// - Sorting applies only to data rows.
void Model::Rebuild(const QJsonArray& array)
{
    if (array.isEmpty()) {
        qDebug() << Q_FUNC_INFO << "Received empty array";
        return;
    }

    QList<Row*> new_rows {};
    new_rows.reserve(array.size() + 2);

    auto* total { ResourcePool<Row>::Instance().Allocate() };
    total->type = RowType::kTotal;

    const auto month_count { std::ranges::count_if(columns_, [](const Column& column) { return column.type == ColumnType::kMonth; }) };
    total->months.resize(month_count);

    for (const auto& value : array) {
        Q_ASSERT(value.isObject());

        auto* row { ResourcePool<Row>::Instance().Allocate() };
        row->ReadJson(value.toObject());

        total->Accumulate(*row);
        new_rows.emplaceBack(row);
    }

    if (!new_rows.isEmpty()) {
        std::ranges::sort(
            new_rows, [](const Row* lhs, const Row* rhs) { return utils::CompareValue(lhs->current_balance, rhs->current_balance, Qt::DescendingOrder); });

        auto* spacer { ResourcePool<Row>::Instance().Allocate() };
        spacer->type = RowType::kSpacer;

        new_rows.emplaceBack(spacer);
        new_rows.emplaceBack(total);
    } else {
        ResourcePool<Row>::Instance().Recycle(total);
    }

    beginResetModel();

    ResourcePool<Row>::Instance().Recycle(rows_);
    rows_ = std::move(new_rows);

    endResetModel();
}

void Model::RebuildHeader(const utils::DateRange& date_range)
{
    Q_ASSERT(date_range.IsValid());

    if (range_.start == date_range.start && range_.end == date_range.end) {
        return;
    }

    qDebug() << Q_FUNC_INFO;

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

    range_ = date_range;

    endResetModel();
}
}
