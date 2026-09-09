#include "cashflowstatementmodel.h"

#include <QJsonArray>

#include "cashflowstatementenum.h"
#include "component/constantbool.h"
#include "global/resourcepool.h"
#include "utils/nodeutils.h"
#include "utils/templateutils.h"

namespace cash_flow {

Model::Model(const QStringList& header, QObject* parent)
    : QAbstractItemModel(parent)
    , header_ { header }
{
    InitFixedNodes();
}

Model::~Model()
{
    ResourcePool<Row>::Instance().Recycle(root_);
    ResourcePool<Row>::Instance().Recycle(node_list_);
    ResourcePool<Row>::Instance().Recycle(first_level_nodes_);
    ResourcePool<Row>::Instance().Recycle(second_level_nodes_);
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
    case RowField::kFinalTotal:
        return node->final_total;
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
        case RowField::kFinalTotal:
            return utils::CompareMember(lhs, rhs, &Row::final_total, order);
        }
    };

    emit layoutAboutToBeChanged();
    node::SortSubtree(root_, Compare);
    emit layoutChanged();
}

void Model::Rebuild(const QJsonArray& node_array)
{
    if (node_array.isEmpty()) {
        qDebug() << Q_FUNC_INFO << "Received empty node array";
    }

    QList<Row*> new_list {};
    new_list.reserve(node_array.size());

    for (const auto& value : node_array) {
        Q_ASSERT(value.isObject());

        auto* node { ResourcePool<Row>::Instance().Allocate() };
        node->ReadJson(value.toObject());

        new_list.emplaceBack(node);
    }

    beginResetModel();

    ResourcePool<Row>::Instance().Recycle(node_list_);

    for (auto* node : std::as_const(second_level_nodes_)) {
        node->children.clear();
    }

    node_list_ = std::move(new_list);

    BuildHierarchy();
    node::SortSubtree(root_, [](const auto* lhs, const auto* rhs) { return utils::CompareMember(lhs, rhs, &Row::name, Qt::AscendingOrder); });

    for (auto* node : std::as_const(node_list_)) {
        UpdateAncestorTotal(node, node->final_total);
    }

    endResetModel();
}

Row* Model::GetNodeByIndex(const QModelIndex& index) const
{
    if (index.isValid() && index.internalPointer())
        return static_cast<Row*>(index.internalPointer());

    return root_;
}

void Model::InitFixedNodes()
{
    root_ = CreateBranchNode(QString(), finance::CashKind::kNone, direction_rule::kDICD);

    auto* operating { CreateBranchNode(tr("Operating"), finance::CashKind::kNone, direction_rule::kDDCI) };
    auto* investing { CreateBranchNode(tr("Investing"), finance::CashKind::kNone, direction_rule::kDDCI) };
    auto* financing { CreateBranchNode(tr("Financing"), finance::CashKind::kNone, direction_rule::kDDCI) };

    {
        root_->children = {
            operating,
            investing,
            financing,
        };

        operating->parent = root_;
        investing->parent = root_;
        financing->parent = root_;

        first_level_nodes_.emplaceBack(operating);
        first_level_nodes_.emplaceBack(investing);
        first_level_nodes_.emplaceBack(financing);
    }

    {
        operating_in_ = CreateBranchNode(tr("Inflows"), finance::CashKind::kNone, direction_rule::kDDCI);
        operating_out_ = CreateBranchNode(tr("Outflows"), finance::CashKind::kNone, direction_rule::kDICD);
        investing_in_ = CreateBranchNode(tr("Inflows"), finance::CashKind::kNone, direction_rule::kDDCI);
        investing_out_ = CreateBranchNode(tr("Outflows"), finance::CashKind::kNone, direction_rule::kDICD);
        financing_in_ = CreateBranchNode(tr("Inflows"), finance::CashKind::kNone, direction_rule::kDDCI);
        financing_out_ = CreateBranchNode(tr("Outflows"), finance::CashKind::kNone, direction_rule::kDICD);

        operating->children = { operating_in_, operating_out_ };
        operating_in_->parent = operating;
        operating_out_->parent = operating;

        investing->children = { investing_in_, investing_out_ };
        investing_in_->parent = investing;
        investing_out_->parent = investing;

        financing->children = { financing_in_, financing_out_ };
        financing_in_->parent = financing;
        financing_out_->parent = financing;

        second_level_nodes_.emplaceBack(operating_in_);
        second_level_nodes_.emplaceBack(operating_out_);
        second_level_nodes_.emplaceBack(investing_in_);
        second_level_nodes_.emplaceBack(investing_out_);
        second_level_nodes_.emplaceBack(financing_in_);
        second_level_nodes_.emplaceBack(financing_out_);
    }
}

void Model::BuildHierarchy() const
{
    // Attach nodes without parent to virtual root
    for (auto* node : std::as_const(node_list_)) {
        if (!node->parent) {
            auto* root { FindNodeGroup(node->cash_kind) };

            if (!root) {
                qWarning() << Q_FUNC_INFO << "Cannot find group node for cash kind:" << std::to_underlying(node->cash_kind);
                continue;
            }

            root->children.emplaceBack(node);
            node->parent = root;

            node->name = CashKindName(node->cash_kind);
        }
    }
}

Row* Model::FindNodeGroup(finance::CashKind kind) const
{
    switch (static_cast<int>(kind)) {
    // Operating
    case 100 ... 199:
        return operating_in_;
    case 200 ... 299:
        return operating_out_;

        // Investing
    case 300 ... 399:
        return investing_in_;
    case 400 ... 499:
        return investing_out_;

        // Financing
    case 500 ... 599:
        return financing_in_;
    case 600 ... 699:
        return financing_out_;

    default:
        return nullptr;
    }
}

void Model::UpdateAncestorTotal(Row* node, double final_delta) const
{
    if (!node || node == root_ || !node->parent || node->parent == root_)
        return;

    if (qFuzzyIsNull(final_delta))
        return;

    const bool rule { node->direction_rule };

    for (auto* current = node->parent; current && current != root_; current = current->parent) {
        const int multiplier { current->direction_rule == rule ? 1 : -1 };

        current->final_total += multiplier * final_delta;
    }
}

Row* Model::CreateBranchNode(const QString& name, finance::CashKind cash_kind, bool direction_rule) const
{
    auto* node { ResourcePool<Row>::Instance().Allocate() };

    node->id = QUuid::createUuidV7();

    node->name = name;
    node->cash_kind = cash_kind;
    node->direction_rule = direction_rule;

    return node;
}
}
