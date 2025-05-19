#include "nodewidgetpt.h"

#include "component/constvalue.h"
#include "component/signalblocker.h"
#include "ui_nodewidgetpt.h"

NodeWidgetPT::NodeWidgetPT(NodeModel* model, CSectionConfig& section_settings, QWidget* parent)
    : NodeWidget(parent)
    , ui(new Ui::NodeWidgetPT)
    , model_ { model }
    , section_settings_ { section_settings }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    ui->treeViewFPT->setModel(model);
    ui->dspin_box_dynamic_->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    ui->dspin_box_static_->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    UpdateStatus();
}

NodeWidgetPT::~NodeWidgetPT() { delete ui; }

void NodeWidgetPT::UpdateStatus()
{
    UpdateStaticStatus();
    UpdateDynamicStatus();
}

void NodeWidgetPT::UpdateStaticStatus()
{
    ui->dspin_box_static_->setDecimals(section_settings_.common_decimal);
    ui->lable_static_->setText(section_settings_.static_label);

    const int static_node_id { section_settings_.static_node };

    if (model_->Contains(static_node_id)) {
        UpdateStaticValue(static_node_id);
    } else {
        ui->dspin_box_static_->setValue(0.0);
    }
}

void NodeWidgetPT::UpdateDynamicStatus()
{
    ui->dspin_box_dynamic_->setDecimals(section_settings_.common_decimal);
    ui->label_dynamic_->setText(section_settings_.dynamic_label);

    const int dynamic_node_id_lhs { section_settings_.dynamic_node_lhs };
    const int dynamic_node_id_rhs { section_settings_.dynamic_node_rhs };

    if (model_->Contains(dynamic_node_id_lhs) || model_->Contains(dynamic_node_id_rhs)) {
        UpdateDynamicValue(dynamic_node_id_lhs, dynamic_node_id_rhs);
    } else {
        ui->dspin_box_dynamic_->setValue(0.0);
    }
}

QPointer<QTreeView> NodeWidgetPT::View() const { return ui->treeViewFPT; }

void NodeWidgetPT::RSyncStatusValue()
{
    UpdateStaticValue(section_settings_.static_node);
    UpdateDynamicValue(section_settings_.dynamic_node_lhs, section_settings_.dynamic_node_rhs);
}

void NodeWidgetPT::UpdateDynamicValue(int lhs_node_id, int rhs_node_id)
{
    if (lhs_node_id == 0 && rhs_node_id == 0)
        return;

    const double lhs_total { model_->InitialTotal(lhs_node_id) };
    const double rhs_total { model_->InitialTotal(rhs_node_id) };

    const auto& operation { section_settings_.operation.isEmpty() ? kPlus : section_settings_.operation };
    const double total { Operate(lhs_total, rhs_total, operation) };

    ui->dspin_box_dynamic_->setValue(total);
}

void NodeWidgetPT::UpdateStaticValue(int node_id)
{
    if (node_id == 0)
        return;

    ui->dspin_box_static_->setValue(model_->InitialTotal(node_id));
}

double NodeWidgetPT::Operate(double lhs, double rhs, const QString& operation)
{
    switch (operation.at(0).toLatin1()) {
    case '+':
        return lhs + rhs;
    case '-':
        return lhs - rhs;
    default:
        return 0.0;
    }
}
