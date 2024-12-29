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

#ifndef EDITDOCUMENT_H
#define EDITDOCUMENT_H

#include <QDialog>
#include <QStringListModel>

#include "component/using.h"

namespace Ui {
class EditDocument;
}

class EditDocument final : public QDialog {
    Q_OBJECT

public:
    explicit EditDocument(QStringList* document, CString& document_dir, QWidget* parent = nullptr);
    ~EditDocument();

private slots:
    void on_pBtnAdd_clicked();
    void on_pBtnRemove_clicked();
    void on_pBtnOk_clicked();
    void on_listView_doubleClicked(const QModelIndex& index);

private:
    void CreateList(QStringList* document);

private:
    Ui::EditDocument* ui;
    QStringList* document_ {};
    QStringListModel* list_model_ {};
    QString document_dir_ {};
};

#endif // EDITDOCUMENT_H
