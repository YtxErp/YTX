#include "treewidgetf.h"

#include "component/constant.h"
#include "component/signalblocker.h"
#include "ui_treewidgetf.h"

TreeWidgetF::TreeWidgetF(TreeModel* model, CSectionInfo& info, CSharedConfig& shared, CSectionConfig& section, QWidget* parent)
    : TreeWidget(parent)
    , ui(new Ui::TreeWidgetF)
    , model_ { model }
    , info_ { info }
    , section_ { section }
    , shared_ { shared }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    ui->treeView->setModel(model);
    model->setParent(ui->treeView);

    InitDoubleSpinBox(ui->dspin_box_dynamic_);
    InitDoubleSpinBox(ui->dspin_box_static_);
}

TreeWidgetF::~TreeWidgetF() { delete ui; }

void TreeWidgetF::RInitStatus()
{
    InitStaticStatus();
    InitDynamicStatus();
}

void TreeWidgetF::InitStaticStatus()
{
    ui->dspin_box_static_->setDecimals(section_.amount_decimal);
    ui->label_static_->setText(section_.static_label);

    const auto static_node { section_.static_node };
    const int static_node_unit { std::to_underlying(model_->Unit(static_node)) };
    ui->dspin_box_static_->setPrefix(info_.unit_symbol_map.value(static_node_unit));

    SyncStaticValue(static_node);
}

void TreeWidgetF::InitDynamicStatus()
{
    ui->dspin_box_dynamic_->setDecimals(section_.amount_decimal);
    ui->dspin_box_dynamic_->setPrefix(info_.unit_symbol_map.value(shared_.default_unit));
    ui->label_dynamic_->setText(section_.dynamic_label);

    SyncDynamicValue(section_.dynamic_node_lhs, section_.dynamic_node_rhs);
}

QTreeView* TreeWidgetF::View() const { return ui->treeView; }

void TreeWidgetF::Reset()
{
    InitDoubleSpinBox(ui->dspin_box_dynamic_);
    InitDoubleSpinBox(ui->dspin_box_static_);

    ui->label_static_->setText(QString());
    ui->label_dynamic_->setText(QString());
}

void TreeWidgetF::RSyncValue()
{
    SyncStaticValue(section_.static_node);
    SyncDynamicValue(section_.dynamic_node_lhs, section_.dynamic_node_rhs);
}

void TreeWidgetF::SyncDynamicValue(const QUuid& lhs_node_id, const QUuid& rhs_node_id)
{
    if (lhs_node_id.isNull() || rhs_node_id.isNull() || !model_->Contains(lhs_node_id) || !model_->Contains(rhs_node_id)) {
        ui->dspin_box_dynamic_->setValue(std::numeric_limits<double>::lowest());
        return;
    }

    const double lhs_total { model_->FinalTotal(lhs_node_id) };
    const double rhs_total { model_->FinalTotal(rhs_node_id) };

    const auto& operation { section_.operation.isEmpty() ? kPlus : section_.operation };
    const double total { Operate(lhs_total, rhs_total, operation) };

    ui->dspin_box_dynamic_->setValue(total);
}

void TreeWidgetF::SyncStaticValue(const QUuid& node_id)
{
    if (node_id.isNull() || !model_->Contains(node_id)) {
        ui->dspin_box_static_->setValue(std::numeric_limits<double>::lowest());
        return;
    }

    ui->dspin_box_static_->setValue(model_->InitialTotal(node_id));
}
