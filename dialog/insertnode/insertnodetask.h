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

#ifndef INSERTNODETASK_H
#define INSERTNODETASK_H

#include <QButtonGroup>
#include <QDialog>

#include "component/arg/insertnodeargfpts.h"
#include "component/using.h"

namespace Ui {
class InsertNodeTask;
}

class InsertNodeTask final : public QDialog {
    Q_OBJECT

public:
    InsertNodeTask(CInsertNodeArgFPTS& arg, int amount_decimal, CString& display_format, QWidget* parent = nullptr);
    ~InsertNodeTask();

private slots:
    void RNameEdited(const QString& arg1);
    void RRuleGroupClicked(int id);
    void RTypeGroupClicked(int id);

    void on_lineEditName_editingFinished();
    void on_lineEditCode_editingFinished();
    void on_lineEditDescription_editingFinished();
    void on_dSpinBoxUnitCost_editingFinished();

    void on_comboUnit_currentIndexChanged(int index);

    void on_plainTextEdit_textChanged();

    void on_pBtnColor_clicked();

    void on_chkBoxFinished_checkStateChanged(const Qt::CheckState& arg1);

    void on_dateTime_editingFinished();

private:
    void IniDialog(QStandardItemModel* unit_model, int amount_decimal, CString& display_format);
    void IniData(Node* node);
    void IniConnect();
    void UpdateColor(QColor color);
    void IniTypeGroup();
    void IniRuleGroup();
    void IniRule(bool rule);

private:
    Ui::InsertNodeTask* ui;
    Node* node_ {};
    QButtonGroup* rule_group_ {};
    QButtonGroup* type_group_ {};

    CString& parent_path_ {};
    CStringList& name_list_ {};
};

#endif // INSERTNODETASK_H
