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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFileInfo>
#include <QLockFile>
#include <QMainWindow>
#include <QPointer>
#include <QSettings>
#include <QTableView>
#include <QTranslator>

#include "component/data.h"
#include "component/settings.h"
#include "component/using.h"
#include "report/widget/reportwidget.h"
#include "report/widget/settlementwidget.h"
#include "support/widget/supportwidget.h"
#include "table/model/transmodel.h"
#include "table/model/transmodelo.h"
#include "table/widget/transwidgeto.h"
#include "tree/model/nodemodel.h"
#include "tree/widget/nodewidget.h"
#include "ui_mainwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    bool ROpenFile(CString& file_path);

public slots:
    void on_actionInsertNode_triggered();
    void on_actionAppendTrans_triggered();
    void on_actionRemove_triggered();

private slots:
    void on_actionAppendNode_triggered();
    void on_actionEditNode_triggered();
    void on_actionJump_triggered();
    void on_actionSupportJump_triggered();
    void on_actionAbout_triggered();
    void on_actionPreferences_triggered();
    void on_actionLicence_triggered();
    void on_actionSearch_triggered();

    void on_actionNewDatabase_triggered();

    void on_actionExportExcel_triggered();

    void on_actionStatement_triggered();
    void on_actionSettlement_triggered();

    void on_tabWidget_currentChanged(int index);
    void on_tabWidget_tabBarDoubleClicked(int index);
    void on_tabWidget_tabCloseRequested(int index);

    void RNodeLocation(int node_id);
    void RTransLocation(int trans_id, int lhs_node_id, int rhs_node_id);

    void RUpdateSettings(const AppConfig& app_settings, const FileConfig& file_settings, const SectionConfig& section_settings);
    void RSyncInt(int node_id, int column, const QVariant& value);
    void RSyncName(int node_id, const QString& name, bool branch);
    void RUpdateState();

    void RFreeWidget(int node_id, int node_type);
    void REditTransDocument(const QModelIndex& index);
    void REditNodeDocument(const QModelIndex& index);
    void RTransRef(const QModelIndex& index);

    void RTreeViewCustomContextMenuRequested(const QPoint& pos);
    void RTreeViewDoubleClicked(const QModelIndex& index);

    void RSectionGroup(int id);
    void RTransRefDoubleClicked(const QModelIndex& index);

    void RStatementPrimary(int party_id, int unit, const QDateTime& start, const QDateTime& end);
    void RStatementSecondary(int party_id, int unit, const QDateTime& start, const QDateTime& end);

    void REnableAction(bool finished);

