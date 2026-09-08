#include <QtCore/qdir.h>
#include <QtWidgets/qheaderview.h>

#include "entryhub/entryhubf.h"
#include "entryhub/entryhubi.h"
#include "entryhub/entryhubo.h"
#include "entryhub/entryhubp.h"
#include "entryhub/entryhubt.h"
#include "global/masterdataregistry.h"
#include "global/printhub.h"
#include "mainwindow.h"
#include "tree/model/treemodelf.h"
#include "tree/model/treemodelo.h"
#include "tree/model/treemodelp.h"
#include "tree/model/treemodelt.h"
#include "tree/widget/treewidgetf.h"
#include "tree/widget/treewidgetit.h"
#include "tree/widget/treewidgeto.h"
#include "tree/widget/treewidgetp.h"
#include "ui_mainwindow.h"
#include "websocket/websocket.h"

void MainWindow::SetAction(bool enable) const
{
    ui->actionAppendNode->setEnabled(enable);
    ui->actionMarkAll->setEnabled(enable);
    ui->actionMarkNone->setEnabled(enable);
    ui->actionMarkToggle->setEnabled(enable);
    ui->actionRename->setEnabled(enable);
    ui->actionInsertNode->setEnabled(enable);
    ui->actionDelete->setEnabled(enable);
    ui->actionAppendEntry->setEnabled(enable);
    ui->actionStatement->setEnabled(enable);
    ui->actionSettlement->setEnabled(enable);
    ui->actionClearColor->setEnabled(enable);
    ui->actionNewBranch->setEnabled(enable);
    ui->actionJumpEntry->setEnabled(enable);
    ui->actionSettlementView->setEnabled(enable);
}

void MainWindow::SetLoginAction(bool enable) const
{
    ui->actionSearch->setEnabled(enable);
    ui->actionTags->setEnabled(enable);
    ui->actionPeriodClose->setEnabled(enable);
    ui->actionBalanceSheet->setEnabled(enable);
    ui->actionCashFlowStatement->setEnabled(enable);
    ui->actionIncomeStatement->setEnabled(enable);
    ui->actionHeatInventory->setEnabled(enable);
    ui->actionHeatPartner->setEnabled(enable);
    ui->actionProfile->setEnabled(enable);
    ui->actionWorkspaceManager->setEnabled(enable);
    ui->actionAuditLog->setEnabled(enable);
}

void MainWindow::UpdateAccountInfo(const QString& workspace, const QString& name, const QString& expire_date)
{
    ui->actionName->setText(tr("Name", "Person") + ": " + name);
    ui->actionWorkspace->setText(tr("Workspace") + ": " + workspace);
    ui->actionExpireDate->setText(tr("Expire Date") + ": " + expire_date);
}

void MainWindow::CreateSection(SectionContext& sc, CString& name)
{
    auto tab_widget { sc.tab_widget };
    auto tree_widget { sc.tree_widget };

    auto view { sc.tree_view };
    auto tree_model { sc.tree_model };
    auto entry_hub { sc.entry_hub };

    const auto& info { sc.info };
    const auto& section { info.section };

    SetTreeView(view, section);

    auto* tab_bar = tab_widget->tabBar();
    const int index { tab_widget->addTab(tree_widget, name) };

    tab_bar->setTabData(index, QUuid());
    tab_bar->setTabButton(index, QTabBar::RightSide, nullptr);

    switch (info.section) {
    case Section::kFinance:
        TreeConnectF(view, tree_model, entry_hub);
        break;
    case Section::kTask:
        TreeConnectT(view, tree_model, entry_hub);
        break;
    case Section::kPartner:
        TreeConnectP(view, tree_model, entry_hub);
        break;
    case Section::kInventory:
        TreeConnectI(view, tree_model, entry_hub);
        break;
    case Section::kSale:
    case Section::kPurchase:
        TreeConnectO(view, tree_model, entry_hub, section);
        break;
    default:
        break;
    }
}

void MainWindow::InitilizeContext()
{
    {
        InitHeader();
        InitHeader(sc_f_.info, sc_i_.info, sc_t_.info, sc_p_.info, sc_sale_.info, sc_purchase_.info);
    }

    {
        InitContextFinance();
        InitContextTask();
        InitContextInventory();
        InitContextPartner();
        InitContextSale();
        InitContextPurchase();
    }

    {
        CreateSection(sc_f_, tr("Finance"));
        CreateSection(sc_p_, tr("Partner"));
        CreateSection(sc_i_, tr("Inventory"));
        CreateSection(sc_t_, tr("Task"));
        CreateSection(sc_sale_, tr("Sale"));
        CreateSection(sc_purchase_, tr("Purchase"));
    }

    {
        if (auto* btn { section_group_->button(std::to_underlying(start_)) })
            btn->click();
    }

    {
        PrintHub::Instance().ScanTemplate();
        PrintHub::Instance().SetAppConfig(&app_config_);
    }

    {
        InitAuditInfo(); // sections must be ready before this
    }
}

