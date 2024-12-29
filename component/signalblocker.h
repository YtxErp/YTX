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

#ifndef SIGNALBLOCKER_H
#define SIGNALBLOCKER_H

#include <QWidget>

class SignalBlocker {
public:
    explicit SignalBlocker(QWidget* parent)
    {
        if (!parent)
            return;

        auto list { parent->findChildren<QWidget*>() };
        list.emplaceBack(parent);

        for (auto* widget : list)
            if (widget && !widget->signalsBlocked()) {
                widget->blockSignals(true);
                blocked_list_.emplace_back(widget);
            }
    }

    ~SignalBlocker()
    {
        for (auto* widget : blocked_list_)
            widget->blockSignals(false);
    }

    SignalBlocker(const SignalBlocker&) = delete;
    SignalBlocker& operator=(const SignalBlocker&) = delete;
    SignalBlocker(SignalBlocker&&) = delete;
    SignalBlocker& operator=(SignalBlocker&&) = delete;

private:
    std::vector<QWidget*> blocked_list_ {};
};

#endif // SIGNALBLOCKER_H
