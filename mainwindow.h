/*
 * Copyright (C) 2023 YtxErp
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
#include "database/ytxsqlite.h"
#include "table/model/transmodel.h"
#include "table/model/transmodelo.h"
#include "tree/model/nodemodel.h"
#include "ui_mainwindow.h"
#include "widget/node/nodewidget.h"
#include "widget/support/supportwidget.h"
#include "widget/trans/transwidgeto.h"

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

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void on_actionRemove_triggered();
    void on_actionAppendNode_triggered();
    void on_actionEditNode_triggered();
    void on_actionJump_triggered();
    void on_actionSupportJump_triggered();
    void on_actionAbout_triggered();
    void on_actionPreferences_triggered();
    void on_actionSearch_triggered();
    void on_actionClearMenu_triggered();
    void on_actionNewFile_triggered();
    void on_actionOpenFile_triggered();
    void on_actionExportYTX_triggered();
    void on_actionExportExcel_triggered();
    void on_actionStatement_triggered();

    void on_tabWidget_currentChanged(int index);
    void on_tabWidget_tabBarDoubleClicked(int index);
    void on_tabWidget_tabCloseRequested(int index);

    void RNodeLocation(int node_id);
    void RTransLocation(int trans_id, int lhs_node_id, int rhs_node_id);

    void RUpdateSettings(const Settings& settings, const Interface& interface);
    void RSyncInt(int node_id, int column, const QVariant& value);
    void RUpdateName(int node_id, const QString& name, bool branch);
    void RUpdateState();

    void RFreeWidget(int node_id);
    void REditTransDocument(const QModelIndex& index);
    void REditNodeDocument(const QModelIndex& index);
    void RTransRef(const QModelIndex& index);

    void RTreeViewCustomContextMenuRequested(const QPoint& pos);
    void RTreeViewDoubleClicked(const QModelIndex& index);

    void RSectionGroup(int id);
    void RTransRefDoubleClicked(const QModelIndex& index);

    void RPrimaryStatement(int party_id, QDateTime start, QDateTime end, double pbalance, double cbalance);
    void RSecondaryStatement(int party_id, QDateTime start, QDateTime end, double pbalance, double cbalance);

private:
    void SetTabWidget();
    void SetClearMenuAction();

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

    void CreateLeafFPTS(PTreeModel tree_model, TransWgtHash* trans_wgt_hash, CData* data, CSettings* settings, int node_id);
    void CreateLeafO(PTreeModel tree_model, TransWgtHash* trans_wgt_hash, CData* data, CSettings* settings, int node_id);
    void DelegateFPTS(PTableView table_view, PTreeModel tree_model, CSettings* settings) const;
    void DelegateFPT(PTableView table_view, PTreeModel tree_model, CSettings* settings, int node_id) const;
    void DelegateS(PTableView table_view) const;
    void DelegateO(PTableView table_view, CSettings* settings) const;
    void SetTableView(PTableView table_view, int stretch_column) const;

    void CreateSupport(PTreeModel tree_model, SupWgtHash* sup_wgt_hash, CData* data, CSettings* settings, int node_id);
    void DelegateSupport(PTableView table_view, PTreeModel tree_model, CSettings* settings) const;

    void CreateTransRef(PTreeModel tree_model, SupWgtHash* sup_wgt_hash, CData* data, int node_id);
    void DelegateTransRef(PTableView table_view, CSettings* settings) const;

    void DelegateSupportS(PTableView table_view, PTreeModel tree_model, PTreeModel product_tree_model) const;
    void SetSupportViewS(PTableView table_view) const;

    void SetStatementView(PTableView table_view) const;
    void DelegateStatement(PTableView table_view, CSettings* settings) const;

    void TableConnectFPT(PTableView table_view, PTableModel table_model, PTreeModel tree_model, CData* data) const;
    void TableConnectO(PTableView table_view, TransModelO* table_model, PTreeModel tree_model, TransWidgetO* widget) const;
    void TableConnectS(PTableView table_view, PTableModel table_model, PTreeModel tree_model, CData* data) const;

    void CreateSection(NodeWidget* tree_widget, TransWgtHash& trans_wgt_hash, CData& data, CSettings& settings, CString& name);
    void SwitchSection(CTab& last_tab) const;
    void UpdateLastTab() const;

    void SetDelegate(PTreeView tree_view, CInfo& info, CSettings& settings) const;
    void DelegateFPTSO(PTreeView tree_view, CInfo& info) const;
    void DelegateF(PTreeView tree_view, CInfo& info, CSettings& settings) const;
    void DelegateT(PTreeView tree_view, CSettings& settings) const;
    void DelegateP(PTreeView tree_view, CSettings& settings) const;
    void DelegateS(PTreeView tree_view, CSettings& settings) const;
    void DelegateO(PTreeView tree_view, CInfo& info, CSettings& settings) const;

    void SetTreeView(PTreeView tree_view, CInfo& info) const;
    void TreeConnect(NodeWidget* tree_widget, const Sqlite* sql) const;

    void InsertNodeFunction(const QModelIndex& parent, int parent_id, int row);
    void InsertNodeFPTS(Node* node, const QModelIndex& parent, int parent_id, int row); // Finance Product Stakeholder Task
    void InsertNodeO(Node* node, const QModelIndex& parent, int row); // Purchase Sales

    void EditNodeFPTS(const QModelIndex& index, int node_id); // Finance Product Stakeholder Task

    void RemoveTrans(TransWidget* leaf_widget);
    void RemoveNode(NodeWidget* tree_widget);
    void RemoveNonBranch(PTreeModel tree_model, const QModelIndex& index, int node_id, int node_type);
    void RemoveBranch(PTreeModel tree_model, const QModelIndex& index, int node_id);

    void UpdateInterface(CInterface& interface);
    void UpdateStakeholderReference(QSet<int> stakeholder_nodes, bool branch) const;

    void LoadAndInstallTranslator(CString& language);
    void ResizeColumn(QHeaderView* header, int stretch_column) const;

    void AppSettings();
    bool LockFile(const QFileInfo& file_info);

    void RestoreTab(PTreeModel tree_model, TransWgtHash& trans_wgt_hash, CIntSet& set, CData& data, CSettings& settings);

    void EnableAction(bool enable) const;
    void RestoreRecentFile();
    void AddRecentFile(CString& file_path);

    QStandardItemModel* CreateModelFromList(QStringList& list, QObject* parent = nullptr);

    void IniSectionGroup();
    void TransRefP(int node_id, int unit);
    void TransRefS(int node_id, int unit);
    void SalesNodeLocation(int node_id);

    void LeafToSupport(TransWidget* widget);
    void SupportToLeaf(SupportWidget* widget);
    void SwitchToLeaf(int node_id, int trans_id = 0) const;
    void SwitchToSupport(int node_id, int trans_id = 0) const;

    void OrderTransLocation(int node_id);

private:
    Ui::MainWindow* ui {};

    QStringList recent_file_ {};
    Section start_ {};
    int statement_id { -1 };

    QTranslator qt_translator_ {};
    QTranslator ytx_translator_ {};
    Interface interface_ {};

    std::unique_ptr<QLockFile> lock_file_;
    std::unique_ptr<YtxSqlite> ytx_sql_ {};
    std::shared_ptr<QSettings> app_settings_ {};
    std::shared_ptr<QSettings> file_settings_ {};

    QButtonGroup* section_group_ {};

    NodeWidget* tree_widget_ {};
    TransWgtHash* trans_wgt_hash_ {};
    QList<PDialog>* dialog_list_ {};
    QHash<int, PDialog>* dialog_hash_ {};
    Settings* settings_ {};
    Data* data_ {};
    SupWgtHash* sup_wgt_hash_ {};

    NodeWidget* finance_tree_ {};
    TransWgtHash finance_trans_wgt_hash_ {};
    QList<PDialog> finance_dialog_list_ {};
    QHash<int, PDialog> finance_dialog_hash_ {};
    Settings finance_settings_ {};
    Data finance_data_ {};
    SupWgtHash finance_sup_wgt_hash_ {};

    NodeWidget* product_tree_ {};
    TransWgtHash product_trans_wgt_hash_ {};
    QList<PDialog> product_dialog_list_ {};
    QHash<int, PDialog> product_dialog_hash_ {};
    Settings product_settings_ {};
    Data product_data_ {};
    SupWgtHash product_sup_wgt_hash_ {};

    NodeWidget* task_tree_ {};
    TransWgtHash task_trans_wgt_hash_ {};
    QList<PDialog> task_dialog_list_ {};
    QHash<int, PDialog> task_dialog_hash_ {};
    Settings task_settings_ {};
    Data task_data_ {};
    SupWgtHash task_sup_wgt_hash_ {};

    NodeWidget* stakeholder_tree_ {};
    TransWgtHash stakeholder_trans_wgt_hash_ {};
    QList<PDialog> stakeholder_dialog_list_ {};
    QHash<int, PDialog> stakeholder_dialog_hash_ {};
    Settings stakeholder_settings_ {};
    Data stakeholder_data_ {};
    SupWgtHash stakeholder_sup_wgt_hash_ {};

    NodeWidget* sales_tree_ {};
    TransWgtHash sales_trans_wgt_hash_ {};
    QList<PDialog> sales_dialog_list_ {};
    QHash<int, PDialog> sales_dialog_hash_ {};
    Settings sales_settings_ {};
    Data sales_data_ {};
    SupWgtHash sales_sup_wgt_hash_ {};

    NodeWidget* purchase_tree_ {};
    TransWgtHash purchase_trans_wgt_hash_ {};
    QList<PDialog> purchase_dialog_list_ {};
    QHash<int, PDialog> purchase_dialog_hash_ {};
    Settings purchase_settings_ {};
    Data purchase_data_ {};
    SupWgtHash purchase_sup_wgt_hash_ {};
};
#endif // MAINWINDOW_H
