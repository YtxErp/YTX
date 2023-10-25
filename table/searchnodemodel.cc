#include "searchnodemodel.h"

#include "component/enumclass.h"
#include "database/sqlite/sqliteorder.h"
#include "tree/model/treemodelstakeholder.h"

SearchNodeModel::SearchNodeModel(CInfo& info, CTreeModel* tree_model, CTreeModel* stakeholder_tree_model, Sqlite* sql, QObject* parent)
    : QAbstractItemModel { parent }
    , sql_ { sql }
    , info_ { info }
    , tree_model_ { tree_model }
    , stakeholder_tree_model_ { stakeholder_tree_model }
{
}

QModelIndex SearchNodeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex SearchNodeModel::parent(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

int SearchNodeModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return node_list_.size();
}

int SearchNodeModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return info_.search_node_header.size();
}

QVariant SearchNodeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* node { node_list_.at(index.row()) };
    const TreeEnumSearch kColumn { index.column() };

    switch (kColumn) {
    case TreeEnumSearch::kName:
        return node->name;
    case TreeEnumSearch::kID:
        return node->id;
    case TreeEnumSearch::kCode:
        return node->code;
    case TreeEnumSearch::kDescription:
        return node->description;
    case TreeEnumSearch::kNote:
        return node->note;
    case TreeEnumSearch::kRule:
        return node->rule;
    case TreeEnumSearch::kType:
        return node->type;
    case TreeEnumSearch::kUnit:
        return node->unit;
    case TreeEnumSearch::kParty:
        return node->party == 0 ? QVariant() : node->party;
    case TreeEnumSearch::kEmployee:
        return node->employee == 0 ? QVariant() : node->employee;
    case TreeEnumSearch::kDateTime:
        return node->date_time;
    case TreeEnumSearch::kColor:
        return node->color;
    case TreeEnumSearch::kDocument:
        return node->document.isEmpty() ? QVariant() : node->document.size();
    case TreeEnumSearch::kFirst:
        return node->first == 0 ? QVariant() : node->first;
    case TreeEnumSearch::kSecond:
        return node->second == 0 ? QVariant() : node->second;
    case TreeEnumSearch::kDiscount:
        return node->discount == 0 ? QVariant() : node->discount;
    case TreeEnumSearch::kFinished:
        return node->finished ? node->finished : QVariant();
    case TreeEnumSearch::kInitialTotal:
        return node->initial_total == 0 ? QVariant() : node->initial_total;
    case TreeEnumSearch::kFinalTotal:
        return node->final_total == 0 ? QVariant() : node->final_total;
    default:
        return QVariant();
    }
}

QVariant SearchNodeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return info_.search_node_header.at(section);

    return QVariant();
}

void SearchNodeModel::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.search_node_header.size())
        return;

    auto Compare = [column, order](const Node* lhs, const Node* rhs) -> bool {
        const TreeEnumSearch kColumn { column };

        switch (kColumn) {
        case TreeEnumSearch::kName:
            return (order == Qt::AscendingOrder) ? (lhs->name < rhs->name) : (lhs->name > rhs->name);
        case TreeEnumSearch::kCode:
            return (order == Qt::AscendingOrder) ? (lhs->code < rhs->code) : (lhs->code > rhs->code);
        case TreeEnumSearch::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case TreeEnumSearch::kNote:
            return (order == Qt::AscendingOrder) ? (lhs->note < rhs->note) : (lhs->note > rhs->note);
        case TreeEnumSearch::kRule:
            return (order == Qt::AscendingOrder) ? (lhs->rule < rhs->rule) : (lhs->rule > rhs->rule);
        case TreeEnumSearch::kType:
            return (order == Qt::AscendingOrder) ? (lhs->type < rhs->type) : (lhs->type > rhs->type);
        case TreeEnumSearch::kUnit:
            return (order == Qt::AscendingOrder) ? (lhs->unit < rhs->unit) : (lhs->unit > rhs->unit);
        case TreeEnumSearch::kParty:
            return (order == Qt::AscendingOrder) ? (lhs->party < rhs->party) : (lhs->party > rhs->party);
        case TreeEnumSearch::kEmployee:
            return (order == Qt::AscendingOrder) ? (lhs->employee < rhs->employee) : (lhs->employee > rhs->employee);
        case TreeEnumSearch::kDateTime:
            return (order == Qt::AscendingOrder) ? (lhs->date_time < rhs->date_time) : (lhs->date_time > rhs->date_time);
        case TreeEnumSearch::kColor:
            return (order == Qt::AscendingOrder) ? (lhs->color < rhs->color) : (lhs->color > rhs->color);
        case TreeEnumSearch::kDocument:
            return (order == Qt::AscendingOrder) ? (lhs->document.size() < rhs->document.size()) : (lhs->document.size() > rhs->document.size());
        case TreeEnumSearch::kFirst:
            return (order == Qt::AscendingOrder) ? (lhs->first < rhs->first) : (lhs->first > rhs->first);
        case TreeEnumSearch::kSecond:
            return (order == Qt::AscendingOrder) ? (lhs->second < rhs->second) : (lhs->second > rhs->second);
        case TreeEnumSearch::kDiscount:
            return (order == Qt::AscendingOrder) ? (lhs->discount < rhs->discount) : (lhs->discount > rhs->discount);
        case TreeEnumSearch::kFinished:
            return (order == Qt::AscendingOrder) ? (lhs->finished < rhs->finished) : (lhs->finished > rhs->finished);
        case TreeEnumSearch::kFinalTotal:
            return (order == Qt::AscendingOrder) ? (lhs->final_total < rhs->final_total) : (lhs->final_total > rhs->final_total);
        case TreeEnumSearch::kInitialTotal:
            return (order == Qt::AscendingOrder) ? (lhs->initial_total < rhs->initial_total) : (lhs->initial_total > rhs->initial_total);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(node_list_.begin(), node_list_.end(), Compare);
    emit layoutChanged();
}

void SearchNodeModel::Query(const QString& text)
{
    node_list_.clear();
    auto* stakeholder_tree { static_cast<const TreeModelStakeholder*>(stakeholder_tree_model_) };

    beginResetModel();
    switch (info_.section) {
    case Section::kSales:
        static_cast<SqliteOrder*>(sql_)->SearchNode(node_list_, stakeholder_tree->PartyList(text, kUnitCust));
        break;
    case Section::kPurchase:
        static_cast<SqliteOrder*>(sql_)->SearchNode(node_list_, stakeholder_tree->PartyList(text, kUnitVend));
        break;
    case Section::kFinance:
    case Section::kProduct:
    case Section::kTask:
    case Section::kStakeholder:
        tree_model_->SearchNodeFPTS(node_list_, sql_->SearchNodeName(text));
        break;
    default:
        break;
    }

    endResetModel();
}
