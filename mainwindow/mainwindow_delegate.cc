#include "audit/auditenum.h"
#include "audit/audittextdelegate.h"
#include "component/constantdouble.h"
#include "component/constantstring.h"
#include "dashboard/balance_sheet/balancesheetenum.h"
#include "dashboard/cash_flow_statement/cashflowstatementenum.h"
#include "dashboard/income_statement/incomestatementenum.h"
#include "dashboard/inventory_heat/inventoryheatenum.h"
#include "dashboard/partner_heat/partnerheatenum.h"
#include "delegate/boolstring.h"
#include "delegate/color.h"
#include "delegate/document.h"
#include "delegate/double.h"
#include "delegate/filterunit.h"
#include "delegate/financeroledelegate.h"
#include "delegate/intstring.h"
#include "delegate/issuedtime.h"
#include "delegate/line.h"
#include "delegate/readonly/amountorderhistoryr.h"
#include "delegate/readonly/amountr.h"
#include "delegate/readonly/boolcolorstringr.h"
#include "delegate/readonly/boolstringr.h"
#include "delegate/readonly/cashflownamer.h"
#include "delegate/readonly/colorr.h"
#include "delegate/readonly/doublenonedecimalr.h"
#include "delegate/readonly/doublenonezeror.h"
#include "delegate/readonly/doubler.h"
#include "delegate/readonly/financeforeignr.h"
#include "delegate/readonly/intstringnonezeror.h"
#include "delegate/readonly/intstringr.h"
#include "delegate/readonly/issuedtimer.h"
#include "delegate/readonly/nodenamer.h"
#include "delegate/readonly/nodepathr.h"
#include "delegate/readonly/percentagedelegater.h"
#include "delegate/readonly/statusr.h"
#include "delegate/rhsnode.h"
#include "delegate/search/searchpathtabler.h"
#include "delegate/sectionpermissionsdelegate.h"
#include "delegate/statusdelegate.h"
#include "delegate/statustestdelegate.h"
#include "delegate/tagdelegate.h"
#include "delegate/workspaceroledelegate.h"
#include "finance/settlement/settlementenum.h"
#include "finance/statement/statementenum.h"
#include "history/orderenum.h"
#include "mainwindow.h"
#include "tag/tagenum.h"
#include "workspace/workspaceenum.h"

void MainWindow::TreeDelegateF(QTreeView* tree_view, CSectionInfo& info, CSectionConfig& section) const
{
    auto* line { new Line(tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumF::kCode), line);
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumF::kDescription), line);

    auto* direction_rule { new BoolString(info.rule_map, QEvent::MouseButtonDblClick, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumF::kDirectionRule), direction_rule);

    auto* unit { new IntStringR(info.unit_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumF::kUnit), unit);

    auto* kind { new IntStringR(info.kind_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumF::kKind), kind);

    auto* color { new Color(tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumF::kColor), color);

    auto* final_total { new AmountR(section.amount_decimal, sc_f_.shared_config.default_unit, info.unit_symbol_map, string_const::kEightDigits, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumF::kFinalTotal), final_total);

    auto* initial_total { new FinanceForeignR(
        section.amount_decimal, sc_f_.shared_config.default_unit, info.unit_symbol_map, string_const::kEightDigits, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumF::kInitialTotal), initial_total);

    auto* tag { new TagDelegate(sc_f_.tag_icon_hash, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumF::kTag), tag);

    auto* document { new Document(sc_f_.shared_config.document_dir, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumF::kDocument), document);

    auto* roles { new FinanceRoleDelegate(finance::RoleItemList(), tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumF::kRoles), roles);
}