void MainWindow::InitContextFinance()
{
    auto& info { sc_f_.info };
    auto& section_config { sc_f_.section_config };
    auto& shared_config { sc_f_.shared_config };
    auto& entry_hub { sc_f_.entry_hub };
    auto& tree_model { sc_f_.tree_model };
    auto& tree_view { sc_f_.tree_view };
    auto& tree_widget { sc_f_.tree_widget };

    info.section = Section::kFinance;
    info.node = kFinanceNode;
    info.path = kFinancePath;
    info.entry = kFinanceEntry;

    for (NodeUnit c : {
             NodeUnit::USD,
             NodeUnit::EUR,
             NodeUnit::JPY,
             NodeUnit::GBP,
             NodeUnit::AUD,
             NodeUnit::CAD,
             NodeUnit::CHF,
             NodeUnit::CNY,
             NodeUnit::HKD,
             NodeUnit::NZD,
             NodeUnit::SEK,
             NodeUnit::NOK,
             NodeUnit::SGD,
             NodeUnit::KRW,
             NodeUnit::MXN,
             NodeUnit::INR,
         }) {
        const int key { std::to_underlying(c) };
        info.unit_map.insert(key, kUnitCode(c));
        info.unit_symbol_map.insert(key, kUnitSymbol(c));
    }

    info.rule_map.insert(direction_rule::kDDCI, kStrDDCI);
    info.rule_map.insert(direction_rule::kDICD, kStrDICD);

    info.kind_map.insert(std::to_underlying(NodeKind::kBranch), kBranchKind);
    info.kind_map.insert(std::to_underlying(NodeKind::kLeaf), kLeafKind);

    info.unit_model = new UnitModel(this);
    info.unit_model->Rebuild(info.unit_map);

    info.cash_kind_map.insert(std::to_underlying(finance::CashKind::kNone), QString());

    // Operating Inflow
    info.cash_kind_map.insert(std::to_underlying(finance::CashKind::kSalesReceipt), QObject::tr("Sales Receipt"));
    info.cash_kind_map.insert(std::to_underlying(finance::CashKind::kTaxRefund), QObject::tr("Tax Refund"));
    info.cash_kind_map.insert(std::to_underlying(finance::CashKind::kOtherOperatingReceipt), QObject::tr("Other Operating Receipt"));

    // Operating Outflow
    info.cash_kind_map.insert(std::to_underlying(finance::CashKind::kPurchasePayment), QObject::tr("Purchase Payment"));
    info.cash_kind_map.insert(std::to_underlying(finance::CashKind::kSalaryPayment), QObject::tr("Salary Payment"));
    info.cash_kind_map.insert(std::to_underlying(finance::CashKind::kTaxPayment), QObject::tr("Tax Payment"));
    info.cash_kind_map.insert(std::to_underlying(finance::CashKind::kOtherOperatingPayment), QObject::tr("Other Operating Payment"));

    // Investing Inflow
    info.cash_kind_map.insert(std::to_underlying(finance::CashKind::kInvestmentReceipt), QObject::tr("Investment Receipt"));
    info.cash_kind_map.insert(std::to_underlying(finance::CashKind::kInvestmentIncomeReceipt), QObject::tr("Investment Income Receipt"));
    info.cash_kind_map.insert(std::to_underlying(finance::CashKind::kAssetDisposalReceipt), QObject::tr("Asset Disposal Receipt"));
    info.cash_kind_map.insert(std::to_underlying(finance::CashKind::kSubsidiaryDisposalReceipt), QObject::tr("Subsidiary Disposal Receipt"));
    info.cash_kind_map.insert(std::to_underlying(finance::CashKind::kLoanRepaymentReceipt), QObject::tr("Loan Repayment Receipt"));
    info.cash_kind_map.insert(std::to_underlying(finance::CashKind::kOtherInvestingReceipt), QObject::tr("Other Investing Receipt"));

    // Investing Outflow
    info.cash_kind_map.insert(std::to_underlying(finance::CashKind::kAssetPurchasePayment), QObject::tr("Asset Purchase Payment"));
    info.cash_kind_map.insert(std::to_underlying(finance::CashKind::kInvestmentPayment), QObject::tr("Investment Payment"));
    info.cash_kind_map.insert(std::to_underlying(finance::CashKind::kSubsidiaryAcquisitionPayment), QObject::tr("Subsidiary Acquisition Payment"));
    info.cash_kind_map.insert(std::to_underlying(finance::CashKind::kLoanPayment), QObject::tr("Loan Payment"));
    info.cash_kind_map.insert(std::to_underlying(finance::CashKind::kOtherInvestingPayment), QObject::tr("Other Investing Payment"));

    // Financing Inflow
    info.cash_kind_map.insert(std::to_underlying(finance::CashKind::kCapitalContributionReceipt), QObject::tr("Capital Contribution Receipt"));
    info.cash_kind_map.insert(std::to_underlying(finance::CashKind::kBorrowingReceipt), QObject::tr("Borrowing Receipt"));
    info.cash_kind_map.insert(std::to_underlying(finance::CashKind::kOtherFinancingReceipt), QObject::tr("Other Financing Receipt"));

    // Financing Outflow
    info.cash_kind_map.insert(std::to_underlying(finance::CashKind::kDebtRepaymentPayment), QObject::tr("Debt Repayment Payment"));
    info.cash_kind_map.insert(std::to_underlying(finance::CashKind::kDividendPayment), QObject::tr("Dividend Payment"));
    info.cash_kind_map.insert(std::to_underlying(finance::CashKind::kInterestPayment), QObject::tr("Interest Payment"));
    info.cash_kind_map.insert(std::to_underlying(finance::CashKind::kOtherFinancingPayment), QObject::tr("Other Financing Payment"));

    // Special
    info.cash_kind_map.insert(std::to_underlying(finance::CashKind::kExchangeRateEffect), QObject::tr("Exchange Rate Effect"));
    info.cash_kind_map.insert(std::to_underlying(finance::CashKind::kAdjustment), QObject::tr("Adjustment"));
    info.cash_kind_map.insert(std::to_underlying(finance::CashKind::kUnclassified), QObject::tr("Unclassified"));

    info.status_map.insert(std::to_underlying(NodeStatus::kInactive), tr("Inactive"));
    info.status_map.insert(std::to_underlying(NodeStatus::kActive), QString());

    info.cash_kind_model = new UnitModel(this);
    info.cash_kind_model->Rebuild(info.cash_kind_map);

    entry_hub = new EntryHubF(info, this);
    tree_model = new TreeModelF(info, app_config_.separator, this);

    WebSocket::Instance()->RegisterTreeModel(Section::kFinance, tree_model);
    WebSocket::Instance()->RegisterEntryHub(Section::kFinance, entry_hub);

    tree_widget = new TreeWidgetF(tree_model, info, shared_config, section_config, this);
    tree_view = tree_widget->View();

    sc_f_.tab_widget = ui->tabWidgetF;

    connect(tree_model, &TreeModel::SSyncValue, tree_widget, &TreeWidget::RSyncValue, Qt::UniqueConnection);
    connect(tree_model, &TreeModel::SInitStatus, tree_widget, &TreeWidget::RInitStatus, Qt::UniqueConnection);
}

