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

#ifndef INSERTNODEORDER_H
#define INSERTNODEORDER_H

#include <QButtonGroup>
#include <QDialog>

#include "component/classparams.h"
#include "component/settings.h"
#include "tree/model/treemodelstakeholder.h"

namespace Ui {
class InsertNodeOrder;
}

class InsertNodeOrder final : public QDialog {
    Q_OBJECT

public:
    InsertNodeOrder(CEditNodeParamsO& params, QWidget* parent = nullptr);
    ~InsertNodeOrder();

signals:
    // send to TableModelOrder
    void SSyncInt(int node_id, int column, int value);

    // send to TableModelOrder, TreeModelOrder
    void SSyncBool(int node_id, int column, bool value);

    // send to TreeModelOrder
    void SUpdateLeafValue(int node_id, double initial_delta, double final_delta, double first_delta, double second_delta, double discount_delta);

public slots:
    void accept() override;

    // receive from TableModelOrder
    void RUpdateLeafValue(int node_id, double initial_delta, double final_delta, double first_delta, double second_delta, double discount_delta);

    // receive from TreeModelOrder
    void RSyncBool(int node_id, int column, bool value);
    void RSyncInt(int node_id, int column, int value);
    void RSyncString(int node_id, int column, const QString& value);

public:
    QPointer<TableModel> Model();
    QPointer<QTableView> View();

private slots:
    void on_comboParty_editTextChanged(const QString& arg1);
    void on_comboParty_currentIndexChanged(int index);
    void on_comboEmployee_currentIndexChanged(int index);

    void on_pBtnInsert_clicked();
    void on_pBtnFinishOrder_toggled(bool checked);

    void on_dateTimeEdit_dateTimeChanged(const QDateTime& date_time);
    void on_lineDescription_editingFinished();

    void on_chkBoxBranch_checkStateChanged(const Qt::CheckState& arg1);

    void RRuleGroupClicked(int id);
    void RUnitGroupClicked(int id);

private:
    void IniDialog(CSettings* settings);
    void IniConnect();
    void IniUnit(int unit);
    void IniRule(bool rule);
    void IniDataCombo(int party, int employee);
    void IniLeafValue();
    void IniText(Section section);
    void IniFinished(bool finished);
    void IniUnitGroup();
    void IniRuleGroup();

    void LockWidgets(bool finished, bool branch);

private:
    Ui::InsertNodeOrder* ui;

    Node* node_ {};
    Sqlite* sql_ {};
    TreeModelStakeholder* stakeholder_tree_ {};
    TableModel* order_table_ {};
    QButtonGroup* rule_group_ {};
    QButtonGroup* unit_group_ {};

    QStandardItemModel* combo_model_employee_ {};
    QStandardItemModel* combo_model_party_ {};

    const QString info_node_ {};
    const int party_unit_ {};

    int node_id_ {};
};

#endif // INSERTNODEORDER_H
