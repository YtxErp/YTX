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

#ifndef NEWDATABASE_H
#define NEWDATABASE_H

#include <QDialog>

namespace Ui {
class NewDatabase;
}

class NewDatabase : public QDialog {
    Q_OBJECT

public:
    explicit NewDatabase(QWidget* parent = nullptr);
    ~NewDatabase();

private slots:
    void on_pushButtonCreate_clicked();

private:
    Ui::NewDatabase* ui;
};

#endif // NEWDATABASE_H
