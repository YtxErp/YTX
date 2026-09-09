#include "cashflowspecialmodel.h"

#include <QJsonArray>

#include "cashflowstatementenum.h"
#include "component/constantbool.h"
#include "global/resourcepool.h"
#include "utils/nodeutils.h"
#include "utils/templateutils.h"

namespace cash_flow {

SpecialModel::SpecialModel(const QStringList& header, QObject* parent)
    : QAbstractItemModel(parent)
    , header_ { header }
{
    InitFixedNodes();
}

SpecialModel::~SpecialModel()
{
    ResourcePool<Row>::Instance().Recycle(root_);
    ResourcePool<Row>::Instance().Recycle(special_);
    ResourcePool<Row>::Instance().Recycle(list_);
}

QVariant SpecialModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return header_.at(section);
    }

    return QVariant();
}

QModelIndex SpecialModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return {};

    auto* parent_node { GetNodeByIndex(parent) };
    auto* node { parent_node->children.at(row) };

    Q_ASSERT(node);

    return createIndex(row, column, node);
}

QModelIndex SpecialModel::parent(const QModelIndex& index) const
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

int SpecialModel::rowCount(const QModelIndex& parent) const { return GetNodeByIndex(parent)->children.size(); }

int SpecialModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return header_.size();
}

QVariant SpecialModel::data(const QModelIndex& index, int role) const
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
    case RowField::kFinalTotal:
        return node->final_total;
    }
}

void SpecialModel::sort(int column, Qt::SortOrder order)
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
        case RowField::kFinalTotal:
            return utils::CompareMember(lhs, rhs, &Row::final_total, order);
        }
    };

    emit layoutAboutToBeChanged();
    node::SortSubtree(root_, Compare);
    emit layoutChanged();
}

void SpecialModel::Rebuild(const QJsonArray& special_array)
{
    if (special_array.isEmpty()) {
        qDebug() << Q_FUNC_INFO << "Received empty special array";
    }

    QList<Row*> new_list {};
    new_list.reserve(special_array.size());

    for (const auto& value : special_array) {
        Q_ASSERT(value.isObject());

        auto* node { ResourcePool<Row>::Instance().Allocate() };
        node->ReadJson(value.toObject());

        new_list.emplaceBack(node);
    }

    beginResetModel();

    ResourcePool<Row>::Instance().Recycle(list_);
    special_->children.clear();

    list_ = std::move(new_list);

    BuildCounterPartHierarchy();
    node::SortSubtree(special_, [](const auto* lhs, const auto* rhs) { return utils::CompareMember(lhs, rhs, &Row::name, Qt::AscendingOrder); });

    endResetModel();
}

Row* SpecialModel::GetNodeByIndex(const QModelIndex& index) const
{
    if (index.isValid() && index.internalPointer())
        return static_cast<Row*>(index.internalPointer());

    return root_;
}

void SpecialModel::InitFixedNodes()
{
    root_ = CreateBranchNode(QString(), finance::Role::kNone, direction_rule::kDICD);
    special_ = CreateBranchNode(tr("Special"), finance::Role::kNone, direction_rule::kDDCI);

    {
        root_->children = { special_ };
        special_->parent = root_;
    }
}

void SpecialModel::BuildCounterPartHierarchy() const
{
    for (auto* node : std::as_const(list_)) {
        node->parent = special_;
        special_->children.emplaceBack(node);

        node->name = CashKindName(node->cash_kind);
    }
}

Row* SpecialModel::CreateBranchNode(const QString& name, finance::Roles roles, bool direction_rule) const
{
    auto* node { ResourcePool<Row>::Instance().Allocate() };

    node->id = QUuid::createUuidV7();

    node->name = name;
    node->roles = roles;
    node->direction_rule = direction_rule;

    return node;
}
}
