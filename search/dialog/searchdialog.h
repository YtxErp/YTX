/*
 * Copyright (C) 2023 YTX
 *
 * This file is part of YTX.
 *
 * YTX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YTX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with YTX. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <QButtonGroup>
#include <QDialog>
#include <QTableView>

#include "component/config.h"
#include "component/sectioncontex.h"
#include "delegate/readonly/boolstringr.h"
#include "delegate/readonly/colorr.h"
#include "delegate/readonly/documentr.h"
#include "delegate/readonly/doublenonezeror.h"
#include "delegate/readonly/intstringr.h"
#include "delegate/readonly/issuedtimer.h"
#include "delegate/readonly/statusr.h"
#include "delegate/search/searchpathtabler.h"
#include "delegate/search/searchpathtreer.h"
#include "delegate/tagdelegate.h"
#include "search/entry/searchentrymodel.h"
#include "search/node/searchnodemodel.h"

namespace Ui {
class SearchDialog;
}

class SearchDialog : public QDialog {
    Q_OBJECT

protected:
    SearchDialog(SectionContext* sc, search::NodeModel* search_node, search::EntryModel* search_entry, QWidget* parent = nullptr);
    ~SearchDialog() override;

signals:
    void SNodeLocation(Section section, const QUuid& node_id);
    void SEntryLocation(const QUuid& entry_id, const QUuid& lhs_node_id, const QUuid& rhs_node_id);

private slots:
    void RContentGroup(int id);
    void RSearchNode();
    void RSearchEntry();

    virtual void RNodeDoubleClicked(const QModelIndex& index);
    virtual void REntryDoubleClicked(const QModelIndex& index);

protected:
    void IniDialog();
    void IniConnect();

    virtual void TreeViewDelegate(QTableView* view) = 0;
    virtual void TableViewDelegate(QTableView* view);

    void IniView(QTableView* view);
    void ResizeTreeColumn(QHeaderView* header);
    void ResizeTableColumn(QHeaderView* header);
    void IniContentGroup();
    void InitDelegate();
    void HideTableColumn(QTableView* view);

protected:
    Ui::SearchDialog* ui;

    search::NodeModel* search_node_ {};
    search::EntryModel* search_entry_ {};
    CTreeModel* tree_model_ {};
    QButtonGroup* content_group_ {};

    DoubleNoneZeroR* amount_ {};
    DoubleNoneZeroR* rate_ {};
    DoubleNoneZeroR* quantity_ {};

    IssuedTimeR* issued_time_ {};
    DocumentR* document_ {};

    IntStringR* unit_ {};
    BoolStringR* direction_rule_ {};
    IntStringR* kind_ {};
    SearchPathTreeR* tree_path_ {};

    StatusR* check_ {};
    ColorR* color_ {};
    SearchPathTableR* table_path_ {};
    TagDelegate* tag_ {};

    CSectionConfig& config_;
    CSectionInfo& info_;
    const QHash<QUuid, tag::Icon>& tag_icon_hash_ {};
};
