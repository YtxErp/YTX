#include "dashboard/balance_sheet/balancesheetdialog.h"
#include "dashboard/balance_sheet/balancesheetenum.h"
#include "dashboard/balance_sheet/balancesheetmodel.h"
#include "mainwindow.h"
#include "utils/mainwindowutils.h"

void MainWindow::on_actionBalanceSheet_triggered()
{
    qInfo() << Q_FUNC_INFO;

    auto* model { new balance_sheet::Model(header_info_.balance_sheet, this) };
    const QUuid widget_id { QUuid::createUuidV7() };

    auto* dialog { new BalanceSheetDialog(sc_f_.tree_model, model, widget_id) };

    {
        auto* view { dialog->View() };
        InitTreeView(view, -1, std::to_underlying(balance_sheet::RowField::kDescription));
        DelegateBalanceSheet(view);
    }

    utils::ManageDialog(sc_f_.widget_hash, dialog, widget_id);
    dialog->show();
}

void MainWindow::RBalanceSheetAck(const QUuid& widget_id, const QJsonArray& node_array, const QJsonArray& path_array)
{
    auto widget { sc_f_.widget_hash.value(widget_id).widget };
    if (!widget)
        return;

    auto* d_widget { static_cast<BalanceSheetDialog*>(widget.data()) };
    if (!d_widget)
        return;

    auto* model { d_widget->Model() };
    model->Rebuild(node_array, path_array);
}
