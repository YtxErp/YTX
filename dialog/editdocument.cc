#include "editdocument.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QStandardPaths>

#include "component/constantint.h"
#include "component/signalblocker.h"
#include "ui_editdocument.h"
#include "utils/mainwindowutils.h"

EditDocument::EditDocument(QStringList& document, CString& document_path, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::EditDocument)
    , document_ { document }
    , list_model_ { new QStringListModel(this) }
    , document_path_ { document_path }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);
    InitConnect();

    setMinimumSize(400, 300);
    CreateList(document);
}

EditDocument::~EditDocument()
{
    delete list_model_;
    delete ui;
}

void EditDocument::on_pBtnAdd_clicked()
{
    const QString filter("*.*");
    const QString base_path { QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) };
    const QString document_path { QDir(base_path).filePath(document_path_) };

    const auto local_documents { QFileDialog::getOpenFileNames(this, tr("Select Document"), document_path, filter, nullptr) };
    if (local_documents.isEmpty())
        return;

    QDir document_dir(document_path);

    for (CString& document : local_documents) {
        const QString relative_path { document_dir.relativeFilePath(document) };
        if (!document_.contains(relative_path)) {
            const int row { list_model_->rowCount() };
            list_model_->insertRow(row);
            list_model_->setData(list_model_->index(row), relative_path);
        }
    }
}

void EditDocument::on_pBtnDelete_clicked()
{
    const auto index { ui->listView->currentIndex() };
    if (!index.isValid())
        return;

    list_model_->removeRow(index.row(), QModelIndex());
}

void EditDocument::on_pBtnOk_clicked() { document_ = list_model_->stringList(); }

void EditDocument::on_listView_doubleClicked(const QModelIndex& index)
{
    const QString base_path { QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) };
    const QString document_path { QDir(base_path).filePath(document_path_) };
    const QString file_path { QDir(document_path).filePath(index.data().toString()) };

    if (QFile::exists(file_path)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(file_path));
        return;
    }

    utils::ShowMessage(QMessageBox::Critical, tr("Not Found"), tr("The document could not be found. Please check and try again."), time_const::kAutoCloseMs);
}

void EditDocument::CreateList(QStringList& document)
{
    list_model_->setStringList(document);
    ui->listView->setModel(list_model_);
}

void EditDocument::InitConnect()
{
    // Connect OK button to accept the dialog
    connect(ui->pBtnOk, &QPushButton::clicked, this, &QDialog::accept);

    // Connect Cancel button to reject the dialog
    connect(ui->pBtnCancel, &QPushButton::clicked, this, &QDialog::reject);
}
