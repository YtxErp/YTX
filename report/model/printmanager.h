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

#include <QPrinter>
#include <QSettings>
#include <QString>

#include "table/trans.h"
#include "tree/model/nodemodel.h"

struct FieldSettings {
    int x {};
    int y {};
};

struct PrintData {
    QString party {};
    QString date_time {};
    QString employee {};
    QString unit {};
    double gross_amount {};
};

class PrintManager {
public:
    PrintManager(NodeModel* product, NodeModel* stakeholder);

    bool LoadIni(const QString& file_path);
    void SetData(const PrintData& print_data, QList<TransShadow*> trans_shadow_list);
    void Preview();
    void Print();

private:
    void RenderAllPages(QPrinter* printer);

    void ApplyConfig(QPrinter* printer);
    void ReadFieldPosition(QSettings& settings, const QString& section, const QString& prefix);

    void DrawHeader(QPainter* painter);
    void DrawTable(QPainter* painter, long long start_index, long long end_index);
    void DrawFooter(QPainter* painter, int page_num, int total_pages);

    QString GetColumnText(int col, const TransShadow* trans_shadow);

private:
    QHash<QString, QVariant> page_settings_ {};
    QHash<QString, FieldSettings> field_settings_ {};

    QList<TransShadow*> trans_shadow_list_ {};
    PrintData data_ {};

    NodeModel* product_ {};
    NodeModel* stakeholder_ {};
};

#endif // PRINTMANAGER_H
