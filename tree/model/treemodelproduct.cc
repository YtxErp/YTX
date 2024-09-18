#include "treemodelproduct.h"

#include <QMimeData>
#include <QQueue>
#include <QRegularExpression>

#include "component/enumclass.h"

TreeModelProduct::TreeModelProduct(SPSqlite sql, CInfo& info, int base_unit, CTableHash& table_hash, CString& separator, QObject* parent)
    : AbstractTreeModel { sql, info, base_unit, table_hash, separator, parent }
{
    ConstructTree();
}

void TreeModelProduct::UpdateNode(const Node* tmp_node)
{
    if (!tmp_node)
        return;

    auto node { const_cast<Node*>(GetNodeByID(tmp_node->id)) };
    if (*node == *tmp_node)
        return;

    UpdateNodeRule(node, tmp_node->node_rule);
    UpdateUnit(node, tmp_node->unit);
    UpdateBranch(node, tmp_node->branch);

    if (node->name != tmp_node->name) {
        UpdateName(node, tmp_node->name);
        emit SUpdateName(node);
    }

    // update code, description, note, unit_price, commission
    *node = *tmp_node;
    sql_->UpdateNodeSimple(node);
}

bool TreeModelProduct::UpdateUnitPrice(Node* node, double value, CString& field) { return UpdateField(node, value, field, &Node::discount); }

bool TreeModelProduct::UpdateCommission(Node* node, double value, CString& field) { return UpdateField(node, value, field, &Node::second); }

void TreeModelProduct::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.tree_header.size())
        return;

    auto Compare = [column, order](const Node* lhs, const Node* rhs) -> bool {
        const TreeEnumProduct kColumn { column };
        switch (kColumn) {
        case TreeEnumProduct::kName:
            return (order == Qt::AscendingOrder) ? (lhs->name < rhs->name) : (lhs->name > rhs->name);
        case TreeEnumProduct::kCode:
            return (order == Qt::AscendingOrder) ? (lhs->code < rhs->code) : (lhs->code > rhs->code);
        case TreeEnumProduct::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case TreeEnumProduct::kNote:
            return (order == Qt::AscendingOrder) ? (lhs->note < rhs->note) : (lhs->note > rhs->note);
        case TreeEnumProduct::kNodeRule:
            return (order == Qt::AscendingOrder) ? (lhs->node_rule < rhs->node_rule) : (lhs->node_rule > rhs->node_rule);
        case TreeEnumProduct::kBranch:
            return (order == Qt::AscendingOrder) ? (lhs->branch < rhs->branch) : (lhs->branch > rhs->branch);
        case TreeEnumProduct::kUnit:
            return (order == Qt::AscendingOrder) ? (lhs->unit < rhs->unit) : (lhs->unit > rhs->unit);
        case TreeEnumProduct::kCommission:
            return (order == Qt::AscendingOrder) ? (lhs->second < rhs->second) : (lhs->second > rhs->second);
        case TreeEnumProduct::kUnitPrice:
            return (order == Qt::AscendingOrder) ? (lhs->discount < rhs->discount) : (lhs->discount > rhs->discount);
        case TreeEnumProduct::kInitialTotal:
            return (order == Qt::AscendingOrder) ? (lhs->initial_total < rhs->initial_total) : (lhs->initial_total > rhs->initial_total);
        case TreeEnumProduct::kFinalTotal:
            return (order == Qt::AscendingOrder) ? (lhs->final_total < rhs->final_total) : (lhs->final_total > rhs->final_total);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    SortIterative(root_, Compare);
    emit layoutChanged();
}

QVariant TreeModelProduct::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto node { GetNodeByIndex(index) };
    if (node->id == -1)
        return QVariant();

    const TreeEnumProduct kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumProduct::kName:
        return node->name;
    case TreeEnumProduct::kID:
        return node->id;
    case TreeEnumProduct::kCode:
        return node->code;
    case TreeEnumProduct::kDescription:
        return node->description;
    case TreeEnumProduct::kNote:
        return node->note;
    case TreeEnumProduct::kNodeRule:
        return node->node_rule;
    case TreeEnumProduct::kBranch:
        return node->branch;
    case TreeEnumProduct::kUnit:
        return node->unit;
    case TreeEnumProduct::kCommission:
        return node->second == 0 ? QVariant() : node->second;
    case TreeEnumProduct::kUnitPrice:
        return node->discount == 0 ? QVariant() : node->discount;
    case TreeEnumProduct::kInitialTotal:
        return node->initial_total;
    case TreeEnumProduct::kFinalTotal:
        return node->final_total;
    default:
        return QVariant();
    }
}

bool TreeModelProduct::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    auto node { GetNodeByIndex(index) };
    if (node->id == -1)
        return false;

    const TreeEnumProduct kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumProduct::kCode:
        UpdateCode(node, value.toString());
        break;
    case TreeEnumProduct::kDescription:
        UpdateDescription(node, value.toString());
        break;
    case TreeEnumProduct::kNote:
        UpdateNote(node, value.toString());
        break;
    case TreeEnumProduct::kNodeRule:
        UpdateNodeRule(node, value.toBool());
        break;
    case TreeEnumProduct::kBranch:
        UpdateBranch(node, value.toBool());
        break;
    case TreeEnumProduct::kUnit:
        UpdateUnit(node, value.toInt());
        break;
    case TreeEnumProduct::kCommission:
        UpdateCommission(node, value.toDouble());
        break;
    case TreeEnumProduct::kUnitPrice:
        UpdateUnitPrice(node, value.toDouble());
        break;
    default:
        return false;
    }

    emit SResizeColumnToContents(index.column());
    return true;
}

Qt::ItemFlags TreeModelProduct::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const TreeEnumProduct kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumProduct::kName:
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        flags &= ~Qt::ItemIsEditable;
        break;
    case TreeEnumProduct::kInitialTotal:
    case TreeEnumProduct::kFinalTotal:
    case TreeEnumProduct::kBranch:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}
