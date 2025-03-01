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

#ifndef INSERTNODETASK_H
#define INSERTNODETASK_H

#include <QDialog>

#include "component/classparams.h"
#include "component/using.h"

namespace Ui {
class InsertNodeTask;
}

class InsertNodeTask final : public QDialog {
    Q_OBJECT

public:
    InsertNodeTask(CInsertNodeParamsFPTS& params, int amount_decimal, CString& display_format, QWidget* parent = nullptr);
    ~InsertNodeTask();

private slots:
    void RNameEdited(const QString& arg1);

    void on_lineEditName_editingFinished();
    void on_lineEditCode_editingFinished();
    void on_lineEditDescription_editingFinished();
    void on_dSpinBoxUnitCost_editingFinished();

    void on_comboUnit_currentIndexChanged(int index);

    void on_rBtnDDCI_toggled(bool checked);

    void on_plainTextEdit_textChanged();

    void on_rBtnLeaf_toggled(bool checked);
    void on_rBtnBranch_toggled(bool checked);
    void on_rBtnSupport_toggled(bool checked);

    void on_pBtnColor_clicked();

    void on_chkBoxFinished_checkStateChanged(const Qt::CheckState& arg1);

    void on_dateTime_editingFinished();

private:
    void IniDialog(QStandardItemModel* unit_model, int amount_decimal, CString& display_format);
    void IniData(Node* node);
    void IniConnect();
    void UpdateColor(QColor color);

private:
    Ui::InsertNodeTask* ui;
    Node* node_ {};

    CString& parent_path_ {};
    CStringList& name_list_ {};
};

#endif // INSERTNODETASK_H
