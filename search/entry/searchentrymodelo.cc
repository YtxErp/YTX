#include "searchentrymodelo.h"

#include "enum/entryenum.h"
#include "global/entrypool.h"

namespace search {
EntryModelO::EntryModelO(CSectionInfo& info, const QHash<QUuid, tag::Row*>& tag_hash, QObject* parent)
    : EntryModel { info, tag_hash, parent }
{
}

EntryModelO::~EntryModelO() { EntryPool::Instance().Recycle(list_, info_.section); }

QVariant EntryModelO::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    const EntryEnumO column { index.column() };
    auto* d_entry { static_cast<EntryO*>(index.internalPointer()) };

    switch (column) {
    case EntryEnumO::kLhsNode:
        return d_entry->lhs_node;
    case EntryEnumO::kUnitPrice:
        return d_entry->unit_price;
    case EntryEnumO::kDescription:
        return d_entry->description;
    case EntryEnumO::kExternalSku:
        return QVariant();
    case EntryEnumO::kTag:
        return d_entry->tag;
    case EntryEnumO::kRhsNode:
        return d_entry->rhs_node;
    case EntryEnumO::kCount:
        return d_entry->count;
    case EntryEnumO::kMeasure:
        return d_entry->measure;
    case EntryEnumO::kInitial:
        return d_entry->initial;
    case EntryEnumO::kStatus:
        return d_entry->status;
    }
}

void EntryModelO::sort(int column, Qt::SortOrder order)
{
    const EntryEnumO e_column { column };

    auto Compare = [e_column, order](const Entry* lhs, const Entry* rhs) -> bool {
        auto* d_lhs { DerivedPtr<EntryO>(lhs) };
        auto* d_rhs { DerivedPtr<EntryO>(rhs) };

        switch (e_column) {
        case EntryEnumO::kLhsNode:
            return utils::CompareMember(lhs, rhs, &Entry::lhs_node, order);
        case EntryEnumO::kRhsNode:
            return utils::CompareMember(lhs, rhs, &Entry::rhs_node, order);
        case EntryEnumO::kDescription:
            return utils::CompareMember(lhs, rhs, &Entry::description, order);
        case EntryEnumO::kCount:
            return utils::CompareMember(d_lhs, d_rhs, &EntryO::count, order);
        case EntryEnumO::kUnitPrice:
            return utils::CompareMember(d_lhs, d_rhs, &EntryO::unit_price, order);
        case EntryEnumO::kMeasure:
            return utils::CompareMember(d_lhs, d_rhs, &EntryO::measure, order);
        case EntryEnumO::kInitial:
            return utils::CompareMember(d_lhs, d_rhs, &EntryO::initial, order);
        case EntryEnumO::kTag:
            return utils::CompareMember(lhs, rhs, &Entry::tag, order);
        case EntryEnumO::kExternalSku:
        case EntryEnumO::kStatus:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::ranges::sort(list_, Compare);
    emit layoutChanged();
}
}
