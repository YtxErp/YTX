#include "insertnodep.h"

#include "component/signalblocker.h"
#include "enum/nodeenum.h"
#include "ui_insertnodep.h"

InsertNodeP::InsertNodeP(CNodeInsertArg& arg, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::InsertNodeP)
    , node_ { static_cast<NodeP*>(arg.node) }
    , parent_path_ { arg.parent_path }
    , name_set_ { arg.name_set }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    IniDialog(arg.unit_model);
    IniKindGroup();
    IniData();
    IniConnect();
}

InsertNodeP::~InsertNodeP() { delete ui; }

void InsertNodeP::IniDialog(UnitModel* unit_model)
{
    ui->lineEditName->setFocus();
    ui->lineEditName->setValidator(&LineEdit::kInputValidator);

    this->setWindowTitle(parent_path_);
    this->setMinimumSize(270, 360);

    ui->comboUnit->setModel(unit_model);

    ui->rBtnBranch->setShortcut(QKeySequence(Qt::ALT | Qt::Key_B));
    ui->rBtnLeaf->setShortcut(QKeySequence(Qt::ALT | Qt::Key_L));
}

void InsertNodeP::IniConnect()
{
    connect(ui->lineEditName, &QLineEdit::textEdited, this, &InsertNodeP::RNameEdited);
    connect(kind_group_, &QButtonGroup::idClicked, this, &InsertNodeP::RKindGroupClicked);
}

void InsertNodeP::IniData()
{
    int unit_index { ui->comboUnit->findData(std::to_underlying(node_->unit)) };
    ui->comboUnit->setCurrentIndex(unit_index);

    ui->rBtnLeaf->setChecked(true);
    ui->pBtnOk->setEnabled(false);
}

void InsertNodeP::IniKindGroup()
{
    kind_group_ = new QButtonGroup(this);
    kind_group_->addButton(ui->rBtnBranch, std::to_underlying(NodeKind::kBranch));
    kind_group_->addButton(ui->rBtnLeaf, std::to_underlying(NodeKind::kLeaf));
}

void InsertNodeP::RNameEdited(const QString& arg1)
{
    const auto& simplified { arg1.simplified() };
    this->setWindowTitle(parent_path_ + simplified);
    ui->pBtnOk->setEnabled(!simplified.isEmpty() && !name_set_.contains(simplified));
}

void InsertNodeP::on_lineEditName_editingFinished() { node_->name = ui->lineEditName->text(); }

void InsertNodeP::on_lineEditCode_editingFinished() { node_->code = ui->lineEditCode->text(); }

void InsertNodeP::on_lineEditDescription_editingFinished() { node_->description = ui->lineEditDescription->text(); }

void InsertNodeP::on_comboUnit_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    node_->unit = NodeUnit(ui->comboUnit->currentData().toInt());
}

void InsertNodeP::RKindGroupClicked(int id) { node_->kind = NodeKind(id); }
