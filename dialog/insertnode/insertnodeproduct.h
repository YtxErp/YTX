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

#ifndef INSERTNODEPRODUCT_H
#define INSERTNODEPRODUCT_H

#include <QDialog>

#include "component/classparams.h"
#include "component/using.h"

namespace Ui {
class InsertNodeProduct;
}

class InsertNodeProduct final : public QDialog {
    Q_OBJECT

public:
    InsertNodeProduct(CEditNodeParamsFPTS& params, int amount_decimal, QWidget* parent = nullptr);
    ~InsertNodeProduct();

private slots:
    void RNameEdited(const QString& arg1);

    void on_lineEditName_editingFinished();
    void on_lineEditCode_editingFinished();
    void on_lineEditDescription_editingFinished();
    void on_dSpinBoxUnitPrice_editingFinished();
    void on_dSpinBoxCommission_editingFinished();

    void on_comboUnit_currentIndexChanged(int index);

    void on_rBtnDDCI_toggled(bool checked);

    void on_plainTextEdit_textChanged();

    void on_rBtnLeaf_toggled(bool checked);
    void on_rBtnBranch_toggled(bool checked);
    void on_rBtnSupport_toggled(bool checked);

    void on_pBtnColor_clicked();

private:
    void IniDialog(QStandardItemModel* unit_model, int amount_decimal);
    void IniConnect();
    void IniData(Node* node);
    void UpdateColor(QColor color);

private:
    Ui::InsertNodeProduct* ui;
    Node* node_ {};

    CString& parent_path_ {};
    CStringList& name_list_ {};
};

#endif // INSERTNODEPRODUCT_H
