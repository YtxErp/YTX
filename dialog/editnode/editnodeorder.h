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

#ifndef EDITNODEORDER_H
#define EDITNODEORDER_H

#include <QDialog>

#include "component/classparams.h"
#include "component/settings.h"
#include "tree/model/treemodelstakeholder.h"

namespace Ui {
class EditNodeOrder;
}

class EditNodeOrder final : public QDialog {
    Q_OBJECT

public:
    EditNodeOrder(CEditNodeParamsOrder& params, QWidget* parent = nullptr);
    ~EditNodeOrder();

signals:
    // send to TableModelOrder and TreeModelOrder
    void SSyncOneValue(int node_id, int column, const QVariant& value);

    // send to TableModelOrder
    void SSyncFinished(int node_id, int column, const QVariant& value);

    // send to TreeModelOrder
    void SUpdateLeafValue(int node_id, double first_diff, double second_diff, double gross_amount_diff, double discount_diff, double net_amount_diff);

public slots:
    void accept() override;
    void RUpdateLeafValue(int node_id, double first_diff, double second_diff, double gross_amount_diff, double discount_diff, double net_amount_diff);
    void RSyncOneValue(int node_id, int column, const QVariant& value);

public:
    QPointer<TableModel> Model();
    QPointer<QTableView> View();

private slots:
    void on_comboParty_editTextChanged(const QString& arg1);
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

    void on_chkBoxBranch_checkStateChanged(const Qt::CheckState& arg1);

private:
    void IniDialog(CSettings* settings);
    void IniConnect();
    void LockWidgets(bool finished, bool branch);
    void IniUnit(int unit);
    void IniDataCombo(int party, int employee);

private:
    Ui::EditNodeOrder* ui;

    NodeShadow* node_shadow_ {};
    Sqlite* sql_ {};
    TreeModelStakeholder* stakeholder_tree_ {};
    TableModel* order_table_ {};

    QStandardItemModel* combo_model_employee_ {};
    QStandardItemModel* combo_model_party_ {};

    const QString info_node_ {};
    const int party_unit_ {};

    int node_id_ {};
};

#endif // EDITNODEORDER_H
