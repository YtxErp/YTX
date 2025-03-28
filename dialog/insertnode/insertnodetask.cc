#include "insertnodetask.h"

#include <QColorDialog>

#include "component/constvalue.h"
#include "component/signalblocker.h"
#include "ui_insertnodetask.h"

InsertNodeTask::InsertNodeTask(CInsertNodeArgFPTS& arg, int amount_decimal, CString& display_format, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::InsertNodeTask)
    , node_ { arg.node }
    , parent_path_ { arg.parent_path }
    , name_list_ { arg.name_list }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    IniDialog(arg.unit_model, amount_decimal, display_format);
    IniData(arg.node);
    IniRuleGroup();
    IniTypeGroup();
    IniConnect();
}

InsertNodeTask::~InsertNodeTask() { delete ui; }

void InsertNodeTask::IniDialog(QStandardItemModel* unit_model, int amount_decimal, CString& display_format)
{
    ui->lineEditName->setFocus();
    ui->lineEditName->setValidator(&LineEdit::kInputValidator);

    this->setWindowTitle(parent_path_);
    this->setFixedSize(350, 650);

    ui->comboUnit->setModel(unit_model);

    ui->dSpinBoxUnitCost->setDecimals(amount_decimal);
    ui->dSpinBoxUnitCost->setReadOnly(true);

    ui->dateTime->setDisplayFormat(display_format);
    ui->dateTime->setCalendarPopup(true);
}

void InsertNodeTask::IniData(Node* node)
{
    int item_index { ui->comboUnit->findData(node->unit) };
    ui->comboUnit->setCurrentIndex(item_index);

    IniRule(node->rule);
    ui->rBtnLeaf->setChecked(true);

    ui->dateTime->setDateTime(QDateTime::fromString(node_->date_time, kDateTimeFST));

    ui->pBtnOk->setEnabled(false);
}

void InsertNodeTask::IniConnect()
{
    connect(ui->lineEditName, &QLineEdit::textEdited, this, &InsertNodeTask::RNameEdited);
    connect(rule_group_, &QButtonGroup::idClicked, this, &InsertNodeTask::RRuleGroupClicked);
    connect(type_group_, &QButtonGroup::idClicked, this, &InsertNodeTask::RTypeGroupClicked);
}

void InsertNodeTask::UpdateColor(QColor color)
{
    if (color.isValid())
        ui->pBtnColor->setStyleSheet(QString(R"(
        background-color: %1;
        border-radius: 2px;
        )")
                .arg(node_->color));
}

void InsertNodeTask::IniTypeGroup()
{
    type_group_ = new QButtonGroup(this);
    type_group_->addButton(ui->rBtnLeaf, 0);
    type_group_->addButton(ui->rBtnBranch, 1);
    type_group_->addButton(ui->rBtnSupport, 2);
}

void InsertNodeTask::IniRuleGroup()
{
    rule_group_ = new QButtonGroup(this);
    rule_group_->addButton(ui->rBtnDICD, 0);
    rule_group_->addButton(ui->rBtnDDCI, 1);
}

void InsertNodeTask::IniRule(bool rule)
{
    const int kRule { static_cast<int>(rule) };

    switch (kRule) {
    case 0:
        ui->rBtnDICD->setChecked(true);
        break;
    case 1:
        ui->rBtnDDCI->setChecked(true);
        break;
    default:
        break;
    }
}

void InsertNodeTask::RNameEdited(const QString& arg1)
{
    const auto& simplified { arg1.simplified() };
    this->setWindowTitle(parent_path_ + simplified);
    ui->pBtnOk->setEnabled(!simplified.isEmpty() && !name_list_.contains(simplified));
}

void InsertNodeTask::on_lineEditName_editingFinished() { node_->name = ui->lineEditName->text(); }

void InsertNodeTask::on_lineEditCode_editingFinished() { node_->code = ui->lineEditCode->text(); }

void InsertNodeTask::on_lineEditDescription_editingFinished() { node_->description = ui->lineEditDescription->text(); }

void InsertNodeTask::on_comboUnit_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    node_->unit = ui->comboUnit->currentData().toInt();
}

void InsertNodeTask::on_plainTextEdit_textChanged() { node_->note = ui->plainTextEdit->toPlainText(); }

void InsertNodeTask::on_dSpinBoxUnitCost_editingFinished() { node_->first = ui->dSpinBoxUnitCost->value(); }

void InsertNodeTask::on_pBtnColor_clicked()
{
    QColor color(node_->color);
    if (!color.isValid())
        color = Qt::white;

    QColor selected_color { QColorDialog::getColor(color, nullptr, tr("Choose Color"), QColorDialog::ShowAlphaChannel) };
    if (selected_color.isValid()) {
        node_->color = selected_color.name(QColor::HexRgb);
        UpdateColor(selected_color);
    }
}

void InsertNodeTask::on_chkBoxFinished_checkStateChanged(const Qt::CheckState& arg1) { node_->finished = arg1 == Qt::Checked; }

void InsertNodeTask::on_dateTime_editingFinished() { node_->date_time = ui->dateTime->dateTime().toString(kDateTimeFST); }

void InsertNodeTask::RRuleGroupClicked(int id) { node_->rule = static_cast<bool>(id); }

void InsertNodeTask::RTypeGroupClicked(int id) { node_->type = id; }