void MainWindow::TreeDelegateT(QTreeView* tree_view, CSectionInfo& info, CSectionConfig& section) const
{
    auto* line { new Line(tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumT::kCode), line);
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumT::kDescription), line);

    auto* direction_rule { new BoolString(info.rule_map, QEvent::MouseButtonDblClick, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumT::kDirectionRule), direction_rule);

    auto* unit { new IntStringR(info.unit_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumT::kUnit), unit);

    auto* kind { new IntStringR(info.kind_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumT::kKind), kind);

    auto* quantity { new DoubleR(section.quantity_decimal, string_const::kEightDigits, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumT::kInitialTotal), quantity);

    auto* amount { new AmountR(section.amount_decimal, sc_f_.shared_config.default_unit, sc_f_.info.unit_symbol_map, string_const::kEightDigits, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumT::kFinalTotal), amount);

    auto* color { new Color(tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumT::kColor), color);

    auto* document { new Document(sc_t_.shared_config.document_dir, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumT::kDocument), document);

    auto* tag { new TagDelegate(sc_t_.tag_icon_hash, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumT::kTag), tag);
}

void MainWindow::TreeDelegateI(QTreeView* tree_view, CSectionInfo& info, CSectionConfig& section) const
{
    auto* line { new Line(tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumI::kCode), line);
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumI::kDescription), line);

    auto* direction_rule { new BoolString(info.rule_map, QEvent::MouseButtonDblClick, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumI::kDirectionRule), direction_rule);

    auto* unit { new IntStringR(info.unit_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumI::kUnit), unit);

    auto* kind { new IntStringR(info.kind_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumI::kKind), kind);

    auto* quantity { new DoubleR(section.quantity_decimal, string_const::kEightDigits, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumI::kInitialTotal), quantity);

    auto* amount { new AmountOrderHistoryR(
        info.section, section.amount_decimal, sc_f_.shared_config.default_unit, sc_f_.info.unit_symbol_map, string_const::kEightDigits, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumI::kFinalTotal), amount);
    connect(amount, &AmountOrderHistoryR::SShowOrderHistoryWidget, this, &MainWindow::RShowOrderHistoryWidget);

    auto* unit_price { new Double(section.rate_decimal, 0.0, kDoubleMax, string_const::kFourDigits, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumI::kUnitPrice), unit_price);
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumI::kCommission), unit_price);

    auto* color { new Color(tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumI::kColor), color);

    auto* tag { new TagDelegate(sc_i_.tag_icon_hash, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumI::kTag), tag);

    auto* document { new Document(sc_i_.shared_config.document_dir, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumI::kDocument), document);
}

void MainWindow::TreeDelegateP(QTreeView* tree_view, CSectionInfo& info, CSectionConfig& section) const
{
    auto* status { new StatusTextDelegate(
        info.status_map, QEvent::MouseButtonDblClick, std::to_underlying(NodeStatus::kInactive), std::to_underlying(NodeStatus::kActive), tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kStatus), status);

    auto* color { new Color(tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kColor), color);

    auto* line { new Line(tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kCode), line);
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kDescription), line);

    auto* unit { new IntStringR(info.unit_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kUnit), unit);

    auto* kind { new IntStringR(info.kind_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kKind), kind);

    auto* amount { new AmountOrderHistoryR(
        info.section, section.amount_decimal, sc_f_.shared_config.default_unit, sc_f_.info.unit_symbol_map, string_const::kEightDigits, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kInitialTotal), amount);
    connect(amount, &AmountOrderHistoryR::SShowOrderHistoryWidget, this, &MainWindow::RShowOrderHistoryWidget);

    auto* tag { new TagDelegate(sc_p_.tag_icon_hash, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kTag), tag);

    auto* document { new Document(sc_p_.shared_config.document_dir, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kDocument), document);
}

