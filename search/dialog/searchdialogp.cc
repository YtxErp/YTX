#include "searchdialogp.h"

#include <QHeaderView>

#include "delegate/search/searchpathtabler.h"
#include "enum/entryenum.h"
#include "ui_searchdialog.h"

SearchDialogP::SearchDialogP(SectionContext* sc, search::NodeModel* search_node, search::EntryModel* search_entry, CTreeModel* inventory, QWidget* parent)
    : SearchDialog(sc, search_node, search_entry, parent)
    , inventory_ { inventory }
{
    TreeViewDelegate(ui->searchViewNode);
    TableViewDelegate(ui->searchViewEntry);
}

void SearchDialogP::TreeViewDelegate(QTableView* view)
{
    view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kInitialTotal), amount_);
    view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kUnit), unit_);
    view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kKind), kind_);
    view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kName), tree_path_);
    view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kColor), color_);
    view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kTag), tag_);
    view->setItemDelegateForColumn(std::to_underlying(NodeEnumP::kDocument), document_);
}

void SearchDialogP::TableViewDelegate(QTableView* view)
{
    view->setItemDelegateForColumn(std::to_underlying(EntryEnumP::kUnitPrice), rate_);
    view->setItemDelegateForColumn(std::to_underlying(EntryEnumP::kLhsNode), table_path_);

    auto* rhs_node_name { new SearchPathTableR(inventory_, view) };
    view->setItemDelegateForColumn(std::to_underlying(EntryEnumP::kRhsNode), rhs_node_name);
    view->setItemDelegateForColumn(std::to_underlying(EntryEnumP::kExternalSku), rhs_node_name);

    view->setItemDelegateForColumn(std::to_underlying(EntryEnumP::kStatus), check_);
    view->setItemDelegateForColumn(std::to_underlying(EntryEnumP::kIssuedTime), issued_time_);

    view->setItemDelegateForColumn(std::to_underlying(EntryEnumP::kDocument), document_);
    view->setItemDelegateForColumn(std::to_underlying(EntryEnumP::kTag), tag_);
}

void SearchDialogP::REntryDoubleClicked(const QModelIndex& index)
{
    auto* entry { static_cast<Entry*>(index.internalPointer()) };

    auto lhs_id { entry->lhs_node };
    auto entry_id { entry->id };

    if (lhs_id.isNull() || entry_id.isNull())
        return;

    emit SEntryLocation(entry_id, lhs_id, QUuid());
}
