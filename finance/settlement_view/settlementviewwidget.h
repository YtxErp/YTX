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

#include <QTableView>
#include <QTimer>
#include <QWidget>

#include "component/using.h"
#include "delegate/readonly/doublenonezeror.h"
#include "enum/section.h"
#include "settlement_view_model.h"
#include "utils/daterange.h"

namespace Ui {
class SettlementViewWidget;
}

class SettlementViewWidget final : public QWidget {
    Q_OBJECT

public:
    explicit SettlementViewWidget(
        const QHash<QUuid, QString>& partner_leaf_path, CUuid& widget_id, const int& amount_decimal, Section section, QWidget* parent = nullptr);
    ~SettlementViewWidget() override;

    QTableView* View() const;
    settlement_view::Model* Model() const { return model_; }

private slots:
    void on_pBtnFetch_clicked();
    void on_start_dateChanged(const QDate& date);
    void on_end_dateChanged(const QDate& date);

private:
    static utils::DateRange DefaultRange()
    {
        const auto today { QDate::currentDate() };

        const auto start_month { today.addMonths(-2) };

        const QDate start { start_month.year(), start_month.month(), 1 };
        const QDate end { today.year(), today.month(), today.daysInMonth() };

        return { start, end };
    }

    void IniWidget();
    void InitTimer();
    void SetupColumns();

private:
    Ui::SettlementViewWidget* ui;

    utils::DateRange range_ {};
    settlement_view::Model* model_ {};
    CUuid widget_id_ {};
    const Section section_ {};

    QTimer* cooldown_timer_ { nullptr };
    DoubleNoneZeroR* amount_dlg_ {};
};