private:
    void SetTabWidget();

    void SetConnect() const;
    void SetAction() const;

    void SetFinanceData();
    void SetProductData();
    void SetStakeholderData();
    void SetTaskData();
    void SetSalesData();
    void SetPurchaseData();

    void CreateLeafFunction(int type, int node_id);
    void CreateSupportFunction(int type, int node_id);

    void CreateLeafFPTS(PNodeModel tree_model, TransWgtHash* trans_wgt_hash, CData* data, CSectionConfig* settings, int node_id);
    void CreateLeafO(PNodeModel tree_model, TransWgtHash* trans_wgt_hash, CData* data, CSectionConfig* settings, int node_id);

    void TableDelegateFPTS(PTableView table_view, PNodeModel tree_model, CSectionConfig* settings) const;
    void TableDelegateFPT(PTableView table_view, PNodeModel tree_model, CSectionConfig* settings, int node_id) const;
    void TableDelegateS(PTableView table_view) const;
    void TableDelegateO(PTableView table_view, CSectionConfig* settings) const;

    void SetTableView(PTableView table_view, int stretch_column) const;
    void TableConnectFPT(PTableView table_view, PTransModel table_model, PNodeModel tree_model) const;
    void TableConnectS(PTableView table_view, PTransModel table_model, PNodeModel tree_model) const;
    void TableConnectO(PTableView table_view, TransModelO* table_model, PNodeModel tree_model, TransWidgetO* widget) const;

    void CreateSupport(PNodeModel tree_model, SupWgtHash* sup_wgt_hash, CData* data, CSectionConfig* settings, int node_id);
    void DelegateSupport(PTableView table_view, PNodeModel tree_model, CSectionConfig* settings) const;

    void CreateTransRef(PNodeModel tree_model, CData* data, int node_id, int unit);
    void DelegateTransRef(PTableView table_view, CSectionConfig* settings) const;

    void DelegateSupportS(PTableView table_view, PNodeModel tree_model, PNodeModel product_tree_model) const;
    void SetSupportViewS(PTableView table_view) const;

    void SetStatementView(PTableView table_view, int stretch_column) const;
    void DelegateStatement(PTableView table_view, CSectionConfig* settings) const;
    void DelegateSettlement(PTableView table_view, CSectionConfig* settings) const;
    void DelegateSettlementPrimary(PTableView table_view, CSectionConfig* settings) const;

    void DelegateStatementPrimary(PTableView table_view, CSectionConfig* settings) const;
    void DelegateStatementSecondary(PTableView table_view, CSectionConfig* settings) const;

    void CreateSection(NodeWidget* node_widget, TransWgtHash& trans_wgt_hash, CData& data, CSectionConfig& settings, CString& name);
    void SwitchSection(CTab& last_tab) const;
    void UpdateLastTab() const;

    void SetTreeDelegate(PTreeView tree_view, CInfo& info, CSectionConfig& settings) const;
    void TreeDelegate(PTreeView tree_view, CInfo& info) const;
    void TreeDelegateF(PTreeView tree_view, CInfo& info, CSectionConfig& settings) const;
    void TreeDelegateT(PTreeView tree_view, CSectionConfig& settings) const;
    void TreeDelegateP(PTreeView tree_view, CSectionConfig& settings) const;
    void TreeDelegateS(PTreeView tree_view, CSectionConfig& settings) const;
    void TreeDelegateO(PTreeView tree_view, CSectionConfig& settings) const;

    void SetTreeView(PTreeView tree_view, CInfo& info) const;
    void TreeConnect(NodeWidget* node_widget, const Sqlite* sql) const;
    void TreeConnectFPT(PNodeModel node_model, const Sqlite* sql) const;
    void TreeConnectS(PNodeModel node_model, const Sqlite* sql) const;
    void TreeConnectPSO(PNodeModel node_order, const Sqlite* sql_order) const;

    void InsertNodeFunction(const QModelIndex& parent, int parent_id, int row);
    void InsertNodeFPTS(Node* node, const QModelIndex& parent, int parent_id, int row); // Finance Product Stakeholder Task
    void InsertNodeO(Node* node, const QModelIndex& parent, int row); // Purchase Sales

    void EditNodeFPTS(const QModelIndex& index, int node_id); // Finance Product Stakeholder Task

    void RemoveNode(NodeWidget* node_widget);
    void RemoveNonBranch(PNodeModel tree_model, const QModelIndex& index, int node_id, int node_type);
    void RemoveBranch(PNodeModel tree_model, const QModelIndex& index, int node_id);

    void UpdateStakeholderReference(QSet<int> stakeholder_nodes, bool branch) const;

    void LoadAndInstallTranslator(CString& language);
    void ResizeColumn(QHeaderView* header, int stretch_column) const;

    void ReadAppConfig();
    void ReadFileConfig(CString& complete_base_name);
    void ReadSectionConfig(SectionConfig& settings, CString& section_name);

    void UpdateAppConfig(CAppConfig& app_config);
    void UpdateFileConfig(CFileConfig& file_config);
    void UpdateSectionConfig(CSectionConfig& section_config);

    bool LockFile(const QFileInfo& file_info);

    void RestoreTab(PNodeModel tree_model, TransWgtHash& trans_wgt_hash, CIntSet& set, CData& data, CSectionConfig& section_settings);

    void EnableAction(bool enable) const;

    QStandardItemModel* CreateModelFromMap(CStringMap& map, QObject* parent = nullptr);

    void IniSectionGroup();
    void TransRefP(int node_id, int unit);
    void TransRefS(int node_id, int unit);
    void OrderNodeLocation(Section section, int node_id);

    void LeafToSupport(TransWidget* widget);
    void SupportToLeaf(SupportWidget* widget);

    void SwitchToLeaf(int node_id, int trans_id = 0) const;
    void SwitchToSupport(int node_id, int trans_id = 0) const;

    void OrderTransLocation(int node_id);
    void RegisterRptWgt(ReportWidget* widget);

    void VerifyActivationOffline();
    void VerifyActivationOnline();

