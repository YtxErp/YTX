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

#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <QObject>
#include <QTimer>
#include <QWebSocket>

#include "component/info.h"
#include "component/using.h"

class WebSocket final : public QObject {
    Q_OBJECT

public:
    static WebSocket& Instance();

    void Connect(const QString& host, int port, CString& user, CString& password, CString& database);
    bool IsConnected() const { return is_connected_; }

    void SendMessage(const QString& type, const QJsonObject& data);
    void SetReconnect(bool enabled = true, int max_attempts = 10);

signals:
signals:
    void SLoginResult(bool success);

private slots:
    void RConnected();
    void RDisconnected();
    void RTextMessageReceived(const QString& message);
    void RTryReconnect();
    void RSendHeartbeat();
    void RErrorOccurred(QAbstractSocket::SocketError error);

private:
    explicit WebSocket();
    ~WebSocket();

    void SendRawJson(const QJsonObject& obj);

private:
    QWebSocket socket_ {};
    QUrl server_url_ {};
    LoginInfo login_info_ {};

    QTimer reconnect_timer_ {};
    QTimer heartbeat_timer_ {};

    bool is_connected_ { false };
    bool reconnect_enabled_ { false };

    int reconnect_attempts_ { 0 };
    int max_reconnect_attempts_ { 10 };
};

#endif // WEBSOCKET_H
