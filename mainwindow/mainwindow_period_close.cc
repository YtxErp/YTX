#include "finance/period_close/periodclosedialog.h"
#include "finance/period_close/periodclosemodel.h"
#include "mainwindow.h"
#include "utils/mainwindowutils.h"

void MainWindow::on_actionPeriodClose_triggered()
{
    qInfo() << Q_FUNC_INFO;

    static QPointer<PeriodCloseDialog> dialog {};

    if (!dialog) {
        auto* table_model { new period_close::Model(sc_f_.info, this) };

        dialog = new PeriodCloseDialog(Section::kFinance, sc_f_.tree_model, table_model);
        utils::ManageDialog(sc_f_.widget_hash, dialog);

        auto* view { dialog->View() };
        InitTableView(view, std::to_underlying(FullEntryEnumF::kDescription));
        DelegatePeriodClose(view);
    }

    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}