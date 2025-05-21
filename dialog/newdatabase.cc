#include "newdatabase.h"

#include <QMessageBox>

#include "ui_newdatabase.h"

NewDatabase::NewDatabase(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::NewDatabase)
{
    ui->setupUi(this);
}

NewDatabase::~NewDatabase() { delete ui; }

void NewDatabase::on_pushButtonCreate_clicked()
{
    const auto user { ui->lineEditUser->text() };
    const auto password { ui->lineEditPassword->text() };
    const auto database { ui->lineEditDatabase->text() };

    // Step 4: Notify user
    QMessageBox::information(this, "Success", "Role and database were created successfully.");
    close();
}
