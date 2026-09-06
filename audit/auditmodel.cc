#include "auditmodel.h"

#include "auditenum.h"
#include "enum/section.h"
#include "global/resourcepool.h"
#include "utils/templateutils.h"

namespace audit {

Model::Model(const Info& info, const QStringList& header, Section section, QObject* parent)
    : QAbstractItemModel(parent)
    , section_ { section }
    , info_ { info }
    , header_ { header }
{
}

QVariant Model::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return header_.at(section);

    return QVariant();
}

QModelIndex Model::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column, list_.at(row));
}

QVariant Model::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    const RowField column { index.column() };
    const auto* row { static_cast<Row*>(index.internalPointer()) };

    switch (column) {
    case RowField::kTargetId:
        return row->target_id.toString(QUuid::WithoutBraces).left(12);
    case RowField::kUsername:
        return row->username;
    case RowField::kCreatedTime:
        return row->created_time;
    case RowField::kTargetCode:
        return row->target_code;
    case RowField::kBefore:
        return JsonValueToString(row->before);
    case RowField::kAfter:
        return JsonValueToString(row->after);
    case RowField::kTargetOperation:
        return info_.target_operation_hash.value(row->target_operation);
    case RowField::kLevel:
        return info_.level_hash.value(row->level);
    case RowField::kTargetType:
        return info_.target_type_hash.value(row->target_type);
    case RowField::kLhsNode:
        return ResolveNode(row->lhs_node);
    case RowField::kRhsNode:
        return ResolveNode(row->rhs_node);
    case RowField::kTargetField:
        return info_.target_field_hash.value(row->target_field);
    }
}

void Model::sort(int column, Qt::SortOrder order)
{
    // Convert integer column to the structured enum using brace initialization
    const RowField e_column { column };

    // Define a lambda for comparison based on the selected column and sort order
    auto Compare = [order, e_column](const Row* lhs, const Row* rhs) -> bool {
        switch (e_column) {
        case RowField::kTargetId:
            return utils::CompareMember(lhs, rhs, &Row::target_id, order);
        case RowField::kUsername:
            return utils::CompareMember(lhs, rhs, &Row::username, order);
        case RowField::kLhsNode:
            return utils::CompareMember(lhs, rhs, &Row::lhs_node, order);
        case RowField::kRhsNode:
            return utils::CompareMember(lhs, rhs, &Row::rhs_node, order);
        case RowField::kTargetCode:
            return utils::CompareMember(lhs, rhs, &Row::target_code, order);
        case RowField::kTargetOperation:
            return utils::CompareMember(lhs, rhs, &Row::target_operation, order);
        case RowField::kTargetType:
            return utils::CompareMember(lhs, rhs, &Row::target_type, order);
        case RowField::kLevel:
            return utils::CompareMember(lhs, rhs, &Row::level, order);
        case RowField::kCreatedTime:
            return utils::CompareMember(lhs, rhs, &Row::created_time, order);
        case RowField::kTargetField:
            return utils::CompareMember(lhs, rhs, &Row::target_field, order);
        case RowField::kBefore:
        case RowField::kAfter:
            return false;
        }
    };

    // Notify the view that the layout is about to change
    emit layoutAboutToBeChanged();

    // Perform the sort on the underlying data list
    std::ranges::sort(list_, Compare);

    // Notify the view that the layout has been updated
    emit layoutChanged();
}

void Model::Rebuild(const QJsonArray& array)
{
    if (array.isEmpty()) {
        qDebug() << Q_FUNC_INFO << "Received empty array";
    }

    // Parse outside the reset block
    QList<Row*> new_list {};
    new_list.reserve(array.size());

    for (const auto& value : array) {
        if (!value.isObject()) {
            qWarning() << Q_FUNC_INFO << "Invalid data, expected object:" << value;
            continue;
        }

        auto* entry { ResourcePool<Row>::Instance().Allocate() };
        entry->ReadJson(value.toObject());
        new_list.emplaceBack(entry);
    }

    std::ranges::sort(new_list, [](const auto* lhs, const auto* rhs) { return utils::CompareMember(lhs, rhs, &Row::created_time, Qt::AscendingOrder); });

    // Keep reset block as short as possible
    beginResetModel();

    ResourcePool<Row>::Instance().Recycle(list_);
    list_ = std::move(new_list);

    endResetModel();
}

const QString Model::NodePath(const QHash<QUuid, QString>* leaf, const QHash<QUuid, QString>* branch, const QUuid& node_id) const
{
    if (const auto it = leaf->constFind(node_id); it != leaf->constEnd())
        return it.value();

    if (const auto it = branch->constFind(node_id); it != branch->constEnd())
        return it.value();

    static const QString kEmpty {};
    return kEmpty;
}

QVariant Model::ResolveNode(const QUuid& node_id) const
{
    switch (section_) {
    case Section::kFinance:
        return NodePath(info_.f_leaf_path, info_.f_branch_path, node_id);
    case Section::kTask:
        return NodePath(info_.t_leaf_path, info_.t_branch_path, node_id);
    case Section::kInventory:
        return NodePath(info_.i_leaf_path, info_.i_branch_path, node_id);
    case Section::kPartner:
    case Section::kSale:
    case Section::kPurchase:
        return NodePath(info_.p_leaf_path, info_.p_branch_path, node_id);
    }
    return QVariant();
}

QString Model::JsonValueToString(const QJsonValue& value)
{
    switch (value.type()) {
    case QJsonValue::String:
        return value.toString();
    case QJsonValue::Object:
        return QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact));
    case QJsonValue::Array:
        return QString::fromUtf8(QJsonDocument(value.toArray()).toJson(QJsonDocument::Indented));
    case QJsonValue::Bool:
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    case QJsonValue::Double:
        return QString::number(value.toDouble());
    case QJsonValue::Null:
    case QJsonValue::Undefined:
        return {};
    }
}

}
