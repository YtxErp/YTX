#include "treewidgetfpt.h"

#include "component/constvalue.h"
#include "ui_treewidgetfpt.h"

TreeWidgetFPT::TreeWidgetFPT(TreeModel* model, CInfo& info, CSettings& settings, QWidget* parent)
    : TreeWidget(parent)
    , ui(new Ui::TreeWidgetFPT)
    , model_ { model }
    , info_ { info }
    , settings_ { settings }
{
    ui->setupUi(this);
    ui->treeViewFPT->setModel(model);
    ui->dspin_box_dynamic_->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    ui->dspin_box_static_->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    UpdateStatus();
}

TreeWidgetFPT::~TreeWidgetFPT() { delete ui; }

void TreeWidgetFPT::UpdateStatus()
{
    UpdateStaticStatus();
    UpdateDynamicStatus();
}

void TreeWidgetFPT::UpdateStaticStatus()
{
    ui->dspin_box_static_->setDecimals(settings_.amount_decimal);
    ui->lable_static_->setText(settings_.static_label);

    const int static_node_id { settings_.static_node };

    if (!model_->Contains(static_node_id)) {
        ResetStatus(ui->dspin_box_static_, static_unit_is_default_);
        return;
    }

    const int static_unit { model_->Unit(static_node_id) };
    static_unit_is_default_ = static_unit == settings_.default_unit;

    ui->dspin_box_static_->setPrefix(info_.unit_symbol_map.value(static_unit, kEmptyString));
    UpdateStaticValue(static_node_id);
}

void TreeWidgetFPT::UpdateDynamicStatus()
{
    const int default_unit { settings_.default_unit };

    ui->dspin_box_dynamic_->setDecimals(settings_.amount_decimal);
    ui->label_dynamic_->setText(settings_.dynamic_label);

    const int dynamic_node_id_lhs { settings_.dynamic_node_lhs };
    const int dynamic_node_id_rhs { settings_.dynamic_node_rhs };

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

QPointer<QTreeView> TreeWidgetFPT::View() const { return ui->treeViewFPT; }

void TreeWidgetFPT::RUpdateStatusValue()
{
    UpdateStaticValue(settings_.static_node);
    UpdateDynamicValue(settings_.dynamic_node_lhs, settings_.dynamic_node_rhs);
}

void TreeWidgetFPT::UpdateDynamicValue(int lhs_node_id, int rhs_node_id)
{
    if (lhs_node_id == 0 && rhs_node_id == 0)
        return;

    const double lhs_total { dynamic_unit_is_not_default_but_equal_ ? model_->InitialTotalFPT(lhs_node_id) : model_->FinalTotalFPT(lhs_node_id) };
    const double rhs_total { dynamic_unit_is_not_default_but_equal_ ? model_->InitialTotalFPT(rhs_node_id) : model_->FinalTotalFPT(rhs_node_id) };

    const auto& operation { settings_.operation.isEmpty() ? kPlus : settings_.operation };
    const double total { Operate(lhs_total, rhs_total, operation) };

    ui->dspin_box_dynamic_->setValue(total);
}

void TreeWidgetFPT::UpdateStaticValue(int node_id)
{
    if (node_id == 0)
        return;

    ui->dspin_box_static_->setValue(static_unit_is_default_ ? model_->FinalTotalFPT(node_id) : model_->InitialTotalFPT(node_id));
}

double TreeWidgetFPT::Operate(double lhs, double rhs, const QString& operation)
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

void TreeWidgetFPT::ResetStatus(QDoubleSpinBox* spin_box, bool& flags)
{
    spin_box->setPrefix(kEmptyString);
    spin_box->setValue(0.0);
    flags = false;
}
