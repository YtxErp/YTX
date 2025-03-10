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

#ifndef REPORTWIDGET_H
#define REPORTWIDGET_H

#include <QPointer>
#include <QTableView>
#include <QWidget>

class ReportWidget : public QWidget {
    Q_OBJECT

public:
    virtual ~ReportWidget() = default;

    virtual QPointer<QAbstractItemModel> Model() const = 0;
    virtual QPointer<QTableView> View() const = 0;
    virtual bool IsReportWidget() const = 0;

protected:
    explicit ReportWidget(QWidget* parent = nullptr)
        : QWidget { parent }
    {
    }
};

using RptWgtHash = QHash<int, QPointer<ReportWidget>>;

#endif // REPORTWIDGET_H
