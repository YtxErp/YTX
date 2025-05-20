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

#ifndef LOGIN_H
#define LOGIN_H

#include <QDialog>
#include <QSettings>

#include "component/config.h"
#include "component/using.h"

namespace Ui {
class Login;
}

class Login final : public QDialog {
    Q_OBJECT

signals:
    void SLoadDatabase();

public:
    explicit Login(LoginConfig& login_config, QSharedPointer<QSettings> app_settings, QWidget* parent = nullptr);
    ~Login();

private slots:
    void on_pushButtonConnect_clicked();

private:
    void InitDialog();
    void SaveLoginConfig(CString& host, int port, CString& user, CString& password, CString& database);

private:
    Ui::Login* ui;
    LoginConfig& login_config_;
    QSharedPointer<QSettings> app_settings_ {};
};

#endif // LOGIN_H
