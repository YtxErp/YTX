/*
 * Copyright (C) 2023 YtxErp
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

#ifndef SEARCH_H
#define SEARCH_H

#include <QButtonGroup>
#include <QDialog>
#include <QTableView>

#include "component/settings.h"
#include "table/transsearchmodel.h"
#include "tree/nodesearchmodel.h"

namespace Ui {
class Search;
}

class Search final : public QDialog {
    Q_OBJECT

public:
    Search(CTreeModel* tree, CTreeModel* stakeholder_tree, CTreeModel* product_tree, CSettings* settings, Sqlite* sql, CInfo& info, QWidget* parent = nullptr);
    ~Search();

signals:
    void SNodeLocation(int node_id);
    void STransLocation(int trans_id, int lhs_node_id, int rhs_node_id);

public slots:
    void RSearch();

private slots:
    void RNodeDoubleClicked(const QModelIndex& index);
    void RTransDoubleClicked(const QModelIndex& index);
    void RContentGroup(int id);

private:
    void IniDialog();
    void IniConnect();

    void TreeViewDelegate(QTableView* view, NodeSearchModel* model);
    void TableViewDelegate(QTableView* view, TransSearchModel* model);

    void IniView(QTableView* view);

    void ResizeTreeColumn(QHeaderView* header);
    void ResizeTableColumn(QHeaderView* header);

    void HideTreeColumn(QTableView* view, Section section);
    void HideTableColumn(QTableView* view, Section section);

    void IniContentGroup();

private:
    Ui::Search* ui;

    NodeSearchModel* search_tree_ {};
    TransSearchModel* search_table_ {};
    Sqlite* sql_ {};
    CTreeModel* tree_ {};
    CTreeModel* stakeholder_tree_ {};
    CTreeModel* product_tree_ {};
    QButtonGroup* content_group_ {};

    CSettings* settings_;
    CInfo& info_;
};

#endif // SEARCH_H