void MainWindow::InitContextInventory()
{
    auto& info { sc_i_.info };
    auto& section_config { sc_i_.section_config };
    auto& entry_hub { sc_i_.entry_hub };
    auto& tree_model { sc_i_.tree_model };
    auto& tree_view { sc_i_.tree_view };
    auto& tree_widget { sc_i_.tree_widget };

    info.section = Section::kInventory;
    info.node = kInventoryNode;
    info.path = kInventoryPath;
    info.entry = kInventoryEntry;

    info.unit_map.insert(std::to_underlying(NodeUnit::IItem), tr("ITM", "Item"));
    info.unit_map.insert(std::to_underlying(NodeUnit::IPosition), tr("POS", "Position"));

    info.rule_map.insert(direction_rule::kDDCI, kStrDDCI);
    info.rule_map.insert(direction_rule::kDICD, kStrDICD);

    info.kind_map.insert(std::to_underlying(NodeKind::kBranch), kBranchKind);
    info.kind_map.insert(std::to_underlying(NodeKind::kLeaf), kLeafKind);

    info.status_map.insert(std::to_underlying(NodeStatus::kInactive), tr("Inactive"));
    info.status_map.insert(std::to_underlying(NodeStatus::kActive), QString());

    info.unit_model = new UnitModel(this);
    info.unit_model->Rebuild(info.unit_map);

    entry_hub = new EntryHubI(info, this);
    tree_model = new TreeModelI(info, app_config_.separator, this);

    WebSocket::Instance()->RegisterTreeModel(Section::kInventory, tree_model);
    WebSocket::Instance()->RegisterEntryHub(Section::kInventory, entry_hub);

    MasterDataRegistry::Instance().RegisterTreeModel(Section::kInventory, tree_model);

    tree_widget = new TreeWidgetIT(tree_model, section_config, this);
    tree_view = tree_widget->View();

    sc_i_.tab_widget = ui->tabWidgetI;

    connect(tree_model, &TreeModel::SSyncValue, tree_widget, &TreeWidget::RSyncValue, Qt::UniqueConnection);
    connect(tree_model, &TreeModel::SInitStatus, tree_widget, &TreeWidget::RInitStatus, Qt::UniqueConnection);
}

