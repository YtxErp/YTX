#include "balancesheetmodel.h"

#include "balancesheetenum.h"
#include "global/resourcepool.h"
#include "utils/nodeutils.h"
#include "utils/pathutils.h"
#include "utils/templateutils.h"

namespace balance_sheet {

Model::Model(const QStringList& header, QObject* parent)
    : QAbstractItemModel(parent)
    , header_ { header }
{
    root_ = ResourcePool<Row>::Instance().Allocate();
    root_->kind = NodeKind::kBranch;
    root_->direction_rule = false;
    root_->name = QString();
    root_->id = QUuid();
}

Model::~Model()
{
    ResourcePool<Row>::Instance().Recycle(node_hash_);
    ResourcePool<Row>::Instance().Recycle(root_);
}

QVariant Model::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return header_.at(section);
    }

    return QVariant();
}

QModelIndex Model::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return {};

    auto* parent_node { GetNodeByIndex(parent) };
    auto* node { parent_node->children.at(row) };

    Q_ASSERT(node);

    return createIndex(row, column, node);
}

QModelIndex Model::parent(const QModelIndex& index) const
{
    // root_'s index is QModelIndex(), root_'s id == -1
    if (!index.isValid())
        return {};

    auto* node { static_cast<Row*>(index.internalPointer()) };
    Q_ASSERT(node);

    auto* parent { node->parent };
    Q_ASSERT(parent);

    if (parent == root_)
        return {};

    // Parent node should have a parent (grandparent)
    auto* grandparent { parent->parent };
    Q_ASSERT(grandparent);

    // Find parent's row in grandparent's children
    const qsizetype row { grandparent->children.indexOf(parent) };
    Q_ASSERT(row >= 0);

    return createIndex(static_cast<int>(row), 0, parent);
}

int Model::rowCount(const QModelIndex& parent) const { return GetNodeByIndex(parent)->children.size(); }

int Model::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return header_.size();
}

QVariant Model::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    auto* node { static_cast<Row*>(index.internalPointer()) };
    Q_ASSERT(node != nullptr);

    const RowField column { index.column() };

    switch (column) {
    case RowField::kName:
        return node->name;
    case RowField::kCode:
        return node->code;
    case RowField::kDescription:
        return node->description;
    case RowField::kDirectionRule:
        return node->direction_rule;
    case RowField::kKind:
        return std::to_underlying(node->kind);
    case RowField::kClosingBalance:
        return node->closing_balance;
    case RowField::kOpeningBalance:
        return node->opening_balance;
    case RowField::kChangeAmount:
        return node->change_amount;
    case RowField::kChangeRate:
        return node->change_rate;
    }
}

void Model::sort(int column, Qt::SortOrder order)
{
    const RowField e_column { column };

    auto Compare = [e_column, order](const Row* lhs, const Row* rhs) -> bool {
        switch (e_column) {
        case RowField::kName:
            return utils::CompareMember(lhs, rhs, &Row::name, order);
        case RowField::kCode:
            return utils::CompareMember(lhs, rhs, &Row::code, order);
        case RowField::kDescription:
            return utils::CompareMember(lhs, rhs, &Row::description, order);
        case RowField::kDirectionRule:
            return utils::CompareMember(lhs, rhs, &Row::direction_rule, order);
        case RowField::kKind:
            return utils::CompareMember(lhs, rhs, &Row::kind, order);
        case RowField::kClosingBalance:
            return utils::CompareMember(lhs, rhs, &Row::closing_balance, order);
        case RowField::kOpeningBalance:
            return utils::CompareMember(lhs, rhs, &Row::opening_balance, order);
        case RowField::kChangeAmount:
            return utils::CompareMember(lhs, rhs, &Row::change_amount, order);
        case RowField::kChangeRate:
            return utils::CompareMember(lhs, rhs, &Row::change_rate, order);
        }
    };

    emit layoutAboutToBeChanged();
    node::SortSubtree(root_, Compare);
    emit layoutChanged();
}

void Model::Rebuild(const QJsonArray& node_array, const QJsonArray& path_array)
{
    if (node_array.isEmpty()) {
        qDebug() << Q_FUNC_INFO << "Received empty node array";
    }

    if (path_array.isEmpty()) {
        qDebug() << Q_FUNC_INFO << "Received empty path array";
    }

    QHash<QUuid, Row*> new_hash {};

    {
        new_hash.reserve(node_array.size());

        for (const auto& value : node_array) {
            Q_ASSERT(value.isObject());

            auto* node { ResourcePool<Row>::Instance().Allocate() };
            node->ReadJson(value.toObject());

            new_hash.insert(node->id, node);
        }

        const auto paths { path::Parse(path_array) };
        path::BuildHierarchy(new_hash, paths);
    }

    beginResetModel();

    ResourcePool<Row>::Instance().Recycle(node_hash_);
    root_->children.clear();

    node_hash_ = std::move(new_hash);

    path::AttachRootNodes(node_hash_, root_);
    node::SortSubtree(root_, [](const auto* lhs, const auto* rhs) { return utils::CompareMember(lhs, rhs, &Row::closing_balance, Qt::DescendingOrder); });

    endResetModel();
}

Row* Model::GetNodeByIndex(const QModelIndex& index) const
{
    if (index.isValid() && index.internalPointer())
        return static_cast<Row*>(index.internalPointer());

    return root_;
}

}
