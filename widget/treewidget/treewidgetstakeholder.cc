#include "treewidgetstakeholder.h"

#include "ui_treewidgetstakeholder.h"

TreeWidgetStakeholder::TreeWidgetStakeholder(TreeModel* model, CInfo& info, CSettings& settings, QWidget* parent)
    : TreeWidget(parent)
    , ui(new Ui::TreeWidgetStakeholder)
    , model_ { model }
    , info_ { info }
    , settings_ { settings }
{
    ui->setupUi(this);
    ui->treeViewStakeholder->setModel(model);
}

TreeWidgetStakeholder::~TreeWidgetStakeholder() { delete ui; }

QPointer<QTreeView> TreeWidgetStakeholder::View() const { return ui->treeViewStakeholder; }
