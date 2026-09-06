#include "searchnodemodelp.h"

namespace search {

NodeModelP::NodeModelP(CSectionInfo& info, CTreeModel* tree_model, const QHash<QUuid, tag::Row*>& tag_hash, QObject* parent)
    : NodeModel { info, tree_model, tag_hash, parent }
{
}

QVariant NodeModelP::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    const NodeEnumP column { index.column() };
    auto* d_node { static_cast<NodeP*>(index.internalPointer()) };

    switch (column) {
    case NodeEnumP::kName:
        return d_node->name;
    case NodeEnumP::kTag:
        return d_node->tag;
    case NodeEnumP::kCode:
        return d_node->code;
    case NodeEnumP::kColor:
        return d_node->color;
    case NodeEnumP::kDocument:
        return d_node->document;
    case NodeEnumP::kDescription:
        return d_node->description;
    case NodeEnumP::kKind:
        return std::to_underlying(d_node->kind);
    case NodeEnumP::kUnit:
        return std::to_underlying(d_node->unit);
    case NodeEnumP::kInitialTotal:
        return d_node->initial_total;
    case NodeEnumP::kStatus:
        return std::to_underlying(d_node->status);
    }
}

void NodeModelP::sort(int column, Qt::SortOrder order)
{
    const NodeEnumP e_column { column };

    auto Compare = [e_column, order](const Node* lhs, const Node* rhs) -> bool {
        auto* d_lhs { DerivedPtr<NodeP>(lhs) };
        auto* d_rhs { DerivedPtr<NodeP>(rhs) };

        switch (e_column) {
        case NodeEnumP::kName:
            return utils::CompareMember(lhs, rhs, &Node::name, order);
        case NodeEnumP::kCode:
            return utils::CompareMember(lhs, rhs, &Node::code, order);
        case NodeEnumP::kDescription:
            return utils::CompareMember(lhs, rhs, &Node::description, order);
        case NodeEnumP::kTag:
            return utils::CompareMember(lhs, rhs, &Node::tag, order);
        case NodeEnumP::kKind:
            return utils::CompareMember(lhs, rhs, &Node::kind, order);
        case NodeEnumP::kUnit:
            return utils::CompareMember(lhs, rhs, &Node::unit, order);
        case NodeEnumP::kColor:
            return utils::CompareMember(lhs, rhs, &Node::color, order);
        case NodeEnumP::kInitialTotal:
            return utils::CompareMember(lhs, rhs, &Node::initial_total, order);
        case NodeEnumP::kDocument:
            return (order == Qt::AscendingOrder) ? (d_lhs->document.size() < d_rhs->document.size()) : (d_lhs->document.size() > d_rhs->document.size());
        case NodeEnumP::kStatus:
            return utils::CompareMember(lhs, rhs, &Node::status, order);
        }
    };

    emit layoutAboutToBeChanged();
    std::ranges::sort(list_, Compare);
    emit layoutChanged();
}
}