void MainWindow::TreeDelegateO(QTreeView* tree_view, CSectionInfo& info, CSectionConfig& section) const
{
    auto* direction_rule_r { new BoolColorStringR(info.rule_map, kWarningColor, QString(), tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kDirectionRule), direction_rule_r);

    auto* unit_r { new IntStringR(info.unit_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kUnit), unit_r);

    auto* kind_r { new IntStringR(info.kind_map, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kKind), kind_r);

    auto* amount { new AmountR(section.amount_decimal, sc_f_.shared_config.default_unit, sc_f_.info.unit_symbol_map, string_const::kEightDigits, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kInitialTotal), amount);
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kFinalTotal), amount);
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kDiscountTotal), amount);

    auto* quantity_r { new DoubleNoneZeroR(section.quantity_decimal, string_const::kFourDigits, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kMeasureTotal), quantity_r);
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kCountTotal), quantity_r);

    auto* name_r { new NodeNameR(sc_p_.tree_model, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kEmployeeId), name_r);

    auto* issued_time { new IssuedTimeR(section.date_format, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kIssuedTime), issued_time);

    auto* status_r { new StatusR(std::to_underlying(OrderStatus::kReleased), tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kStatus), status_r);

    const auto& sc { info.section == Section::kSale ? sc_sale_ : sc_purchase_ };
    auto* tag { new TagDelegate(sc.tag_icon_hash, tree_view) };
    tree_view->setItemDelegateForColumn(std::to_underlying(NodeEnumO::kTag), tag);
}

void MainWindow::TableDelegateF(QTableView* table_view, TreeModel* tree_model, CSectionConfig& config, const QUuid& node_id) const
{
    auto* issued_time { new IssuedTime(config.date_format, true, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumF::kIssuedTime), issued_time);

    auto* lhs_rate { new Double(config.rate_decimal, 0.0, kDoubleMax, string_const::kFourDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumF::kLhsRate), lhs_rate);

    auto* line { new Line(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumF::kCode), line);
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumF::kDescription), line);

    auto* document { new Document(sc_->shared_config.document_dir, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumF::kDocument), document);

    auto* status { new StatusDelegate(
        QEvent::MouseButtonRelease, std::to_underlying(EntryStatus::kUnmarked), std::to_underlying(EntryStatus::kMarked), table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumF::kStatus), status);

    QSortFilterProxyModel* filter_model { tree_model->ExcludeId(node_id, table_view) };

    auto* node { new RhsNode(tree_model, filter_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumF::kRhsNode), node);

    auto* value { new Double(config.quantity_decimal, kDoubleLowest, kDoubleMax, string_const::kFourDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumF::kDebit), value);
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumF::kCredit), value);

    auto* balance { new DoubleR(config.quantity_decimal, string_const::kEightDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumF::kBalance), balance);

    auto* tag { new TagDelegate(sc_f_.tag_icon_hash, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumF::kTag), tag);

    const auto& info { sc_f_.info };
    auto* cash_kind { new IntString(info.cash_kind_model, info.cash_kind_map, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumF::kCashKind), cash_kind);
}

void MainWindow::TableDelegateI(QTableView* table_view, TreeModel* tree_model, CSectionConfig& config, const QUuid& node_id) const
{
    auto* issued_time { new IssuedTime(config.date_format, false, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnum::kIssuedTime), issued_time);

    auto* unit_cost { new Double(config.rate_decimal, 0.0, kDoubleMax, string_const::kFourDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnum::kLhsRate), unit_cost);

    auto* line { new Line(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnum::kCode), line);
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnum::kDescription), line);

    auto* document { new Document(sc_->shared_config.document_dir, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnum::kDocument), document);

    auto* status { new StatusDelegate(
        QEvent::MouseButtonRelease, std::to_underlying(EntryStatus::kUnmarked), std::to_underlying(EntryStatus::kMarked), table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnum::kStatus), status);

    QSortFilterProxyModel* filter_model { tree_model->ExcludeId(node_id, table_view) };

    auto* node { new RhsNode(tree_model, filter_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnum::kRhsNode), node);

    auto* value { new Double(config.quantity_decimal, kDoubleLowest, kDoubleMax, string_const::kFourDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnum::kDebit), value);
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnum::kCredit), value);

    auto* balance { new DoubleR(config.quantity_decimal, string_const::kEightDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnum::kBalance), balance);

    auto* tag { new TagDelegate(sc_i_.tag_icon_hash, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnum::kTag), tag);
}

