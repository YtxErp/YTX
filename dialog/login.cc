#include "login.h"

#include <QDir>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QStandardPaths>

#include "component/constvalue.h"
#include "component/signalblocker.h"
#include "database/websocket.h"
#include "ui_login.h"

Login::Login(LoginInfo& login_info, LicenseInfo& license_info, QSharedPointer<QSettings> app_settings, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::Login)
    , login_info_ { login_info }
    , license_info_ { license_info }
    , app_settings_ { app_settings }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    IniDialog();
    IniConnect();
}

Login::~Login() { delete ui; }

void Login::RLoginResult(bool success)
{
    if (success) {
        emit SLoadDatabase(GetCacheFilePath(login_info_.user, login_info_.database));
        SaveLoginConfig();
        this->close();
    } else {
        QMessageBox::critical(this, tr("Connection Failed"),
            tr("Unable to connect to the database. Please check if the PostgreSQL service is running, "
               "and verify the username, password, and database name."));
    }
}

void Login::on_pushButtonConnect_clicked()
{
    login_info_.host = ui->lineEditHost->text();
    login_info_.port = ui->lineEditPort->text().toInt();
    login_info_.user = ui->lineEditUser->text();
    login_info_.password = ui->lineEditPassword->text();
    login_info_.database = ui->lineEditDatabase->text();

    if (!license_info_.is_activated) {
        QMessageBox::critical(this, tr("Activation Required"), tr("The software is not activated. Please activate it first."));
        return;
    }

    WebSocket::Instance().Connect(login_info_.host, login_info_.port, login_info_.user, login_info_.password, login_info_.database);
}

void Login::IniDialog()
{
    ui->lineEditUser->setText(login_info_.user);
    ui->lineEditDatabase->setText(login_info_.database);
    ui->lineEditPassword->setText(login_info_.password);
    ui->chkBoxSave->setChecked(login_info_.is_saved);
    ui->lineEditHost->setText(login_info_.host);
    ui->lineEditPort->setText(QString::number(login_info_.port));
}

void Login::IniConnect() { connect(&WebSocket::Instance(), &WebSocket::SLoginResult, this, &Login::RLoginResult); }

void Login::SaveLoginConfig()
{
    const bool is_saved { ui->chkBoxSave->isChecked() };

    login_info_.is_saved = is_saved;
    if (!is_saved)
        login_info_.password = QString();

    app_settings_->beginGroup(kLogin);
    app_settings_->setValue(kHost, login_info_.host);
    app_settings_->setValue(kPort, login_info_.port);
    app_settings_->setValue(kUser, login_info_.user);
    app_settings_->setValue(kPassword, login_info_.password);
    app_settings_->setValue(kDatabase, login_info_.database);
    app_settings_->setValue(kIsSaved, is_saved);
    app_settings_->endGroup();
}

QString Login::GetCacheFilePath(const QString& user, const QString& database)
{
    const QString file_name { QString("%1@%2.cache").arg(user, database) };
    const QString base_path { QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) };
    const QString cache_path { QDir(base_path).filePath("cache") };

    QDir().mkpath(cache_path);
    return QDir(cache_path).filePath(file_name);
}
