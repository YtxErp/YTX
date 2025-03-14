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

#ifndef EDITNODENAME_H
#define EDITNODENAME_H

#include <QDialog>

#include "component/using.h"

namespace Ui {
class EditNodeName;
}

class EditNodeName final : public QDialog {
    Q_OBJECT

public:
    EditNodeName(CString& name, CString& parent_path, CStringList& children_name, QWidget* parent = nullptr);
    ~EditNodeName();

private slots:
    void RNameEdited(const QString& arg1);

public:
    QString GetName() const;

private:
    void IniDialog(CString& name);
    void IniConnect();
    void IniData(CString& name);

private:
    Ui::EditNodeName* ui;
    CString& parent_path_ {};
    CStringList& name_list_ {};
};

#endif // EDITNODENAME_H
