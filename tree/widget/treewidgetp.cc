#include "treewidgetp.h"

#include "ui_treewidgetp.h"

TreeWidgetP::TreeWidgetP(TreeModel* model, CSectionConfig& config, QWidget* parent)
    : TreeWidget(parent)
    , ui(new Ui::TreeWidgetP)
    , model_ { model }
    , config_ { config }
{
    ui->setupUi(this);
    ui->treeView->setModel(model);
    model->setParent(ui->treeView);

    InitDoubleSpinBox(ui->dspin_box_dynamic_);
    InitDoubleSpinBox(ui->dspin_box_static_);
}

void TreeWidgetP::RSyncValue()
{
    SyncStaticValue(config_.static_node);
    SyncDynamicValue(config_.dynamic_node_lhs, config_.dynamic_node_rhs);
}

void TreeWidgetP::RInitStatus()
{
    InitStaticStatus();
    InitDynamicStatus();
}

TreeWidgetP::~TreeWidgetP() { delete ui; }

QTreeView* TreeWidgetP::View() const { return ui->treeView; }

void TreeWidgetP::Reset()
{
    InitDoubleSpinBox(ui->dspin_box_dynamic_);
    InitDoubleSpinBox(ui->dspin_box_static_);

    ui->label_static_->setText(QString());
    ui->label_dynamic_->setText(QString());
}

void TreeWidgetP::InitStaticStatus()
{
    ui->dspin_box_static_->setDecimals(config_.amount_decimal);
    ui->label_static_->setText(config_.static_label);

    SyncStaticValue(config_.static_node);
}

void TreeWidgetP::InitDynamicStatus()
{
    ui->dspin_box_dynamic_->setDecimals(config_.amount_decimal);
    ui->label_dynamic_->setText(config_.dynamic_label);

    SyncDynamicValue(config_.dynamic_node_lhs, config_.dynamic_node_rhs);
}

void TreeWidgetP::SyncDynamicValue(const QUuid& lhs_node_id, const QUuid& rhs_node_id)
{
    if (lhs_node_id.isNull() || rhs_node_id.isNull() || !model_->Contains(lhs_node_id) || !model_->Contains(rhs_node_id)) {
        ui->dspin_box_dynamic_->setValue(std::numeric_limits<double>::lowest());
        return;
    }

    const double lhs_total { model_->InitialTotal(lhs_node_id) };
    const double rhs_total { model_->InitialTotal(rhs_node_id) };

    const auto& operation { config_.operation.isEmpty() ? kPlus : config_.operation };
    const double total { Operate(lhs_total, rhs_total, operation) };

    ui->dspin_box_dynamic_->setValue(total);
}

void TreeWidgetP::SyncStaticValue(const QUuid& node_id)
{
    if (node_id.isNull() || !model_->Contains(node_id)) {
        ui->dspin_box_static_->setValue(std::numeric_limits<double>::lowest());
        return;
    }

    ui->dspin_box_static_->setValue(model_->InitialTotal(node_id));
}
