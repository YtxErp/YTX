/*
 * Copyright (C) 2023 YTX
 *
 * This file is part of YTX.
 *
 * YTX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YTX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with YTX. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <QActionGroup>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QTableView>
#include <QTranslator>

#include "audit/auditinfo.h"
#include "component/config.h"
#include "component/headerinfo.h"
#include "component/sectioncontex.h"
#include "component/sectioninfo.h"
#include "entryhub/entryhub.h"
#include "finance/settlement/settlement.h"
#include "finance/settlement/settlementprimarymodel.h"
#include "finance/statement/statementtertiarywidget.h"
#include "table/model/tablemodel.h"
#include "table/model/tablemodelo.h"
#include "table/widget/tablewidgetfipt.h"
#include "table/widget/tablewidgeto.h"
#include "tree/model/treemodel.h"
#include "tree/widget/treewidgeto.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void on_actionInsertNode_triggered();
    void on_actionAppendEntry_triggered();
    void on_actionDelete_triggered();
    void on_actionAppendNode_triggered();
    void on_actionRename_triggered();
    void on_actionClearColor_triggered();
    void on_actionNewBranch_triggered();
    void on_actionJumpEntry_triggered();
    void on_actionAbout_triggered();
    void on_actionPreferences_triggered();

    void on_actionSearch_triggered();
    void on_actionTags_triggered();

    void on_actionSignIn_triggered();
    void on_actionSignOut_triggered();
    void on_actionReconnect_triggered();

    void on_actionProfile_triggered();
    void on_actionWorkspaceManager_triggered();

    void on_actionCheckUpdates_triggered();

    void on_actionStatement_triggered();
    void on_actionSettlement_triggered();
    void on_actionAuditLog_triggered();
    void on_actionPeriodClose_triggered();

    void on_actionHeatInventory_triggered();
    void on_actionHeatPartner_triggered();
    void on_actionBalanceSheet_triggered();
    void on_actionIncomeStatement_triggered();
    void on_actionCashFlowStatement_triggered();
    void on_actionSettlementView_triggered();

    void tabWidget_currentChanged();
    void tabWidget_tabBarDoubleClicked(int index);
    void tabWidget_tabCloseRequestedFIT(int index);
    void tabWidget_tabCloseRequestedP(int index);
    void tabWidget_tabCloseRequestedO(int index);

    void RLocateNode(Section section, const QUuid& node_id);
    void REntryLocation(const QUuid& entry_id, const QUuid& lhs_node_id, const QUuid& rhs_node_id);
    void RSelectNode(Section section, const QUuid& node_id);

    void RUpdateConfig(const AppConfig& app, const SharedConfig& shared, const SectionConfig& section);
    void RSyncPartner(const QUuid& parent_id, const QUuid& node_id, const QUuid& partner_id);
    void RUpdatePartner(const QUuid& widget_id, const QUuid& partner_id);
    void RUpdateName(const QUuid& node_id, const QString& name, bool branch);
    void RMessage(QMessageBox::Icon icon, const QString& message);

    void RAccountName(const QString& name);
    void RAccountUsername(const QJsonObject& obj);

    void RMarkEntries(MarkOperation operation);
    void RStatementMarkEntries(MarkOperation operation);

    void RDenyConnection();
    void RAllowConnection();
    void RRemoteHostClosed();

    void RAllowLogin(const QString& name, const QString& expire_date);
    void RDenyLogin();
    void RFinishTreeSync();

    void RFreeWidget(Section section, const QUuid& node_id);
    void RFlushCaches();

    void ROrderHistory(Section section, const QUuid& widget_id, const QJsonArray& array);
    void RShowOrderHistoryWidget(const QUuid& node_id, NodeUnit unit);

    void RStatementPrimary(Section section, const QUuid& widget_id, const QJsonArray& array);
    void RStatemetSecondary(Section section, const QUuid& widget_id, const QJsonArray& array);
    void RStatementTertiary(Section section, const QUuid& widget_id, const QJsonArray& array);

    void RTreeViewCustomContextMenuRequested(const QPoint& pos);
    void RInsertNodeTag(const tag::Row* tag, TreeModel* model, const Node* node);
    void RRemoveNodeTag(const tag::Row* tag, TreeModel* model, const Node* node);

    void RTreeViewDoubleClicked(const QModelIndex& index);
    void ROpenSettlementSecondaryWidget(const QModelIndex& index);
    void RCreateSettlementSecondaryWidget(settlement::PrimaryModel* model);

    void RSectionGroup(int id);

    void RShowSecondaryStatement(const QUuid& partner_id, const utils::DateRange& range, int unit);
    void RShowTertiaryStatement(const QUuid& partner_id, const utils::DateRange& range, int unit);

    void RSettlementPrimary(Section section, const QUuid& widget_id, const QJsonArray& array);
    void RSettlementSecondary(Section section, const QUuid& widget_id, const QJsonArray& array);
    void RInsertSettlement(const QJsonObject& obj);
    void RRecallSettlement(const QJsonObject& obj);
    void RUpdateSettlement(const QJsonObject& obj);
    void RSettlementView(Section section, const QUuid& widget_id, const QJsonArray& array);

    void RReleaseOrder(Section section, const QUuid& node_id, int version);
    void RRecallOrder(Section section, const QUuid& node_id, int version);
    void RSaveOrder(Section section, const QUuid& node_id, int version);
    void RDenyOperation();

    void RDenyLeafDelete(const QJsonObject& obj);

    void RWorkspaceMemberAck(const QUuid& widget_id, const QJsonArray& array);
    void RAuditLogAck(const QUuid& widget_id, const QJsonArray& array);
    void RInventoryHeatAck(const QUuid& widget_id, const QJsonArray& array);
    void RPartnerHeatAck(const QUuid& widget_id, const QJsonArray& array);
    void RBalanceSheetAck(const QUuid& widget_id, const QJsonArray& node_array, const QJsonArray& path_array);
    void RIncomeStatementAck(const QUuid& widget_id, const QJsonObject& obj);
    void RCashFlowStatementAck(const QUuid& widget_id, const QJsonObject& obj);

    void RAccountRoleUpdate();

    void RApplyTag(const QJsonObject& obj);
    void RInsertTag(const QJsonObject& obj);
    void RInsertLocalTag(Section section, tag::Row* tag);
    void RUpdateTag(const QJsonObject& obj);
    void RDeleteTag(const QJsonObject& obj);

    void RTableViewCustomContextMenuRequested(const QPoint& pos);
    void RInsertEntryTag(const tag::Row* tag, TableModel* model, const Entry* entry);
    void RRemoveEntryTag(const tag::Row* tag, TableModel* model, const Entry* entry);

    void RApplySharedConfig(const QJsonArray& arr);
    void RUpdateDocumentDir(Section section, const QString& document_dir);
    void RUpdateDefaultUnit(Section section, int unit);
    void RDenyDefaultUnit(const QString& section);
    void RSelectEntry(const QUuid& node_id, const QUuid& entry_id);

private:
    void SetTabWidget(QTabWidget* tab_widget);
    void ResetMainwindow();

    void SetUniqueConnection() const;
    void SetIcon() const;

    void InitilizeContext();
    void InitContextFinance();
    void InitContextInventory();
    void InitContextPartner();
    void InitContextTask();
    void InitContextSale();
    void InitContextPurchase();

    void ShowLeafWidget(const QUuid& node_id, const QUuid& entry_id = {});

    void CreateLeafFIPT(SectionContext* sc, const QUuid& node_id);
    void CreateLeafO(SectionContext* sc, const QUuid& node_id);
    void CreateSettlementSecondary(const settlement::PrimaryRow& primary_row, settlement::PrimaryModel* primary_model);

    void TableDelegateF(QTableView* table_view, TreeModel* tree_model, CSectionConfig& config, const QUuid& node_id) const;
    void TableDelegateI(QTableView* table_view, TreeModel* tree_model, CSectionConfig& config, const QUuid& node_id) const;
    void TableDelegateT(QTableView* table_view, TreeModel* tree_model, CSectionConfig& config, const QUuid& node_id) const;
    void TableDelegateP(QTableView* table_view, CSectionConfig& config) const;
    void TableDelegateO(QTableView* table_view, CSectionInfo& info, CSectionConfig& config) const;

    void SetTableView(QTableView* view, Section section, int stretch_column) const;

    void TableConnectF(TableModel* table_model) const;
    void TableConnectI(TableModel* table_model) const;
    void TableConnectT(TableModel* table_model) const;
    void TableConnectP(TableModel* table_model) const;
    void TableConnectO(TableModelO* table_model_o, TableWidgetO* widget) const;

    void DelegateSalesHistoryI(QTableView* table_view, CSectionConfig& config) const;
    void DelegateOrderHistoryP(QTableView* table_view, CSectionConfig& config) const;

    void DelegateStatementPrimary(QTableView* table_view, CSectionConfig& config) const;
    void DelegateStatementSecondary(QTableView* table_view, CSectionConfig& config) const;
    void DelegateStatementTertiary(QTableView* table_view, CSectionConfig& config) const;
    void DelegateSettlementPrimary(QTableView* table_view, CSectionConfig& config) const;
    void DelegateSettlementSecondary(QTableView* table_view, CSectionConfig& config) const;
    void DelegateTag(QTableView* table_view) const;
    void DelegateWorkspaceMember(QTableView* table_view) const;
    void DelegateAuditLog(QTableView* table_view) const;
    void DelegatePeriodClose(QTableView* table_view) const;
    void DelegateInventoryHeat(QTableView* table_view) const;
    void DelegatePartnerHeat(QTableView* table_view) const;
    void DelegateBalanceSheet(QTreeView* view) const;
    void DelegateIncomeStatement(QTreeView* view) const;
    void DelegateCashFlowStatement(QTreeView* view) const;
    void DelegateCashFlowStatementWrong(QTableView* view) const;
    void DelegateCashFlowStatementCarrier(QTreeView* view) const;

    void InitTableView(QTableView* view, int stretch_column) const;
    void InitTreeView(QTreeView* view, int id_column, int stretch_column) const;

    void CreateSection(SectionContext& sc, CString& name);

    void EditNameFIPT();
    void EditNameO();

    void SetTreeView(QTreeView* view, Section section) const;
    void SetTreeHeader(QTreeView* view, Section section);

    void TreeDelegateF(QTreeView* tree_view, CSectionInfo& info, CSectionConfig& section) const;
    void TreeDelegateT(QTreeView* tree_view, CSectionInfo& info, CSectionConfig& section) const;
    void TreeDelegateI(QTreeView* tree_view, CSectionInfo& info, CSectionConfig& section) const;
    void TreeDelegateP(QTreeView* tree_view, CSectionInfo& info, CSectionConfig& section) const;
    void TreeDelegateO(QTreeView* tree_view, CSectionInfo& info, CSectionConfig& section) const;

    void TreeConnectF(QTreeView* tree_view, TreeModel* tree_model, const EntryHub* entry_hub) const;
    void TreeConnectI(QTreeView* tree_view, TreeModel* tree_model, const EntryHub* entry_hub) const;
    void TreeConnectT(QTreeView* tree_view, TreeModel* tree_model, const EntryHub* entry_hub) const;
    void TreeConnectP(QTreeView* tree_view, TreeModel* tree_model, const EntryHub* entry_hub) const;
    void TreeConnectO(QTreeView* tree_view, TreeModel* tree_model, const EntryHub* entry_hub, Section section) const;

    void InsertNodeFunction(const QModelIndex& parent_index);
    void InsertNodeFIPT(const QModelIndex& parent_index); // Finance Inventory Partner Task
    void InsertNodeO(const QModelIndex& parent_index); // Purchase Sales

    void DeleteNode();
    void DeleteEntry(TableWidget* widget);
    void DeleteBranch(Node* node);

    void UpdatePartnerReference(const SectionContext& sc, const QSet<QUuid>& partner_nodes, bool branch) const;

    void LoadAndInstallTranslator(CString& language);
    void ResizeColumn(QHeaderView* header, int stretch_column) const;

    void ReadLocalConfig();
    void ReadSectionConfig(SectionConfig& config, CString& section_name);

    void UpdateAppConfig(CAppConfig& app);
    void UpdateSectionConfig(CSectionConfig& section) const;
    void UpdateSharedConfig(CSharedConfig& shared);

    void UpdateAccountInfo(const QString& workspace, const QString& name, const QString& expire_date);

    void SetAction(bool enable) const;
    void SetLoginAction(bool enable) const;

    void IniSectionGroup();
    void IniMarkGroup();

    void FocusTabWidget(const QUuid& node_id) const;

    void RegisterWidget(QWidget* widget, const QUuid& widget_id, WidgetRole role);
    void WriteConfig();

    SectionContext* GetSectionContex(Section section);
    void InitSystemTray();
    void InitStatusLabel();
    void InitHeader();
    void InitHeader(SectionInfo& finance, SectionInfo& inventory, SectionInfo& task, SectionInfo& partner, SectionInfo& sale, SectionInfo& purchase);
    void InitAuditInfo();

    void FlushCaches(SectionContext& sc);

    QSet<QString> ChildrenName(const Node* node) const;
    QSet<QUuid> LeafChildrenId(const Node* node) const;

    void UpdateLeafNodeName(const QUuid& node_id, const QString& name);
    void UpdateOneTab(const QUuid& node_id, const QString& name);

    void UpdateBranchNodeName(const QUuid& node_id);
    void UpdateMultiTabs(const QSet<QUuid>& nodes);

    QIcon GetTagIcon(SectionContext* sc, const tag::Row* tag, bool checked);

    void UpdateTagIcon(SectionContext* sc, const tag::Row* tag);

    inline bool IsTreeWidget(const QWidget* widget) { return widget && widget->inherits(kTreeWidget); }
    inline bool IsTreeWidgetO(const QWidget* widget) { return widget && widget->inherits(kTreeWidgetO); }
    inline bool IsTableWidgetFIPT(const QWidget* widget) { return widget && widget->inherits(kTableWidgetFIPT); }
    inline bool IsTableWidgetO(const QWidget* widget) { return widget && widget->inherits(kTableWidgetO); }
    inline bool IsStatementTertiaryWidget(const QWidget* widget) { return widget && widget->inherits(kStatementTertiaryWidget); }

private:
    Ui::MainWindow* ui {};
    QHash<QUuid, WidgetContext> widget_hash_ {};

    HeaderInfo header_info_ {};
    audit::Info audit_info_ {};

    Section start_ {};
    QLabel* connection_label_ {};
    QLabel* login_label_ {};

    QSystemTrayIcon* tray_icon_ {};
    QMenu* tray_menu_ {};

    QTranslator qt_translator_ {};
    QTranslator ytx_translator_ {};

    AppConfig app_config_ {};

    QSharedPointer<QSettings> app_settings_ {};
    QSharedPointer<QSettings> section_settings_ {};
    QNetworkAccessManager* network_manager_ {};

    QButtonGroup* section_group_ {};
    QActionGroup* mark_group_ {};

    SectionContext* sc_ {};

    SectionContext sc_f_ {};
    SectionContext sc_i_ {};
    SectionContext sc_t_ {};
    SectionContext sc_p_ {};
    SectionContext sc_sale_ {};
    SectionContext sc_purchase_ {};
};
