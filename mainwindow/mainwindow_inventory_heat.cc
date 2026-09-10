#include "dashboard/inventory_heat/inventoryheatdialog.h"
#include "dashboard/inventory_heat/inventoryheatenum.h"
#include "mainwindow.h"
#include "utils/mainwindowutils.h"

void MainWindow::on_actionHeatInventory_triggered()
{
    qInfo() << Q_FUNC_INFO;

    auto* model { new inventory_heat::Model(header_info_.inventory_heat, this) };
    const QUuid widget_id { QUuid::createUuidV7() };

    auto* dialog { new InventoryHeatDialog(model, widget_id) };

    {
        auto* view { dialog->View() };
        InitTableView(view, std::to_underlying(inventory_heat::RowField::kPlaceholder));
        DelegateInventoryHeat(view);
    }

    utils::ManageDialog(sc_i_.widget_hash, dialog, widget_id);

    dialog->show();
}

void MainWindow::RInventoryHeatAck(const QUuid& widget_id, const QJsonArray& array)
{
    auto widget { sc_i_.widget_hash.value(widget_id).widget };
    if (!widget)
        return;

    auto* d_widget { static_cast<InventoryHeatDialog*>(widget.data()) };
    if (!d_widget)
        return;

    auto* model { d_widget->Model() };
    model->Rebuild(array);
}