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
#include "database/mainwindowsqlite.h"
#include "widget/tablewidget/tablewidget.h"

template <typename T>
concept InheritQAbstractItemView = std::is_base_of_v<QAbstractItemView, T>;

template <typename T>
concept InheritQWidget = std::is_base_of_v<QWidget, T>;

template <typename T>
concept MemberFunction = std::is_member_function_pointer_v<T>;

class MainWindowUtils {
public:
    static QString ResourceFile();
    static QVariantList SaveTab(CTableHash& table_hash);
    static QSet<int> ReadSettings(std::shared_ptr<QSettings> settings, CString& section, CString& property);

    static void WriteSettings(std::shared_ptr<QSettings> settings, const QVariant& value, CString& section, CString& property);
    static void ExportColumns(CString& source, CString& destination, CStringList& table_names, CStringList& columns);
    static void Message(QMessageBox::Icon icon, CString& title, CString& text, int timeout);

    static bool CopyFile(CString& source, CString& destination);
    static bool NewFile(MainwindowSqlite& sql, QString& file_path);
    static bool IsValidFile(const QFileInfo& file_info, CString& suffix = ytx);
    static bool IsTreeWidget(const QWidget* widget) { return widget && widget->inherits("TreeWidget"); }

    template <InheritQAbstractItemView T> static bool HasSelection(QPointer<T> view)
    {
        return view && view->selectionModel() && view->selectionModel()->hasSelection();
    }

    template <InheritQWidget T> static void FreeWidget(T*& widget)
    {
        if (widget) {
            if (auto model = widget->Model())
                delete model;

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
    static bool IsSQLiteFile(CString& file_path);
    static QString GeneratePlaceholder(const QVariantList& values);
};

#endif // MAINWINDOWUTILS_H
