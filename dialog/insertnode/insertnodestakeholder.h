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

#ifndef INSERTNODESTAKEHOLDER_H
#define INSERTNODESTAKEHOLDER_H

#include <QButtonGroup>
#include <QDialog>

#include "component/arg/insertnodeargfpts.h"
#include "component/using.h"

namespace Ui {
class InsertNodeStakeholder;
}

class InsertNodeStakeholder final : public QDialog {
    Q_OBJECT

public:
    InsertNodeStakeholder(CInsertNodeArgFPTS& arg, QStandardItemModel* employee_model, int amount_decimal, QWidget* parent = nullptr);
    ~InsertNodeStakeholder();

private slots:
    void RNameEdited(const QString& arg1);
    void RRuleGroupClicked(int id);
    void RTypeGroupClicked(int id);

    void on_lineEditName_editingFinished();
    void on_lineEditCode_editingFinished();
    void on_lineEditDescription_editingFinished();
    void on_dSpinPaymentPeriod_editingFinished();
    void on_dSpinTaxRate_editingFinished();

    void on_comboUnit_currentIndexChanged(int index);
    void on_comboEmployee_currentIndexChanged(int index);

    void on_plainTextEdit_textChanged();
    void on_deadline_editingFinished();

private:
    void IniDialog(QStandardItemModel* unit_model, QStandardItemModel* employee_model, int common_decimal);
    void IniConnect();
    void IniData();
    void IniTypeGroup();

private:
    Ui::InsertNodeStakeholder* ui;
    Node* node_ {};
    QButtonGroup* type_group_ {};

    CString& parent_path_ {};
    CStringList& name_list_ {};
};

#endif // INSERTNODESTAKEHOLDER_H
