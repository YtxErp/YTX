#include "nodewidgetf.h"

#include "component/constvalue.h"
#include "component/signalblocker.h"
#include "ui_nodewidgetf.h"

NodeWidgetF::NodeWidgetF(NodeModel* model, CInfo& info, CSectionSettings& section_settings, QWidget* parent)
    : NodeWidget(parent)
    , ui(new Ui::NodeWidgetF)
    , model_ { model }
    , info_ { info }
    , section_settings_ { section_settings }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    ui->treeViewFPT->setModel(model);
    ui->dspin_box_dynamic_->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    ui->dspin_box_static_->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    UpdateStatus();
}

NodeWidgetF::~NodeWidgetF() { delete ui; }

void NodeWidgetF::UpdateStatus()
{
    UpdateStaticStatus();
    UpdateDynamicStatus();
}

void NodeWidgetF::UpdateStaticStatus()
{
    ui->dspin_box_static_->setDecimals(section_settings_.amount_decimal);
    ui->lable_static_->setText(section_settings_.static_label);

    const int static_node_id { section_settings_.static_node };

    if (!model_->Contains(static_node_id)) {
        ResetStatus(ui->dspin_box_static_, static_unit_is_default_);
        return;
    }

    const int static_unit { model_->Unit(static_node_id) };
    static_unit_is_default_ = static_unit == section_settings_.default_unit;

    ui->dspin_box_static_->setPrefix(info_.unit_symbol_map.value(static_unit, kEmptyString));
    UpdateStaticValue(static_node_id);
}

void NodeWidgetF::UpdateDynamicStatus()
{
    const int default_unit { section_settings_.default_unit };

    ui->dspin_box_dynamic_->setDecimals(section_settings_.amount_decimal);
    ui->label_dynamic_->setText(section_settings_.dynamic_label);

    const int dynamic_node_id_lhs { section_settings_.dynamic_node_lhs };
    const int dynamic_node_id_rhs { section_settings_.dynamic_node_rhs };

    if (!model_->Contains(dynamic_node_id_lhs) && !model_->Contains(dynamic_node_id_rhs)) {
        ResetStatus(ui->dspin_box_dynamic_, dynamic_unit_is_not_default_but_equal_);
        return;
    }

    const int lhs_unit { model_->Unit(dynamic_node_id_lhs) };
    const int rhs_unit { model_->Unit(dynamic_node_id_rhs) };
    dynamic_unit_is_not_default_but_equal_ = (lhs_unit == rhs_unit && lhs_unit != default_unit);

    ui->dspin_box_dynamic_->setPrefix(info_.unit_symbol_map.value((dynamic_unit_is_not_default_but_equal_ ? lhs_unit : default_unit), kEmptyString));
    UpdateDynamicValue(dynamic_node_id_lhs, dynamic_node_id_rhs);
}

QPointer<QTreeView> NodeWidgetF::View() const { return ui->treeViewFPT; }

void NodeWidgetF::RSyncStatusValue()
{
    UpdateStaticValue(section_settings_.static_node);
    UpdateDynamicValue(section_settings_.dynamic_node_lhs, section_settings_.dynamic_node_rhs);
}

void NodeWidgetF::UpdateDynamicValue(int lhs_node_id, int rhs_node_id)
{
    if (lhs_node_id == 0 && rhs_node_id == 0)
        return;

    const double lhs_total { dynamic_unit_is_not_default_but_equal_ ? model_->InitialTotal(lhs_node_id) : model_->FinalTotal(lhs_node_id) };
    const double rhs_total { dynamic_unit_is_not_default_but_equal_ ? model_->InitialTotal(rhs_node_id) : model_->FinalTotal(rhs_node_id) };

    const auto& operation { section_settings_.operation.isEmpty() ? kPlus : section_settings_.operation };
    const double total { Operate(lhs_total, rhs_total, operation) };

    ui->dspin_box_dynamic_->setValue(total);
}

void NodeWidgetF::UpdateStaticValue(int node_id)
{
    if (node_id == 0)
        return;

    ui->dspin_box_static_->setValue(static_unit_is_default_ ? model_->FinalTotal(node_id) : model_->InitialTotal(node_id));
}

double NodeWidgetF::Operate(double lhs, double rhs, const QString& operation)
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

void NodeWidgetF::ResetStatus(QDoubleSpinBox* spin_box, bool& flags)
{
    spin_box->setPrefix(kEmptyString);
    spin_box->setValue(0.0);
    flags = false;
}
