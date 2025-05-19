#include "login.h"

#include <QMessageBox>
#include <QSqlDatabase>

#include "global/pgconnection.h"
#include "ui_login.h"

Login::Login(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::Login)
{
    ui->setupUi(this);
}

Login::~Login() { delete ui; }

void Login::on_pushButtonConnect_clicked()
{
    QSqlDatabase db;
    const bool ok { PGConnection::Instance().InitConnection(ui->lineEditUser->text(), ui->lineEditPassword->text(), ui->lineEditDatabase->text()) };

    if (ok) {
        this->close();
    } else {
        QMessageBox::critical(this, tr("Connection Failed"),
            tr("Unable to connect to the database. Please check if the PostgreSQL service is running, "
               "and verify the username, password, and database name."));
        return;
    }
}
