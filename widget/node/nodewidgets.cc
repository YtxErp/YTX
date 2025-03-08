#include "nodewidgets.h"

#include "ui_nodewidgets.h"

NodeWidgetS::NodeWidgetS(NodeModel* model, QWidget* parent)
    : NodeWidget(parent)
    , ui(new Ui::NodeWidgetS)
    , model_ { model }
{
    ui->setupUi(this);
    ui->treeViewStakeholder->setModel(model);
}

NodeWidgetS::~NodeWidgetS() { delete ui; }

QPointer<QTreeView> NodeWidgetS::View() const { return ui->treeViewStakeholder; }
