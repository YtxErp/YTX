#include "dashboard/income_statement/incomestatementdialog.h"
#include "dashboard/income_statement/incomestatementenum.h"
#include "dashboard/income_statement/incomestatementmodel.h"
#include "mainwindow.h"
#include "utils/mainwindowutils.h"

void MainWindow::on_actionIncomeStatement_triggered()
{
    qInfo() << Q_FUNC_INFO;

    auto* model { new income_statement::Model(header_info_.income_statement, this) };
    const QUuid widget_id { QUuid::createUuidV7() };

    auto* dialog { new IncomeStatementDialog(sc_f_.tree_model, model, widget_id) };

    {
        auto* view { dialog->View() };
        InitTreeView(view, -1, std::to_underlying(income_statement::RowField::kDescription));
        DelegateIncomeStatement(view);
    }

    utils::ManageDialog(sc_f_.widget_hash, dialog, widget_id);

    dialog->show();
}

void MainWindow::RIncomeStatementAck(const QUuid& widget_id, const QJsonObject& obj)
{
    auto widget { sc_f_.widget_hash.value(widget_id).widget };
    if (!widget)
        return;

    auto* d_widget { static_cast<IncomeStatementDialog*>(widget.data()) };
    if (!d_widget)
        return;

    const QJsonArray node_array { obj.value(kNodeArray).toArray() };
    const QJsonArray path_array { obj.value(kPathArray).toArray() };

    const auto profits { obj.value(income_statement::kProfits).toObject() };
    const double net_profit { profits.value(income_statement::kNetProfit).toString().toDouble() };
    const double yoy_net_profit { profits.value(income_statement::kYoyNetProfit).toString().toDouble() };
    const double mom_net_profit { profits.value(income_statement::kMomNetProfit).toString().toDouble() };

    const auto date_range { obj.value(income_statement::kDateRange).toObject() };
    const auto yoy_start { QDateTime::fromString(date_range.value(income_statement::kYoyStart).toString(), Qt::ISODate).toLocalTime() };
    const auto yoy_end { QDateTime::fromString(date_range.value(income_statement::kYoyEnd).toString(), Qt::ISODate).toLocalTime().addDays(-1) };
    const auto mom_start { QDateTime::fromString(date_range.value(income_statement::kMomStart).toString(), Qt::ISODate).toLocalTime() };
    const auto mom_end { QDateTime::fromString(date_range.value(income_statement::kMomEnd).toString(), Qt::ISODate).toLocalTime().addDays(-1) };

    const QString yoy_tooltip { QString("%1 ~ %2").arg(yoy_start.toString(datetime_format::kDottedDate), yoy_end.toString(datetime_format::kDottedDate)) };
    const QString mom_tooltip { QString("%1 ~ %2").arg(mom_start.toString(datetime_format::kDottedDate), mom_end.toString(datetime_format::kDottedDate)) };

    auto* model { d_widget->Model() };
    model->UpdateHeaderTooltip(yoy_tooltip, mom_tooltip);
    model->Rebuild(node_array, path_array, net_profit, yoy_net_profit, mom_net_profit);
}