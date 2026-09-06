#include "finance/settlement_view/settlementviewwidget.h"
#include "mainwindow.h"

void MainWindow::on_actionSettlementView_triggered()
{
    qInfo() << Q_FUNC_INFO;

    Q_ASSERT(IsOrderSection(start_));

    const QUuid widget_id { QUuid::createUuidV7() };

    auto* widget { new SettlementViewWidget(sc_p_.tree_model->LeafPath(), widget_id, sc_->section_config.amount_decimal, start_, this) };

    {
        const int tab_index { sc_->tab_widget->addTab(widget, tr("Settlement View")) };
        auto* tab_bar { sc_->tab_widget->tabBar() };

        tab_bar->setTabData(tab_index, widget_id);
    }

    {
        auto* view { widget->View() };
        InitTableView(view, -1);
    }

    RegisterWidget(widget, widget_id, WidgetRole::kSettlementView);
}

void MainWindow::RSettlementView(Section section, const QUuid& widget_id, const QJsonArray& array)
{
    auto* sc { GetSectionContex(section) };

    auto widget { sc->widget_hash.value(widget_id).widget };
    if (!widget)
        return;

    auto* d_widget { static_cast<SettlementViewWidget*>(widget.data()) };

    auto* model { d_widget->Model() };
    model->Rebuild(array);
}
