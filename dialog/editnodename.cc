#include "editnodename.h"

#include "component/signalblocker.h"
#include "ui_editnodename.h"

EditNodeName::EditNodeName(CString& name, CString& parent_path, CStringList& children_name, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::EditNodeName)
    , parent_path_ { parent_path }
    , name_list_ { children_name }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    IniDialog(name);
    IniConnect();
    IniData(name);
}

EditNodeName::~EditNodeName() { delete ui; }

void EditNodeName::IniDialog(CString& name)
{
    ui->lineName->setFocus();
    ui->lineName->setValidator(&LineEdit::kInputValidator);

    this->setWindowTitle(parent_path_ + name);
    this->setFixedSize(400, 300);
}

void EditNodeName::IniConnect() { connect(ui->lineName, &QLineEdit::textEdited, this, &EditNodeName::RNameEdited); }

void EditNodeName::IniData(CString& name)
{
    ui->lineName->setText(name);
    ui->pBtnOk->setEnabled(false);
}

void EditNodeName::RNameEdited(const QString& arg1)
{
    const auto& simplified { arg1.simplified() };
    this->setWindowTitle(parent_path_ + simplified);
    ui->pBtnOk->setEnabled(!simplified.isEmpty() && !name_list_.contains(simplified));
}

QString EditNodeName::GetName() const { return ui->lineName->text(); }