void MainWindow::InitContextTask()
{
    auto& info { sc_t_.info };
    auto& section_config { sc_t_.section_config };
    auto& entry_hub { sc_t_.entry_hub };
    auto& tree_model { sc_t_.tree_model };
    auto& tree_view { sc_t_.tree_view };
    auto& tree_widget { sc_t_.tree_widget };

    info.section = Section::kTask;
    info.node = kTaskNode;
    info.path = kTaskPath;
    info.entry = kTaskEntry;

    info.unit_map.insert(std::to_underlying(NodeUnit::TTarget), tr("TGT", "Target"));
    info.unit_map.insert(std::to_underlying(NodeUnit::TAction), tr("ACT", "Action"));
    info.unit_map.insert(std::to_underlying(NodeUnit::TOrigin), tr("ORG", "Origin"));

    info.rule_map.insert(direction_rule::kDDCI, kStrDDCI);
    info.rule_map.insert(direction_rule::kDICD, kStrDICD);

    info.kind_map.insert(std::to_underlying(NodeKind::kBranch), kBranchKind);
    info.kind_map.insert(std::to_underlying(NodeKind::kLeaf), kLeafKind);

    info.status_map.insert(std::to_underlying(NodeStatus::kInactive), tr("Inactive"));
    info.status_map.insert(std::to_underlying(NodeStatus::kActive), QString());

    info.unit_model = new UnitModel(this);
    info.unit_model->Rebuild(info.unit_map);

    entry_hub = new EntryHubT(info, this);
    tree_model = new TreeModelT(info, app_config_.separator, this);

    WebSocket::Instance()->RegisterTreeModel(Section::kTask, tree_model);
    WebSocket::Instance()->RegisterEntryHub(Section::kTask, entry_hub);

    tree_widget = new TreeWidgetIT(tree_model, section_config, this);
    tree_view = tree_widget->View();

    sc_t_.tab_widget = ui->tabWidgetT;

    connect(tree_model, &TreeModel::SSyncValue, tree_widget, &TreeWidget::RSyncValue, Qt::UniqueConnection);
    connect(tree_model, &TreeModel::SInitStatus, tree_widget, &TreeWidget::RInitStatus, Qt::UniqueConnection);
}

void MainWindow::InitContextPartner()
{
    auto& info { sc_p_.info };
    auto& entry_hub { sc_p_.entry_hub };
    auto& tree_model { sc_p_.tree_model };
    auto& tree_view { sc_p_.tree_view };
    auto& tree_widget { sc_p_.tree_widget };
    auto& section_config { sc_p_.section_config };

    info.section = Section::kPartner;
    info.node = kPartnerNode;
    info.path = kPartnerPath;
    info.entry = kPartnerEntry;

    info.unit_map.insert(std::to_underlying(NodeUnit::PCustomer), tr("CUS", "Customer"));
    info.unit_map.insert(std::to_underlying(NodeUnit::PEmployee), tr("EMP", "Employee"));
    info.unit_map.insert(std::to_underlying(NodeUnit::PVendor), tr("VEN", "Vendor"));

    info.kind_map.insert(std::to_underlying(NodeKind::kBranch), kBranchKind);
    info.kind_map.insert(std::to_underlying(NodeKind::kLeaf), kLeafKind);

    info.status_map.insert(std::to_underlying(NodeStatus::kInactive), tr("Inactive"));
    info.status_map.insert(std::to_underlying(NodeStatus::kActive), QString());

    info.unit_model = new UnitModel(this);
    info.unit_model->Rebuild(info.unit_map);

    entry_hub = new EntryHubP(info, this);
    tree_model = new TreeModelP(info, app_config_.separator, this);

    WebSocket::Instance()->RegisterTreeModel(Section::kPartner, tree_model);
    WebSocket::Instance()->RegisterEntryHub(Section::kPartner, entry_hub);

    MasterDataRegistry::Instance().RegisterTreeModel(Section::kPartner, tree_model);

    tree_widget = new TreeWidgetP(tree_model, section_config, this);
    tree_view = tree_widget->View();

    sc_p_.tab_widget = ui->tabWidgetP;

    connect(tree_model, &TreeModel::SSyncValue, tree_widget, &TreeWidget::RSyncValue, Qt::UniqueConnection);
    connect(tree_model, &TreeModel::SInitStatus, tree_widget, &TreeWidget::RInitStatus, Qt::UniqueConnection);
}