private:
    Ui::MainWindow* ui {};

    QStringList recent_file_ {};
    Section start_ {};
    int report_id_ { -1 };

    QPointer<SettlementWidget> settlement_widget_ {};
    QMap<QString, QString> print_template_ {};

    LicenseConfig license_config_ {};
    QSharedPointer<QSettings> license_settings_ {};

    QTranslator qt_translator_ {};
    QTranslator ytx_translator_ {};

    AppConfig app_config_ {};
    FileConfig file_config_ {};
    LoginConfig login_config_ {};

    QScopedPointer<QLockFile> lock_file_ {};

    QSharedPointer<QSettings> app_settings_ {};
    QSharedPointer<QSettings> file_settings_ {};

    QButtonGroup* section_group_ {};

    NodeWidget* node_widget_ {};
    TransWgtHash* trans_wgt_hash_ {};
    QList<PDialog>* dialog_list_ {};
    SectionConfig* section_config_ {};
    Data* data_ {};
    SupWgtHash* sup_wgt_hash_ {};
    RptWgtHash* rpt_wgt_hash_ {};

    NodeWidget* finance_tree_ {};
    TransWgtHash finance_trans_wgt_hash_ {};
    QList<PDialog> finance_dialog_list_ {};
    SectionConfig finance_config_ {};
    Data finance_data_ {};
    SupWgtHash finance_sup_wgt_hash_ {};

    NodeWidget* product_tree_ {};
    TransWgtHash product_trans_wgt_hash_ {};
    QList<PDialog> product_dialog_list_ {};
    SectionConfig product_config_ {};
    Data product_data_ {};
    SupWgtHash product_sup_wgt_hash_ {};
    RptWgtHash product_rpt_wgt_hash_ {};

    NodeWidget* task_tree_ {};
    TransWgtHash task_trans_wgt_hash_ {};
    QList<PDialog> task_dialog_list_ {};
    SectionConfig task_config_ {};
    Data task_data_ {};
    SupWgtHash task_sup_wgt_hash_ {};

    NodeWidget* stakeholder_tree_ {};
    TransWgtHash stakeholder_trans_wgt_hash_ {};
    QList<PDialog> stakeholder_dialog_list_ {};
    SectionConfig stakeholder_config_ {};
    Data stakeholder_data_ {};
    SupWgtHash stakeholder_sup_wgt_hash_ {};
    RptWgtHash stakeholder_rpt_wgt_hash_ {};

    NodeWidget* sales_tree_ {};
    TransWgtHash sales_trans_wgt_hash_ {};
    QList<PDialog> sales_dialog_list_ {};
    SectionConfig sales_config_ {};
    Data sales_data_ {};
    RptWgtHash sales_rpt_wgt_hash_ {};

    NodeWidget* purchase_tree_ {};
    TransWgtHash purchase_trans_wgt_hash_ {};
    QList<PDialog> purchase_dialog_list_ {};
    SectionConfig purchase_config_ {};
    Data purchase_data_ {};
    RptWgtHash purchase_rpt_wgt_hash_ {};
};
#endif // MAINWINDOW_H
