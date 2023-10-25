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

#ifndef USING_H
#define USING_H

#include <QDialog>
#include <QHash>
#include <QMap>
#include <QStringList>
#include <QTime>

using CStringMap = const QMap<int, QString>;
using StringMap = QMap<int, QString>;

using CStringHash = const QHash<int, QString>;
using StringHash = QHash<int, QString>;

using CString = const QString;
using CVariant = const QVariant;
using CStringList = const QStringList;

using CIntSet = const QSet<int>;
using PDialog = QPointer<QDialog>;

inline const QString kEmptyString {};
inline const QString kDD { "dd" };
inline const QTime kStartTime { 0, 0 }; // 00:00
inline const QTime kEndTime { 23, 59 }; // 23:59

#endif // USING_H
