#include "mainwindow.h"
#include "search/dialog/searchdialog.h"
#include "search/dialog/searchdialogf.h"
#include "search/dialog/searchdialogi.h"
#include "search/dialog/searchdialogo.h"
#include "search/dialog/searchdialogp.h"
#include "search/dialog/searchdialogt.h"
#include "search/entry/searchentrymodelf.h"
#include "search/entry/searchentrymodeli.h"
#include "search/entry/searchentrymodelo.h"
#include "search/entry/searchentrymodelp.h"
#include "search/entry/searchentrymodelt.h"
#include "search/node/searchnodemodelf.h"
#include "search/node/searchnodemodeli.h"
#include "search/node/searchnodemodelo.h"
#include "search/node/searchnodemodelp.h"
#include "search/node/searchnodemodelt.h"
#include "utils/mainwindowutils.h"
#include "websocket/websocket.h"

void MainWindow::on_actionSearch_triggered()
{
    qInfo() << Q_FUNC_INFO;

    if (!section_settings_)
        return;

    search::NodeModel* node {};
    search::EntryModel* entry {};
    SearchDialog* dialog {};

    switch (start_) {
    case Section::kFinance:
        node = new search::NodeModelF(sc_->info, sc_->tree_model, sc_->tag_hash, this);
        entry = new search::EntryModelF(sc_->info, sc_->tag_hash, this);
        dialog = new SearchDialogF(sc_, node, entry, nullptr);
        break;
    case Section::kInventory:
        node = new search::NodeModelI(sc_->info, sc_->tree_model, sc_->tag_hash, this);
        entry = new search::EntryModelI(sc_->info, sc_->tag_hash, this);
        dialog = new SearchDialogI(sc_, node, entry, nullptr);
        break;
    case Section::kTask:
        node = new search::NodeModelT(sc_->info, sc_->tree_model, sc_->tag_hash, this);
        entry = new search::EntryModelT(sc_->info, sc_->tag_hash, this);
        dialog = new SearchDialogT(sc_, node, entry, nullptr);
        break;
    case Section::kPartner:
        node = new search::NodeModelP(sc_->info, sc_->tree_model, sc_->tag_hash, this);
        entry = new search::EntryModelP(sc_->entry_hub, sc_->info, sc_->tag_hash, this);
        dialog = new SearchDialogP(sc_, node, entry, sc_i_.tree_model, nullptr);
        break;
    case Section::kSale:
    case Section::kPurchase:
        node = new search::NodeModelO(sc_->info, sc_->tree_model, sc_->tag_hash, this);
        entry = new search::EntryModelO(sc_->info, sc_->tag_hash, this);
        dialog = new SearchDialogO(sc_, node, entry, sc_i_.tree_model, sc_p_.tree_model, nullptr);
        connect(WebSocket::Instance(), &WebSocket::SOrderSearch, node, &search::NodeModel::ROrderSearch);
        break;
    default:
        break;
    }

    utils::ManageDialog(sc_->widget_hash, dialog);

    connect(dialog, &SearchDialog::SNodeLocation, this, &MainWindow::RLocateNode);
    connect(dialog, &SearchDialog::SEntryLocation, this, &MainWindow::REntryLocation);
    connect(sc_->entry_hub, &EntryHub::SSearchEntry, entry, &search::EntryModel::RSearchEntry);

    dialog->show();
}
