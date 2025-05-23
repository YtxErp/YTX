#include "removenode.h"

#include <QMessageBox>

#include "component/signalblocker.h"
#include "tree/excludeintfiltermodel.h"
#include "ui_removenode.h"

RemoveNode::RemoveNode(CNodeModel* model, Section section, int node_id, int node_type, int unit, bool exteral_reference, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::RemoveNode)
    , node_id_ { node_id }
    , node_unit_ { unit }
    , node_type_ { node_type }
    , model_ { model }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    IniData(section, exteral_reference, node_type);
    IniOptionGroup();
    IniConnect();
}

RemoveNode::~RemoveNode() { delete ui; }

void RemoveNode::IniConnect() { connect(option_group_, &QButtonGroup::idClicked, this, &RemoveNode::RButtonGroup); }

void RemoveNode::IniOptionGroup()
{
    option_group_ = new QButtonGroup(this);
    option_group_->addButton(ui->rBtnRemoveRecords, 0);
    option_group_->addButton(ui->rBtnReplaceRecords, 1);
}

void RemoveNode::on_pBtnOk_clicked()
{
    QMessageBox msg(this);
    msg.setIcon(QMessageBox::Question);
    msg.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);

    const auto& path { model_->Path(node_id_) };
    QString text {};
    QString informative_text {};

    if (ui->rBtnRemoveRecords->isChecked()) {
        text = tr("Remove Node");
        informative_text = tr("Remove %1 and all its internal references. Are you sure").arg(path);
    }

    if (ui->rBtnReplaceRecords->isChecked()) {
        text = tr("Replace Node");
        informative_text = tr("Replace %1 with %2 for all internal and external references. This action is risky and cannot be undone. Are you sure?")
                               .arg(path, ui->comboBox->currentText());
    }

    msg.setText(text);
    msg.setInformativeText(informative_text);

    if (msg.exec() == QMessageBox::Ok) {
        if (ui->rBtnRemoveRecords->isChecked())
            emit SRemoveNode(node_id_, node_type_);

        if (ui->rBtnReplaceRecords->isChecked()) {
            int new_node_id { ui->comboBox->currentData().toInt() };
            emit SReplaceNode(node_id_, new_node_id, node_type_, node_unit_);
        }

        accept();
    }
}

void RemoveNode::IniData(Section section, bool exteral_reference, int node_type)
{
    ui->label->setWordWrap(true);
    ui->pBtnCancel->setDefault(true);
    this->setWindowTitle(tr("Remove %1").arg(model_->Path(node_id_)));

    if (exteral_reference) {
        ui->rBtnRemoveRecords->setEnabled(false);
        ui->label->setText(tr("The node has external references, so it can’t be removed directly. Should it be replaced instead?"));
    }

    if (section == Section::kSales || section == Section::kPurchase) {
        ui->comboBox->setEnabled(false);
        ui->rBtnReplaceRecords->setEnabled(false);

        if (exteral_reference) {
            ui->pBtnOk->setEnabled(false);
            ui->label->setText(tr("This order has been settled and cannot be deleted or modified."));
        }
        return;
    }

    ui->rBtnReplaceRecords->setChecked(true);

    auto* filter_model { new ExcludeIntFilterModel(node_id_, this) };

    if (node_type == kTypeSupport) {
        filter_model->setSourceModel(model_->SupportModel());
    } else {
        filter_model->setSourceModel(model_->LeafModel());
    }

    ui->comboBox->setModel(filter_model);

    if (section == Section::kStakeholder || node_type_ == kTypeSupport) {
        RcomboBoxCurrentIndexChanged(0);
        connect(ui->comboBox, &QComboBox::currentIndexChanged, this, &RemoveNode::RcomboBoxCurrentIndexChanged);
    }
}

void RemoveNode::RcomboBoxCurrentIndexChanged(int /*index*/)
{
    int new_node_id { ui->comboBox->currentData().toInt() };
    ui->pBtnOk->setEnabled(new_node_id != 0 && model_->Unit(new_node_id) == node_unit_);
}

void RemoveNode::RButtonGroup(int id)
{
    switch (id) {
    case 0:
        ui->pBtnOk->setEnabled(true);
        break;
    case 1:
        RcomboBoxCurrentIndexChanged(0);
        break;
    default:
        break;
    }
}
