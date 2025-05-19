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

#ifndef TRANSWIDGETO_H
#define TRANSWIDGETO_H

#include <QButtonGroup>

#include "component/arg/insertnodeargo.h"
#include "database/sqlite/sqliteo.h"
#include "report/printmanager.h"
#include "table/model/transmodelo.h"

namespace Ui {
class TransWidgetO;
}

class TransWidgetO final : public TransWidget {
    Q_OBJECT

public:
    TransWidgetO(CInsertNodeArgO& arg, const QMap<QString, QString>& print_template, QSharedPointer<PrintManager> print_manager, QWidget* parent = nullptr);
    ~TransWidgetO();

signals:
    // send to TableModelOrder, MainWindow
    void SSyncInt(int node_id, int column, int value);

    // send to TreeModelOrder, finished
    void SSyncBoolNode(int node_id, int column, bool value);

    // send to TableModelOrder, finished, rule
    void SSyncBoolTrans(int node_id, int column, bool value);

    // send to TreeModelOrder
    void SSyncLeafValue(int node_id, double initial_delta, double final_delta, double first_delta, double second_delta, double discount_delta);

    // send to MainWindow
    void SEnableAction(bool finished);

public slots:
    // receive from TreeModelOrder
    void RSyncBoolNode(int node_id, int column, bool value);
    void RSyncInt(int node_id, int column, int value);
    void RSyncString(int node_id, int column, const QString& value);

    // receive from TableModelOrder
    void RSyncLeafValue(int node_id, double initial_delta, double final_delta, double first_delta, double second_delta, double discount_delta);

public:
    QPointer<TransModel> Model() const override { return order_trans_; }
    QPointer<QTableView> View() const override;

private slots:

    void on_comboParty_currentIndexChanged(int index);
    void on_comboEmployee_currentIndexChanged(int index);

    void on_pBtnFinishOrder_toggled(bool checked);
    void on_pBtnInsert_clicked();

    void on_dateTimeEdit_dateTimeChanged(const QDateTime& date_time);
    void on_lineDescription_editingFinished();

    void on_pBtnPreview_clicked();
    void on_pBtnPrint_clicked();

    void RRuleGroupClicked(int id);
    void RUnitGroupClicked(int id);

private:
    void IniWidget();
    void IniData();
    void IniConnect();
    void IniDataCombo(int party, int employee);
    void LockWidgets(bool finished);
    void IniUnit(int unit);
    void IniLeafValue();
    void IniText(Section section);
    void IniRule(bool rule);
    void IniFinished(bool finished);
    void IniRuleGroup();
    void IniUnitGroup();

    void PreparePrint();

private:
    Ui::TransWidgetO* ui;
    Node* node_ {};
    SqliteO* sql_ {};
    TransModelO* order_trans_ {};
    NodeModel* stakeholder_node_ {};
    CSectionConfig* settings_ {};
    QButtonGroup* rule_group_ {};
    QButtonGroup* unit_group_ {};

    QSortFilterProxyModel* emodel_ {};
    QSortFilterProxyModel* pmodel_ {};

    const int node_id_ {};
    const int party_unit_ {};
    const QString party_info_ {};

    const QMap<QString, QString>& print_template_ {};
    QSharedPointer<PrintManager> print_manager_ {};
};

#endif // TRANSWIDGETO_H
