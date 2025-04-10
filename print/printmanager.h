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

#ifndef PRINTMANAGER_H
#define PRINTMANAGER_H

#include <QMap>
#include <QPrinter>
#include <QString>
#include <QTextDocument>

#include "printdata.h"
#include "table/trans.h"
#include "tree/model/nodemodel.h"

class PrintManager {
public:
    PrintManager(NodeModel* product, NodeModel* stakeholder);

    bool LoadHtml(const QString& file_path);
    void SetData(const PrintData& print_data, QList<TransShadow*> trans_shadow);
    void Preview();
    void Print();

private:
    QString BuildProductRow(TransShadow* item);
    void RenderAllPages(QPrinter* printer);

    QString ReadHtmlConfig(const QString& html, const QString& id) const;
    void ReadConfig();
    void ApplyConfig(QPrinter* printer) const;

private:
    QString html_ {};
    int rows_per_page_ { 7 };
    QString paper_size_ { "a5" };
    QString paper_orientation_ { "landscape" };

    QList<TransShadow*> trans_shadow_ {};

    NodeModel* product_ {};
    NodeModel* stakeholder_ {};
};

#endif // PRINTMANAGER_H