void MainWindow::TableDelegateT(QTableView* table_view, TreeModel* tree_model, CSectionConfig& config, const QUuid& node_id) const
{
    auto* issued_time { new IssuedTime(config.date_format, false, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnum::kIssuedTime), issued_time);

    auto* unit_cost { new Double(config.rate_decimal, 0.0, kDoubleMax, string_const::kFourDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnum::kLhsRate), unit_cost);

    auto* line { new Line(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnum::kCode), line);
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnum::kDescription), line);

    auto* document { new Document(sc_->shared_config.document_dir, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnum::kDocument), document);

    auto* status { new StatusDelegate(
        QEvent::MouseButtonRelease, std::to_underlying(EntryStatus::kUnmarked), std::to_underlying(EntryStatus::kMarked), table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnum::kStatus), status);

    QSortFilterProxyModel* filter_model { tree_model->ExcludeId(node_id, table_view) };

    auto* node { new RhsNode(tree_model, filter_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnum::kRhsNode), node);

    auto* value { new Double(config.quantity_decimal, kDoubleLowest, kDoubleMax, string_const::kFourDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnum::kDebit), value);
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnum::kCredit), value);

    auto* balance { new DoubleR(config.quantity_decimal, string_const::kEightDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnum::kBalance), balance);

    auto* tag { new TagDelegate(sc_t_.tag_icon_hash, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnum::kTag), tag);
}

void MainWindow::TableDelegateP(QTableView* table_view, CSectionConfig& config) const
{
    auto* issued_time { new IssuedTime(config.date_format, false, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumP::kIssuedTime), issued_time);

    auto* unit_price { new Double(config.rate_decimal, 0.0, kDoubleMax, string_const::kFourDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumP::kUnitPrice), unit_price);

    auto* line { new Line(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumP::kCode), line);
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumP::kDescription), line);
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumP::kExternalSku), line);

    auto* document { new Document(sc_->shared_config.document_dir, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumP::kDocument), document);

    auto* status { new StatusDelegate(
        QEvent::MouseButtonRelease, std::to_underlying(EntryStatus::kUnmarked), std::to_underlying(EntryStatus::kMarked), table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumP::kStatus), status);

    auto tree_model_i { sc_i_.tree_model };
    auto* itm_filter_model { tree_model_i->IncludeUnit(NodeUnit::IItem, table_view) };
    auto* internal_sku { new FilterUnit(tree_model_i, itm_filter_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumP::kRhsNode), internal_sku);

    auto* tag { new TagDelegate(sc_p_.tag_icon_hash, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumP::kTag), tag);
}

void MainWindow::TableDelegateO(QTableView* table_view, CSectionInfo& info, CSectionConfig& config) const
{
    auto tree_model_i { sc_i_.tree_model };
    auto* itm_filter_model { tree_model_i->IncludeUnit(NodeUnit::IItem, table_view) };

    auto* internal_sku { new FilterUnit(tree_model_i, itm_filter_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumO::kRhsNode), internal_sku);

    auto* line { new Line(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumO::kDescription), line);

    auto* price { new Double(config.rate_decimal, 0.0, kDoubleMax, string_const::kFourDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumO::kUnitPrice), price);
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumO::kUnitDiscount), price);

    auto* quantity { new Double(config.quantity_decimal, kDoubleLowest, kDoubleMax, string_const::kFourDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumO::kCount), quantity);
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumO::kMeasure), quantity);

    auto* amount { new DoubleNoneZeroR(config.amount_decimal, string_const::kEightDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumO::kInitial), amount);
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumO::kDiscount), amount);
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumO::kFinal), amount);

    const auto& sc { info.section == Section::kSale ? sc_sale_ : sc_purchase_ };
    auto* tag { new TagDelegate(sc.tag_icon_hash, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumO::kTag), tag);

    auto* status { new StatusDelegate(
        QEvent::MouseButtonRelease, std::to_underlying(EntryStatus::kUnmarked), std::to_underlying(EntryStatus::kMarked), table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(EntryEnumO::kStatus), status);
}

