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

#ifndef TABLEWIDGETORDER_H
#define TABLEWIDGETORDER_H

#include "component/classparams.h"
#include "component/settings.h"
#include "table/model/tablemodel.h"
#include "tree/model/treemodelstakeholder.h"
#include "widget/tablewidget/tablewidget.h"

namespace Ui {
class TableWidgetOrder;
}

class TableWidgetOrder final : public TableWidget {
    Q_OBJECT

public:
    TableWidgetOrder(CEditNodeParamsOrder& params, QWidget* parent = nullptr);
    ~TableWidgetOrder();

signals:
    // send to TableModelOrder, MainWindow
    void SSyncInt(int node_id, int column, int value);

    // send to TableModelOrder, TreeModelOrder
    void SSyncBool(int node_id, int column, bool value);

    // send to TreeModelOrder
    void SUpdateLeafValue(int node_id, double initial_delta, double final_delta, double first_delta, double second_delta, double discount_delta);

public slots:
    // receive from TreeModelOrder
    void RSyncBool(int node_id, int column, bool value);
    void RSyncInt(int node_id, int column, int value);
    void RSyncString(int node_id, int column, const QString& value);

    // receive from TableModelOrder
    void RUpdateLeafValue(int node_id, double initial_delta, double final_delta, double first_delta, double second_delta, double discount_delta);

public:
    QPointer<TableModel> Model() const override { return order_table_; }
    QPointer<QTableView> View() const override;

private slots:

    void on_comboParty_currentIndexChanged(int index);
    void on_comboEmployee_currentIndexChanged(int index);

    void on_chkBoxRefund_toggled(bool checked);
    void on_pBtnFinishOrder_toggled(bool checked);
    void on_rBtnCash_toggled(bool checked);
    void on_rBtnMonthly_toggled(bool checked);
    void on_rBtnPending_toggled(bool checked);

    void on_pBtnInsert_clicked();
    void on_dateTimeEdit_dateTimeChanged(const QDateTime& date_time);
    void on_lineDescription_editingFinished();

private:
    void IniDialog();
    void IniData();
    void IniDataCombo(int party, int employee);
    void LockWidgets(bool finished);
    void IniUnit(int unit);

private:
    Ui::TableWidgetOrder* ui;
    NodeShadow* node_shadow_ {};
    Sqlite* sql_ {};
    TableModel* order_table_ {};
    TreeModelStakeholder* stakeholder_tree_ {};
    CSettings* settings_ {};

    QStandardItemModel* emodel_ {};
    QStandardItemModel* pmodel_ {};

    const int node_id_ {};
    const QString info_node_ {};
    int party_unit_ {};
};

#endif // TABLEWIDGETORDER_H
