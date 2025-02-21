#include "treewidgetpt.h"

#include "component/constvalue.h"
#include "ui_treewidgetpt.h"

TreeWidgetPT::TreeWidgetPT(TreeModel* model, CSettings& settings, QWidget* parent)
    : TreeWidget(parent)
    , ui(new Ui::TreeWidgetPT)
    , model_ { model }
    , settings_ { settings }
{
    ui->setupUi(this);
    ui->treeViewFPT->setModel(model);
    ui->dspin_box_dynamic_->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    ui->dspin_box_static_->setRange(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    UpdateStatus();
}

TreeWidgetPT::~TreeWidgetPT() { delete ui; }

void TreeWidgetPT::UpdateStatus()
{
    UpdateStaticStatus();
    UpdateDynamicStatus();
}

void TreeWidgetPT::UpdateStaticStatus()
{
    ui->dspin_box_static_->setDecimals(settings_.common_decimal);
    ui->lable_static_->setText(settings_.static_label);

    const int static_node_id { settings_.static_node };

    if (model_->Contains(static_node_id)) {
        UpdateStaticValue(static_node_id);
    } else {
        ui->dspin_box_static_->setValue(0.0);
    }
}

void TreeWidgetPT::UpdateDynamicStatus()
{
    ui->dspin_box_dynamic_->setDecimals(settings_.common_decimal);
    ui->label_dynamic_->setText(settings_.dynamic_label);

    const int dynamic_node_id_lhs { settings_.dynamic_node_lhs };
    const int dynamic_node_id_rhs { settings_.dynamic_node_rhs };

    if (model_->Contains(dynamic_node_id_lhs) || model_->Contains(dynamic_node_id_rhs)) {
        UpdateDynamicValue(dynamic_node_id_lhs, dynamic_node_id_rhs);
    } else {
        ui->dspin_box_dynamic_->setValue(0.0);
    }
}

QPointer<QTreeView> TreeWidgetPT::View() const { return ui->treeViewFPT; }

void TreeWidgetPT::RUpdateStatusValue()
{
    UpdateStaticValue(settings_.static_node);
    UpdateDynamicValue(settings_.dynamic_node_lhs, settings_.dynamic_node_rhs);
}

void TreeWidgetPT::UpdateDynamicValue(int lhs_node_id, int rhs_node_id)
{
    if (lhs_node_id == 0 && rhs_node_id == 0)
        return;

    const double lhs_total { model_->InitialTotalFPT(lhs_node_id) };
    const double rhs_total { model_->InitialTotalFPT(rhs_node_id) };

    const auto& operation { settings_.operation.isEmpty() ? kPlus : settings_.operation };
    const double total { Operate(lhs_total, rhs_total, operation) };

    ui->dspin_box_dynamic_->setValue(total);
}

void TreeWidgetPT::UpdateStaticValue(int node_id)
{
    if (node_id == 0)
        return;

    ui->dspin_box_static_->setValue(model_->InitialTotalFPT(node_id));
}

double TreeWidgetPT::Operate(double lhs, double rhs, const QString& operation)
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