void MainWindow::InitContextSale()
{
    auto& info { sc_sale_.info };
    auto& entry_hub { sc_sale_.entry_hub };
    auto& tree_model { sc_sale_.tree_model };
    auto& tree_view { sc_sale_.tree_view };
    auto& tree_widget { sc_sale_.tree_widget };

    info.section = Section::kSale;
    info.node = kSaleNode;
    info.path = kSalePath;
    info.entry = kSaleEntry;

    info.rule_map.insert(direction_rule::kRO, tr("RO", "Return Order"));
    info.rule_map.insert(direction_rule::kFO, tr("FO", "Forward Order"));

    info.unit_map.insert(std::to_underlying(NodeUnit::OImmediate), tr("IMM", "Immediate"));
    info.unit_map.insert(std::to_underlying(NodeUnit::OMonthly), tr("MON", "Monthly"));
    info.unit_map.insert(std::to_underlying(NodeUnit::OPending), tr("PEND", "Pending"));

    info.kind_map.insert(std::to_underlying(NodeKind::kBranch), kBranchKind);
    info.kind_map.insert(std::to_underlying(NodeKind::kLeaf), kLeafKind);

    info.unit_model = new UnitModel(this);
    info.unit_model->Rebuild(info.unit_map);

    auto* entry_hub_o = new EntryHubO(info, this);
    auto* tree_model_o = new TreeModelO(info, app_config_.separator, this);

    entry_hub = entry_hub_o;
    tree_model = tree_model_o;

    WebSocket::Instance()->RegisterTreeModel(Section::kSale, tree_model);
    WebSocket::Instance()->RegisterEntryHub(Section::kSale, entry_hub);

    tree_widget = new TreeWidgetO(Section::kSale, tree_model, this);
    tree_view = tree_widget->View();

    sc_sale_.tab_widget = ui->tabWidgetSale;
}

void MainWindow::InitContextPurchase()
{
    auto& info { sc_purchase_.info };
    auto& entry_hub { sc_purchase_.entry_hub };
    auto& tree_model { sc_purchase_.tree_model };
    auto& tree_view { sc_purchase_.tree_view };
    auto& tree_widget { sc_purchase_.tree_widget };

    info.section = Section::kPurchase;
    info.node = kPurchaseNode;
    info.path = kPurchasePath;
    info.entry = kPurchaseEntry;

    info.rule_map.insert(direction_rule::kRO, tr("RO", "Return Order"));
    info.rule_map.insert(direction_rule::kFO, tr("FO", "Forward Order"));

    info.unit_map.insert(std::to_underlying(NodeUnit::OImmediate), tr("IMM", "Immediate"));
    info.unit_map.insert(std::to_underlying(NodeUnit::OMonthly), tr("MON", "Monthly"));
    info.unit_map.insert(std::to_underlying(NodeUnit::OPending), tr("PEND", "Pending"));

    info.kind_map.insert(std::to_underlying(NodeKind::kBranch), kBranchKind);
    info.kind_map.insert(std::to_underlying(NodeKind::kLeaf), kLeafKind);

    info.unit_model = new UnitModel(this);
    info.unit_model->Rebuild(info.unit_map);

    auto* entry_hub_o = new EntryHubO(info, this);
    auto* tree_model_o = new TreeModelO(info, app_config_.separator, this);

    entry_hub = entry_hub_o;
    tree_model = tree_model_o;

    WebSocket::Instance()->RegisterTreeModel(Section::kPurchase, tree_model);
    WebSocket::Instance()->RegisterEntryHub(Section::kPurchase, entry_hub);

    tree_widget = new TreeWidgetO(Section::kPurchase, tree_model, this);
    tree_view = tree_widget->View();

    sc_purchase_.tab_widget = ui->tabWidgetPurchase;
}
