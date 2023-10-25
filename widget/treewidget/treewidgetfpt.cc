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
    ui->dspin_box_dynamic_->setRange(std::numeric_limits<double>::min(), std::numeric_limits<double>::max());
    ui->dspin_box_static_->setRange(std::numeric_limits<double>::min(), std::numeric_limits<double>::max());
    SetStatus();
}

TreeWidgetFPT::~TreeWidgetFPT() { delete ui; }

void TreeWidgetFPT::SetStatus()
{
    ui->dspin_box_static_->setDecimals(settings_.amount_decimal);
    ui->lable_static_->setText(settings_.static_label);

    int static_node_id { settings_.static_node };

    if (model_->Contains(static_node_id)) {
        ui->dspin_box_static_->setPrefix(info_.unit_symbol_map.value(model_->Unit(static_node_id), QString()));
        StaticStatus(static_node_id);
    }

    ui->dspin_box_dynamic_->setDecimals(settings_.amount_decimal);
    ui->label_dynamic_->setText(settings_.dynamic_label);

    int dynamic_node_id_lhs { settings_.dynamic_node_lhs };
    int dynamic_node_id_rhs { settings_.dynamic_node_rhs };

    if (model_->Contains(dynamic_node_id_lhs) && model_->Contains(dynamic_node_id_rhs)) {
        int lhs_unit { model_->Unit(dynamic_node_id_lhs) };
        int rhs_unit { model_->Unit(dynamic_node_id_rhs) };
        equal_unit = lhs_unit == rhs_unit;

        ui->dspin_box_dynamic_->setPrefix(info_.unit_symbol_map.value((equal_unit ? lhs_unit : settings_.default_unit), QString()));
        DynamicStatus(dynamic_node_id_lhs, dynamic_node_id_rhs);
    }
}

QPointer<QTreeView> TreeWidgetFPT::View() const { return ui->treeViewFPT; }

void TreeWidgetFPT::RUpdateDSpinBox()
{
    StaticStatus(settings_.static_node);
    DynamicStatus(settings_.dynamic_node_lhs, settings_.dynamic_node_rhs);
}

void TreeWidgetFPT::DynamicStatus(int lhs_node_id, int rhs_node_id)
{
    double lhs_total { equal_unit ? model_->InitialTotalFPT(lhs_node_id) : model_->FinalTotalFPT(lhs_node_id) };
    double rhs_total { equal_unit ? model_->InitialTotalFPT(rhs_node_id) : model_->FinalTotalFPT(rhs_node_id) };

    const auto& operation { settings_.operation.isEmpty() ? kPlus : settings_.operation };
    double total { Operate(lhs_total, rhs_total, operation) };

    ui->dspin_box_dynamic_->setValue(total);
}

void TreeWidgetFPT::StaticStatus(int node_id) { ui->dspin_box_static_->setValue(model_->InitialTotalFPT(node_id)); }

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