void MainWindow::DelegateSalesHistoryI(QTableView* table_view, CSectionConfig& config) const
{
    auto* price { new DoubleNoneZeroR(config.rate_decimal, string_const::kFourDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(history::SalesFieldI::kUnitPrice), price);

    auto* quantity { new DoubleNoneZeroR(config.quantity_decimal, string_const::kFourDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(history::SalesFieldI::kCount), quantity);
    table_view->setItemDelegateForColumn(std::to_underlying(history::SalesFieldI::kMeasure), quantity);

    auto* amount { new DoubleNoneZeroR(config.amount_decimal, string_const::kEightDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(history::SalesFieldI::kInitial), amount);

    auto* issued_time { new IssuedTimeR(config.date_format, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(history::SalesFieldI::kIssuedTime), issued_time);

    auto* name { new NodeNameR(sc_p_.tree_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(history::SalesFieldI::kPartnerId), name);
}

void MainWindow::DelegateOrderHistoryP(QTableView* table_view, CSectionConfig& config) const
{
    auto* price { new DoubleNoneZeroR(config.rate_decimal, string_const::kFourDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(history::OrderFieldP::kUnitPrice), price);

    auto* quantity { new DoubleNoneZeroR(config.quantity_decimal, string_const::kFourDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(history::OrderFieldP::kCount), quantity);
    table_view->setItemDelegateForColumn(std::to_underlying(history::OrderFieldP::kMeasure), quantity);

    auto* amount { new DoubleNoneZeroR(config.amount_decimal, string_const::kEightDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(history::OrderFieldP::kInitial), amount);

    auto* issued_time { new IssuedTimeR(config.date_format, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(history::OrderFieldP::kIssuedTime), issued_time);

    auto* internal_sku { new NodePathR(sc_i_.tree_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(history::OrderFieldP::kInternalSku), internal_sku);

    auto* color { new ColorR(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(history::OrderFieldP::kColor), color);
}

void MainWindow::DelegateStatementPrimary(QTableView* table_view, CSectionConfig& config) const
{
    auto* quantity { new DoubleNoneZeroR(config.amount_decimal, string_const::kFourDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(statement::PrimaryField::kCount), quantity);
    table_view->setItemDelegateForColumn(std::to_underlying(statement::PrimaryField::kMeasure), quantity);

    auto* amount { new DoubleNoneZeroR(config.amount_decimal, string_const::kEightDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(statement::PrimaryField::kAmount), amount);

    auto* name { new NodeNameR(sc_p_.tree_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(statement::PrimaryField::kPartner), name);
}

void MainWindow::DelegateStatementSecondary(QTableView* table_view, CSectionConfig& config) const
{
    auto* quantity { new DoubleNoneZeroR(config.quantity_decimal, string_const::kFourDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(statement::SecondaryField::kCount), quantity);
    table_view->setItemDelegateForColumn(std::to_underlying(statement::SecondaryField::kMeasure), quantity);

    auto* amount { new DoubleNoneZeroR(config.amount_decimal, string_const::kEightDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(statement::SecondaryField::kAmount), amount);

    auto* employee { new NodeNameR(sc_p_.tree_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(statement::SecondaryField::kEmployee), employee);

    auto* status { new StatusDelegate(
        QEvent::MouseButtonRelease, std::to_underlying(EntryStatus::kUnmarked), std::to_underlying(EntryStatus::kMarked), table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(statement::SecondaryField::kStatus), status);

    auto* issued_time { new IssuedTimeR(sc_sale_.section_config.date_format, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(statement::SecondaryField::kIssuedTime), issued_time);
}

void MainWindow::DelegateStatementTertiary(QTableView* table_view, CSectionConfig& config) const
{
    auto* quantity { new DoubleNoneZeroR(config.quantity_decimal, string_const::kFourDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(statement::TertiaryField::kCount), quantity);
    table_view->setItemDelegateForColumn(std::to_underlying(statement::TertiaryField::kMeasure), quantity);

    auto* amount { new DoubleNoneZeroR(config.amount_decimal, string_const::kEightDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(statement::TertiaryField::kAmount), amount);

    auto* unit_price { new DoubleNoneZeroR(config.rate_decimal, string_const::kFourDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(statement::TertiaryField::kUnitPrice), unit_price);

    auto* status { new StatusDelegate(
        QEvent::MouseButtonRelease, std::to_underlying(EntryStatus::kUnmarked), std::to_underlying(EntryStatus::kMarked), table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(statement::TertiaryField::kStatus), status);

    auto* issued_time { new IssuedTimeR(sc_sale_.section_config.date_format, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(statement::TertiaryField::kIssuedTime), issued_time);

    auto* node_path { new NodePathR(sc_i_.tree_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(statement::TertiaryField::kInternalSku), node_path);
}

void MainWindow::DelegateSettlementPrimary(QTableView* table_view, CSectionConfig& config) const
{
    auto* amount { new DoubleNoneZeroR(config.amount_decimal, string_const::kEightDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(settlement::PrimaryField::kAmount), amount);

    auto* status_r { new StatusR(std::to_underlying(SettlementStatus::kReleased), table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(settlement::PrimaryField::kStatus), status_r);

    auto* issued_time { new IssuedTimeR(datetime_format::kDashedDate, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(settlement::PrimaryField::kIssuedTime), issued_time);

    auto* name { new NodeNameR(sc_p_.tree_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(settlement::PrimaryField::kPartner), name);
}

void MainWindow::DelegateSettlementSecondary(QTableView* table_view, CSectionConfig& config) const
{
    auto* amount { new DoubleNoneZeroR(config.amount_decimal, string_const::kEightDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(settlement::SecondaryField::kAmount), amount);

    auto* employee { new NodeNameR(sc_p_.tree_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(settlement::SecondaryField::kEmployee), employee);

    auto* status { new StatusDelegate(
        QEvent::MouseButtonRelease, std::to_underlying(SettlementStatus::kRecalled), std::to_underlying(SettlementStatus::kReleased), table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(settlement::SecondaryField::kIsSettled), status);

    auto* issued_time { new IssuedTimeR(datetime_format::kDashedDate, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(settlement::SecondaryField::kIssuedTime), issued_time);
}

void MainWindow::DelegateTag(QTableView* table_view) const
{
    auto* color { new Color(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(tag::RowField::kColor), color);

    auto* line { new Line(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(tag::RowField::kName), line);
}

void MainWindow::DelegateWorkspaceMember(QTableView* table_view) const
{
    auto* created_time { new IssuedTimeR(datetime_format::kDashedDate, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(workspace::MemberField::kCreatedTime), created_time);

    auto* workspace_role { new WorkspaceRoleDelegate(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(workspace::MemberField::kWorkspaceRole), workspace_role);

    auto* database_role { new SectionPermissionsDelegate(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(workspace::MemberField::kSectionPermissions), database_role);
}

void MainWindow::DelegateAuditLog(QTableView* table_view) const
{
    auto* created_time { new IssuedTimeR(datetime_format::kDateTime, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(audit::RowField::kCreatedTime), created_time);

    auto* audit_text { new AuditTextDelegate(table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(audit::RowField::kBefore), audit_text);
    table_view->setItemDelegateForColumn(std::to_underlying(audit::RowField::kAfter), audit_text);
}

void MainWindow::DelegatePeriodClose(QTableView* table_view) const
{
    const auto& section_config { sc_f_.section_config };

    auto* quantity = new DoubleNoneZeroR(section_config.quantity_decimal, string_const::kFourDigits, table_view);
    table_view->setItemDelegateForColumn(std::to_underlying(FullEntryEnumF::kLhsDebit), quantity);
    table_view->setItemDelegateForColumn(std::to_underlying(FullEntryEnumF::kRhsDebit), quantity);
    table_view->setItemDelegateForColumn(std::to_underlying(FullEntryEnumF::kLhsCredit), quantity);
    table_view->setItemDelegateForColumn(std::to_underlying(FullEntryEnumF::kRhsCredit), quantity);

    auto* rate { new DoubleNoneZeroR(section_config.rate_decimal, string_const::kFourDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(FullEntryEnumF::kLhsRate), rate);
    table_view->setItemDelegateForColumn(std::to_underlying(FullEntryEnumF::kRhsRate), rate);

    auto* path { new SearchPathTableR(sc_f_.tree_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(FullEntryEnumF::kLhsNode), path);
    table_view->setItemDelegateForColumn(std::to_underlying(FullEntryEnumF::kRhsNode), path);

    auto* issued_time { new IssuedTimeR(section_config.date_format, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(FullEntryEnumF::kIssuedTime), issued_time);

    auto* status { new StatusR(std::to_underlying(EntryStatus::kMarked), table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(FullEntryEnumF::kStatus), status);

    auto* cash_kind { new IntStringNoneZeroR(sc_f_.info.cash_kind_map, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(FullEntryEnumF::kCashKind), cash_kind);
}

void MainWindow::DelegateInventoryHeat(QTableView* table_view) const
{
    auto* path { new SearchPathTableR(sc_i_.tree_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(inventory_heat::RowField::kInventoryNode), path);

    auto* quantity { new DoubleR(sc_i_.section_config.quantity_decimal, string_const::kEightDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(inventory_heat::RowField::kTotalQuantity), quantity);

    auto* score { new DoubleNoneDecimalR(string_const::kEightDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(inventory_heat::RowField::kHeatScore), score);
}

void MainWindow::DelegatePartnerHeat(QTableView* table_view) const
{
    auto* path { new SearchPathTableR(sc_p_.tree_model, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(partner_heat::RowField::kPartnerNode), path);

    auto* quantity { new DoubleR(sc_p_.section_config.quantity_decimal, string_const::kEightDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(partner_heat::RowField::kTotalQuantity), quantity);

    auto* score { new DoubleNoneDecimalR(string_const::kEightDigits, table_view) };
    table_view->setItemDelegateForColumn(std::to_underlying(partner_heat::RowField::kHeatScore), score);
}

void MainWindow::DelegateBalanceSheet(QTreeView* view) const
{
    const auto& info { sc_f_.info };

    auto* amount { new AmountR(sc_f_.section_config.amount_decimal, sc_f_.shared_config.default_unit, info.unit_symbol_map, string_const::kEightDigits, view) };
    view->setItemDelegateForColumn(std::to_underlying(balance_sheet::RowField::kClosingBalance), amount);
    view->setItemDelegateForColumn(std::to_underlying(balance_sheet::RowField::kOpeningBalance), amount);
    view->setItemDelegateForColumn(std::to_underlying(balance_sheet::RowField::kChangeAmount), amount);

    auto* direction_rule { new BoolStringR(info.rule_map, view) };
    view->setItemDelegateForColumn(std::to_underlying(balance_sheet::RowField::kDirectionRule), direction_rule);

    auto* kind { new IntStringR(info.kind_map, view) };
    view->setItemDelegateForColumn(std::to_underlying(balance_sheet::RowField::kKind), kind);

    auto* growth_rate { new PercentageDelegateR(view) };
    view->setItemDelegateForColumn(std::to_underlying(balance_sheet::RowField::kChangeRate), growth_rate);
}

void MainWindow::DelegateIncomeStatement(QTreeView* view) const
{
    const auto& info { sc_f_.info };

    auto* amount { new AmountR(sc_f_.section_config.amount_decimal, sc_f_.shared_config.default_unit, info.unit_symbol_map, string_const::kEightDigits, view) };
    view->setItemDelegateForColumn(std::to_underlying(income_statement::RowField::kFinalTotal), amount);
    view->setItemDelegateForColumn(std::to_underlying(income_statement::RowField::kYoyFinalTotal), amount);
    view->setItemDelegateForColumn(std::to_underlying(income_statement::RowField::kMomFinalTotal), amount);

    auto* growth_rate { new PercentageDelegateR(view) };
    view->setItemDelegateForColumn(std::to_underlying(income_statement::RowField::kYoyGrowthRate), growth_rate);
    view->setItemDelegateForColumn(std::to_underlying(income_statement::RowField::kMomGrowthRate), growth_rate);

    auto* direction_rule { new BoolStringR(info.rule_map, view) };
    view->setItemDelegateForColumn(std::to_underlying(income_statement::RowField::kDirectionRule), direction_rule);

    auto* kind { new IntStringR(info.kind_map, view) };
    view->setItemDelegateForColumn(std::to_underlying(income_statement::RowField::kKind), kind);
}

void MainWindow::DelegateCashFlowStatement(QTreeView* view) const
{
    const auto& info { sc_f_.info };

    auto* amount { new AmountR(sc_f_.section_config.amount_decimal, sc_f_.shared_config.default_unit, info.unit_symbol_map, string_const::kEightDigits, view) };
    view->setItemDelegateForColumn(std::to_underlying(cash_flow::RowField::kFinalTotal), amount);

    auto* direction_rule { new BoolStringR(info.rule_map, view) };
    view->setItemDelegateForColumn(std::to_underlying(cash_flow::RowField::kDirectionRule), direction_rule);
}

void MainWindow::DelegateCashFlowStatementCarrier(QTreeView* view) const
{
    const auto& info { sc_f_.info };

    auto* direction_rule { new BoolStringR(info.rule_map, view) };
    view->setItemDelegateForColumn(std::to_underlying(cash_flow::RowField::kDirectionRule), direction_rule);

    auto* name { new CashFlowNameR(sc_f_.tree_model, view) };
    view->setItemDelegateForColumn(std::to_underlying(cash_flow::RowField::kName), name);
}

void MainWindow::DelegateCashFlowStatementWrong(QTableView* view) const
{
    const auto& config { sc_f_.section_config };
    const auto& info { sc_f_.info };

    auto* issued_time { new IssuedTimeR(config.date_format, view) };
    view->setItemDelegateForColumn(std::to_underlying(cash_flow::WrongRowField::kIssuedTime), issued_time);

    auto* value { new DoubleNoneZeroR(config.quantity_decimal, string_const::kFourDigits, view) };
    view->setItemDelegateForColumn(std::to_underlying(cash_flow::WrongRowField::kLhsDebit), value);
    view->setItemDelegateForColumn(std::to_underlying(cash_flow::WrongRowField::kLhsCredit), value);
    view->setItemDelegateForColumn(std::to_underlying(cash_flow::WrongRowField::kRhsDebit), value);
    view->setItemDelegateForColumn(std::to_underlying(cash_flow::WrongRowField::kRhsCredit), value);

    auto* cash_kind { new IntStringNoneZeroR(info.cash_kind_map, view) };
    view->setItemDelegateForColumn(std::to_underlying(cash_flow::WrongRowField::kCashKind), cash_kind);

    auto* node_path { new NodePathR(sc_f_.tree_model, view) };
    view->setItemDelegateForColumn(std::to_underlying(cash_flow::WrongRowField::kLhsNode), node_path);
    view->setItemDelegateForColumn(std::to_underlying(cash_flow::WrongRowField::kRhsNode), node_path);

    auto* kind { new IntStringR(info.cash_kind_map, view) };
    view->setItemDelegateForColumn(std::to_underlying(cash_flow::WrongRowField::kCashKind), kind);
}
