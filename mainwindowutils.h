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

#ifndef MAINWINDOWUTILS_H
#define MAINWINDOWUTILS_H

#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>
#include <QWidget>

#include "component/constvalue.h"
#include "widget/support/supportwidget.h"
#include "widget/trans/transwidget.h"
#include "worksheet.h"

template <typename T>
concept InheritQAbstractItemView = std::is_base_of_v<QAbstractItemView, T>;

template <typename T>
concept InheritQWidget = std::is_base_of_v<QWidget, T>;

template <typename T>
concept MemberFunction = std::is_member_function_pointer_v<T>;

template <typename T>
concept LeafWidgetLike = std::is_base_of_v<QWidget, T> && requires(T t) {
    { t.Model() } -> std::convertible_to<QPointer<TransModel>>;
    { t.View() } -> std::convertible_to<QPointer<QTableView>>;
};

class MainWindowUtils {
public:
    static QString ResourceFile();
    static QVariantList SaveTab(CTransWgtHash& trans_wgt_hash);
    static QSet<int> ReadSettings(std::shared_ptr<QSettings> settings, CString& section, CString& property);

    static void WriteSettings(std::shared_ptr<QSettings> settings, const QVariant& value, CString& section, CString& property);
    static void ExportYTX(CString& source, CString& destination, CStringList& table_names, CStringList& columns);
    static void ExportExcel(CString& source, CString& table, QSharedPointer<YXlsx::Worksheet> worksheet, bool where = true);
    static void Message(QMessageBox::Icon icon, CString& title, CString& text, int timeout);

    static bool IsTreeWidget(const QWidget* widget) { return widget && widget->inherits("TreeWidget"); }

    static bool CheckFileName(QString& file_path, CString& suffix);
    static bool CheckFileValid(CString& file_path, CString& suffix = kSuffixYTX);
    static bool CheckFileSQLite(CString& file_path);

    static bool AddDatabase(QSqlDatabase& db, CString& db_path, CString& connection_name);
    static QSqlDatabase GetDatabase(CString& connection_name);
    static void RemoveDatabase(CString& connection_name);

    template <InheritQAbstractItemView T> static bool HasSelection(QPointer<T> view)
    {
        return view && view->selectionModel() && view->selectionModel()->hasSelection();
    }

    template <InheritQWidget T> static void FreeWidget(QPointer<T>& widget)
    {
        if (widget) {
            delete widget;
            widget = nullptr;
        }
    }

    template <typename Container> static void SwitchDialog(Container* container, bool enable)
    {
        if (container) {
            for (auto dialog : *container) {
                if (dialog) {
                    dialog->setVisible(enable);
                }
            }
        }
    }

    template <LeafWidgetLike T> static void AppendTrans(T* widget, Section start)
    {
        if (!widget || dynamic_cast<SupportWidget*>(widget))
            return;

        auto model { widget->Model() };
        if (!model)
            return;

        constexpr int ID_ZERO = 0;
        const int empty_row = model->GetNodeRow(ID_ZERO);

        QModelIndex target_index {};

        if (empty_row == -1) {
            const int new_row = model->rowCount();
            if (!model->insertRows(new_row, 1))
                return;

            target_index = model->index(new_row, std::to_underlying(TransEnum::kDateTime));
        } else if (start != Section::kSales && start != Section::kPurchase)
            target_index = model->index(empty_row, std::to_underlying(TransEnum::kRhsNode));

        if (target_index.isValid()) {
            widget->View()->setCurrentIndex(target_index);
        }
    }

    template <InheritQWidget Widget, MemberFunction Function, typename... Args>
    static void WriteSettings(Widget* widget, Function function, std::shared_ptr<QSettings> settings, CString& section, CString& property, Args&&... args)
    {
        static_assert(std::is_same_v<decltype((std::declval<Widget>().*function)(std::forward<Args>(args)...)), QByteArray>, "Function must return QByteArray");

        if (!widget || !settings) {
            qWarning() << "SaveSettings: Invalid parameters (widget or settings is null)";
            return;
        }

        auto value { std::invoke(function, widget, std::forward<Args>(args)...) };
        settings->setValue(QString("%1/%2").arg(section, property), value);
    }

    template <InheritQWidget Widget, MemberFunction Function, typename... Args>
    static void ReadSettings(Widget* widget, Function function, std::shared_ptr<QSettings> settings, CString& section, CString& property, Args&&... args)
    {
        static_assert(std::is_same_v<decltype((std::declval<Widget>().*function)(std::declval<QByteArray>(), std::declval<Args>()...)), bool>,
            "Function must accept QByteArray and additional arguments, and return bool");

        if (!widget || !settings) {
            qWarning() << "RestoreSettings: Invalid parameters (widget or settings is null)";
            return;
        }

        auto value { settings->value(QString("%1/%2").arg(section, property)).toByteArray() };
        if (!value.isEmpty()) {
            std::invoke(function, widget, value, std::forward<Args>(args)...);
        }
    }

private:
    static QString GeneratePlaceholder(const QVariantList& values);
    static bool CopyFile(CString& source, QString& destination);
};

#endif // MAINWINDOWUTILS_H
