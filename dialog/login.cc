#include "login.h"

#include <QMessageBox>
#include <QSqlDatabase>

#include "component/constvalue.h"
#include "global/pgconnectionpool.h"
#include "ui_login.h"

Login::Login(LoginConfig& login_config, QSharedPointer<QSettings> app_settings, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::Login)
    , login_config_ { login_config }
    , app_settings_ { app_settings }
{
    ui->setupUi(this);
    InitDialog();
}

Login::~Login() { delete ui; }

void Login::on_pushButtonConnect_clicked()
{
    const auto host { ui->lineEditHost->text() };
    const int port { ui->lineEditPort->text().toInt() };
    const auto user { ui->lineEditUser->text() };
    const auto password { ui->lineEditPassword->text() };
    const auto database { ui->lineEditDatabase->text() };

    const bool ok { PGConnectionPool::Instance().Initialize(host, port, user, password, database) };

    if (ok) {
        SaveLoginConfig(host, port, user, password, database);
        this->close();
    } else {
        QMessageBox::critical(this, tr("Connection Failed"),
            tr("Unable to connect to the database. Please check if the PostgreSQL service is running, "
               "and verify the username, password, and database name."));
    }
}

void Login::InitDialog()
{
    ui->lineEditUser->setText(login_config_.user);
    ui->lineEditDatabase->setText(login_config_.database);
    ui->lineEditPassword->setText(login_config_.password);
    ui->chkBoxSave->setChecked(login_config_.is_saved);
    ui->lineEditHost->setText(login_config_.host);
    ui->lineEditPort->setText(QString::number(login_config_.port));
}

void Login::SaveLoginConfig(CString& host, int port, CString& user, CString& password, CString& database)
{
    login_config_.host = host;
    login_config_.port = port;
    login_config_.user = user;
    login_config_.database = database;
    login_config_.is_saved = ui->chkBoxSave->isChecked();
    login_config_.password = login_config_.is_saved ? password : QString();

    app_settings_->beginGroup(kLogin);
    app_settings_->setValue(kHost, host);
    app_settings_->setValue(kPort, port);
    app_settings_->setValue(kUser, user);
    app_settings_->setValue(kPassword, password);
    app_settings_->setValue(kDatabase, database);
    app_settings_->setValue(kIsSaved, login_config_.is_saved);
    app_settings_->endGroup();
}
