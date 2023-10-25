#include "insertnodeproduct.h"

#include <QColorDialog>

#include "component/signalblocker.h"
#include "ui_insertnodeproduct.h"

InsertNodeProduct::InsertNodeProduct(CInsertNodeParamsFPTS& params, int amount_decimal, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::InsertNodeProduct)
    , node_ { params.node }
    , parent_path_ { params.parent_path }
    , name_list_ { params.name_list }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    IniDialog(params.unit_model, amount_decimal);
    IniRuleGroup();
    IniTypeGroup();
    IniData(params.node);
    IniConnect();
}

InsertNodeProduct::~InsertNodeProduct() { delete ui; }

void InsertNodeProduct::IniDialog(QStandardItemModel* unit_model, int amount_decimal)
{
    ui->lineEditName->setFocus();
    ui->lineEditName->setValidator(&LineEdit::kInputValidator);

    this->setWindowTitle(parent_path_);
    this->setFixedSize(350, 650);

    ui->comboUnit->setModel(unit_model);

    ui->dSpinBoxUnitPrice->setRange(0.0, std::numeric_limits<double>::max());
    ui->dSpinBoxCommission->setRange(0.0, std::numeric_limits<double>::max());
    ui->dSpinBoxUnitPrice->setDecimals(amount_decimal);
    ui->dSpinBoxCommission->setDecimals(amount_decimal);
}

void InsertNodeProduct::IniConnect()
{
    connect(ui->lineEditName, &QLineEdit::textEdited, this, &InsertNodeProduct::RNameEdited);
    connect(rule_group_, &QButtonGroup::idClicked, this, &InsertNodeProduct::RRuleGroupClicked);
    connect(type_group_, &QButtonGroup::idClicked, this, &InsertNodeProduct::RTypeGroupClicked);
}

void InsertNodeProduct::IniData(Node* node)
{
    int unit_index { ui->comboUnit->findData(node->unit) };
    ui->comboUnit->setCurrentIndex(unit_index);

    IniRule(node->rule);
    ui->rBtnLeaf->setChecked(true);

    ui->pBtnOk->setEnabled(false);
}

void InsertNodeProduct::UpdateColor(QColor color)
{
    if (color.isValid())
        ui->pBtnColor->setStyleSheet(QString(R"(
        background-color: %1;
        border-radius: 2px;
        )")
                .arg(node_->color));
}

void InsertNodeProduct::IniTypeGroup()
{
    type_group_ = new QButtonGroup(this);
    type_group_->addButton(ui->rBtnLeaf, 0);
    type_group_->addButton(ui->rBtnBranch, 1);
    type_group_->addButton(ui->rBtnSupport, 2);
}

void InsertNodeProduct::IniRuleGroup()
{
    rule_group_ = new QButtonGroup(this);
    rule_group_->addButton(ui->rBtnDICD, 0);
    rule_group_->addButton(ui->rBtnDDCI, 1);
}

void InsertNodeProduct::IniRule(bool rule)
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

void InsertNodeProduct::RNameEdited(const QString& arg1)
{
    const auto& simplified { arg1.simplified() };
    this->setWindowTitle(parent_path_ + simplified);
    ui->pBtnOk->setEnabled(!simplified.isEmpty() && !name_list_.contains(simplified));
}

void InsertNodeProduct::on_lineEditName_editingFinished() { node_->name = ui->lineEditName->text(); }

void InsertNodeProduct::on_lineEditCode_editingFinished() { node_->code = ui->lineEditCode->text(); }

void InsertNodeProduct::on_lineEditDescription_editingFinished() { node_->description = ui->lineEditDescription->text(); }

void InsertNodeProduct::on_comboUnit_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    node_->unit = ui->comboUnit->currentData().toInt();
}

void InsertNodeProduct::on_plainTextEdit_textChanged() { node_->note = ui->plainTextEdit->toPlainText(); }

void InsertNodeProduct::on_dSpinBoxUnitPrice_editingFinished() { node_->first = ui->dSpinBoxUnitPrice->value(); }

void InsertNodeProduct::on_dSpinBoxCommission_editingFinished() { node_->second = ui->dSpinBoxCommission->value(); }

void InsertNodeProduct::on_pBtnColor_clicked()
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

void InsertNodeProduct::RRuleGroupClicked(int id) { node_->rule = static_cast<bool>(id); }

void InsertNodeProduct::RTypeGroupClicked(int id) { node_->type = id; }
