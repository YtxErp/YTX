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

#ifndef COMBOBOX_H
#define COMBOBOX_H

#include <QApplication>
#include <QComboBox>
#include <QCompleter>

class ComboBox final : public QComboBox {
    Q_OBJECT

public:
    explicit ComboBox(QWidget* parent = nullptr)
        : QComboBox { parent }
    {
        setFrame(false);
        setEditable(true);
        setInsertPolicy(QComboBox::NoInsert);

        auto* completer { new QCompleter(model(), this) };
        completer->setFilterMode(Qt::MatchContains);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        setCompleter(completer);
        setSizeAdjustPolicy(QComboBox::AdjustToContents);
    }

protected:
    QSize sizeHint() const override
    {
        QSize sz { QComboBox::sizeHint() };
        int scrollbar_width { QApplication::style()->pixelMetric(QStyle::PM_ScrollBarExtent) };

        sz.setWidth(sz.width() + scrollbar_width);
        return sz;
    }
};

#endif // COMBOBOX_H
