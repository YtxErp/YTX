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

#include <QDialog>
#include <QTableView>
#include <QTimer>

#include "auditmodel.h"
#include "component/using.h"
#include "enum/section.h"
#include "utils/daterange.h"

namespace Ui {
class AuditDialog;
}

class AuditDialog final : public QDialog {
    Q_OBJECT

public:
    explicit AuditDialog(audit::Model* model, CUuid& widget_id, CString& title, Section seciton, QWidget* parent = nullptr);
    ~AuditDialog() override;

    QTableView* View();
    audit::Model* Model() { return model_; }

private slots:
    void on_pBtnFetch_clicked();

    void on_dateEditStart_dateChanged(const QDate& date);
    void on_dateEditEnd_dateChanged(const QDate& date);

private:
    void InitDialog();
    void InitTimer();
    static utils::DateRange DefaultRange()
    {
        const auto today { QDate::currentDate() };
        return { today.addDays(-7), today };
    }

private:
    Ui::AuditDialog* ui;
    audit::Model* model_ {};
    const Section section_ {};

    utils::DateRange range_ {};
    QTimer* cooldown_timer_ { nullptr };

    const QUuid widget_id_ {};
};
