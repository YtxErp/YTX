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
    const auto user { ui->lineEditUser->text() };
    const auto password { ui->lineEditPassword->text() };
    const auto database { ui->lineEditDatabase->text() };

    const bool ok { PGConnectionPool::Instance().Initialize(user, password, database) };

    if (ok) {
        SaveLoginConfig(user, password, database);
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
}

void Login::SaveLoginConfig(const QString& user, const QString& password, const QString& database)
{
    login_config_.user = user;
    login_config_.database = database;
    login_config_.is_saved = ui->chkBoxSave->isChecked();
    login_config_.password = login_config_.is_saved ? password : QString();

    app_settings_->beginGroup(kLogin);
    app_settings_->setValue(kUser, login_config_.user);
    app_settings_->setValue(kPassword, login_config_.password);
    app_settings_->setValue(kDatabase, login_config_.database);
    app_settings_->setValue(kIsSaved, login_config_.is_saved);
    app_settings_->endGroup();
}
