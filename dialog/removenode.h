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

#ifndef REMOVENODE_H
#define REMOVENODE_H

#include <QDialog>

#include "tree/model/nodemodel.h"

namespace Ui {
class RemoveNode;
}

class RemoveNode final : public QDialog {
    Q_OBJECT

public:
    RemoveNode(CNodeModel* model, Section section, int node_id, int node_type, int unit, bool exteral_reference, QWidget* parent = nullptr);
    ~RemoveNode();

signals:
    // send to sqlite
    void SRemoveNode(int node_id, int node_type);
    void SReplaceNode(int old_node_id, int new_node_id, int node_type);

private slots:
    void on_pBtnOk_clicked();

private:
    void IniData(Section section, bool exteral_reference, int node_type);
    void DisableRemove();

private:
    Ui::RemoveNode* ui;

    int node_id_ {};
    int unit_ {};
    int node_type_ {};

    CNodeModel* model_ {};
};

#endif // REMOVENODE_H
