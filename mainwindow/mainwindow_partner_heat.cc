#include "dashboard/partner_heat/partnerheatdialog.h"
#include "dashboard/partner_heat/partnerheatenum.h"
#include "dashboard/partner_heat/partnerheatmodel.h"
#include "mainwindow.h"
#include "utils/mainwindowutils.h"

void MainWindow::on_actionHeatPartner_triggered()
{
    qInfo() << Q_FUNC_INFO;

    auto* model { new partner_heat::Model(header_info_.partner_heat, this) };
    const QUuid widget_id { QUuid::createUuidV7() };

    auto* dialog { new PartnerHeatDialog(model, widget_id) };

    {
        auto* view { dialog->View() };
        InitTableView(view, std::to_underlying(partner_heat::RowField::kPlaceholder));
        DelegatePartnerHeat(view);
    }

    utils::ManageDialog(sc_p_.widget_hash, dialog, widget_id);

    dialog->show();
}

void MainWindow::RPartnerHeatAck(const QUuid& widget_id, const QJsonArray& array)
{
    auto widget { sc_p_.widget_hash.value(widget_id).widget };
    if (!widget)
        return;

    auto* d_widget { static_cast<PartnerHeatDialog*>(widget.data()) };
    if (!d_widget)
        return;

    auto* model { d_widget->Model() };
    model->Rebuild(array);
}