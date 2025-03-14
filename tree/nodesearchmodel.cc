#include "nodesearchmodel.h"

#include "component/enumclass.h"
#include "database/sqlite/sqliteorder.h"
#include "tree/model/nodemodels.h"

NodeSearchModel::NodeSearchModel(CInfo& info, CNodeModel* tree_model, CNodeModel* stakeholder_tree_model, Sqlite* sql, QObject* parent)
    : QAbstractItemModel { parent }
    , sql_ { sql }
    , info_ { info }
    , tree_model_ { tree_model }
    , stakeholder_tree_model_ { stakeholder_tree_model }
{
}

QModelIndex NodeSearchModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex NodeSearchModel::parent(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

int NodeSearchModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return node_list_.size();
}

int NodeSearchModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return info_.search_node_header.size();
}

QVariant NodeSearchModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto* node { node_list_.at(index.row()) };
    const NodeSearchEnum kColumn { index.column() };

    switch (kColumn) {
    case NodeSearchEnum::kName:
        return node->name;
    case NodeSearchEnum::kID:
        return node->id;
    case NodeSearchEnum::kCode:
        return node->code;
    case NodeSearchEnum::kDescription:
        return node->description;
    case NodeSearchEnum::kNote:
        return node->note;
    case NodeSearchEnum::kRule:
        return node->rule;
    case NodeSearchEnum::kType:
        return node->type;
    case NodeSearchEnum::kUnit:
        return node->unit;
    case NodeSearchEnum::kParty:
        return node->party == 0 ? QVariant() : node->party;
    case NodeSearchEnum::kEmployee:
        return node->employee == 0 ? QVariant() : node->employee;
    case NodeSearchEnum::kDateTime:
        return node->date_time;
    case NodeSearchEnum::kColor:
        return node->color;
    case NodeSearchEnum::kDocument:
        return node->document.isEmpty() ? QVariant() : node->document.size();
    case NodeSearchEnum::kFirst:
        return node->first == 0 ? QVariant() : node->first;
    case NodeSearchEnum::kSecond:
        return node->second == 0 ? QVariant() : node->second;
    case NodeSearchEnum::kDiscount:
        return node->discount == 0 ? QVariant() : node->discount;
    case NodeSearchEnum::kFinished:
        return node->finished ? node->finished : QVariant();
    case NodeSearchEnum::kInitialTotal:
        return node->initial_total == 0 ? QVariant() : node->initial_total;
    case NodeSearchEnum::kFinalTotal:
        return node->final_total == 0 ? QVariant() : node->final_total;
    default:
        return QVariant();
    }
}

QVariant NodeSearchModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return info_.search_node_header.at(section);

    return QVariant();
}

void NodeSearchModel::sort(int column, Qt::SortOrder order)
{
    if (column <= -1 || column >= info_.search_node_header.size())
        return;

    auto Compare = [column, order](const Node* lhs, const Node* rhs) -> bool {
        const NodeSearchEnum kColumn { column };

        switch (kColumn) {
        case NodeSearchEnum::kName:
            return (order == Qt::AscendingOrder) ? (lhs->name < rhs->name) : (lhs->name > rhs->name);
        case NodeSearchEnum::kCode:
            return (order == Qt::AscendingOrder) ? (lhs->code < rhs->code) : (lhs->code > rhs->code);
        case NodeSearchEnum::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case NodeSearchEnum::kNote:
            return (order == Qt::AscendingOrder) ? (lhs->note < rhs->note) : (lhs->note > rhs->note);
        case NodeSearchEnum::kRule:
            return (order == Qt::AscendingOrder) ? (lhs->rule < rhs->rule) : (lhs->rule > rhs->rule);
        case NodeSearchEnum::kType:
            return (order == Qt::AscendingOrder) ? (lhs->type < rhs->type) : (lhs->type > rhs->type);
        case NodeSearchEnum::kUnit:
            return (order == Qt::AscendingOrder) ? (lhs->unit < rhs->unit) : (lhs->unit > rhs->unit);
        case NodeSearchEnum::kParty:
            return (order == Qt::AscendingOrder) ? (lhs->party < rhs->party) : (lhs->party > rhs->party);
        case NodeSearchEnum::kEmployee:
            return (order == Qt::AscendingOrder) ? (lhs->employee < rhs->employee) : (lhs->employee > rhs->employee);
        case NodeSearchEnum::kDateTime:
            return (order == Qt::AscendingOrder) ? (lhs->date_time < rhs->date_time) : (lhs->date_time > rhs->date_time);
        case NodeSearchEnum::kColor:
            return (order == Qt::AscendingOrder) ? (lhs->color < rhs->color) : (lhs->color > rhs->color);
        case NodeSearchEnum::kDocument:
            return (order == Qt::AscendingOrder) ? (lhs->document.size() < rhs->document.size()) : (lhs->document.size() > rhs->document.size());
        case NodeSearchEnum::kFirst:
            return (order == Qt::AscendingOrder) ? (lhs->first < rhs->first) : (lhs->first > rhs->first);
        case NodeSearchEnum::kSecond:
            return (order == Qt::AscendingOrder) ? (lhs->second < rhs->second) : (lhs->second > rhs->second);
        case NodeSearchEnum::kDiscount:
            return (order == Qt::AscendingOrder) ? (lhs->discount < rhs->discount) : (lhs->discount > rhs->discount);
        case NodeSearchEnum::kFinished:
            return (order == Qt::AscendingOrder) ? (lhs->finished < rhs->finished) : (lhs->finished > rhs->finished);
        case NodeSearchEnum::kFinalTotal:
            return (order == Qt::AscendingOrder) ? (lhs->final_total < rhs->final_total) : (lhs->final_total > rhs->final_total);
        case NodeSearchEnum::kInitialTotal:
            return (order == Qt::AscendingOrder) ? (lhs->initial_total < rhs->initial_total) : (lhs->initial_total > rhs->initial_total);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(node_list_.begin(), node_list_.end(), Compare);
    emit layoutChanged();
}

void NodeSearchModel::Query(const QString& text)
{
    node_list_.clear();
    auto* stakeholder_tree { static_cast<const NodeModelS*>(stakeholder_tree_model_) };

    beginResetModel();
    switch (info_.section) {
    case Section::kSales:
        static_cast<SqliteOrder*>(sql_)->SearchNode(node_list_, stakeholder_tree->PartyList(text, std::to_underlying(UnitS::kCust)));
        break;
    case Section::kPurchase:
        static_cast<SqliteOrder*>(sql_)->SearchNode(node_list_, stakeholder_tree->PartyList(text, std::to_underlying(UnitS::kVend)));
        break;
    case Section::kFinance:
    case Section::kProduct:
    case Section::kTask:
    case Section::kStakeholder:
        tree_model_->SearchNode(node_list_, sql_->SearchNodeName(text));
        break;
    default:
        break;
    }

    endResetModel();
}
