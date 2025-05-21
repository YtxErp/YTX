#include "websocket.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

#include "component/constvalue.h"

WebSocket::WebSocket()
{
    connect(&socket_, &QWebSocket::connected, this, &WebSocket::RConnected);
    connect(&socket_, &QWebSocket::disconnected, this, &WebSocket::RDisconnected);
    connect(&socket_, &QWebSocket::textMessageReceived, this, &WebSocket::RTextMessageReceived);
    connect(&socket_, &QWebSocket::errorOccurred, this, &WebSocket::RErrorOccurred);

    reconnect_timer_.setInterval(3000);
    reconnect_timer_.setSingleShot(true);
    connect(&reconnect_timer_, &QTimer::timeout, this, &WebSocket::RTryReconnect);

    heartbeat_timer_.setInterval(5000);
    connect(&heartbeat_timer_, &QTimer::timeout, this, &WebSocket::RSendHeartbeat);
}

WebSocket::~WebSocket()
{
    if (socket_.isValid()) {
        socket_.close();
        qDebug() << "WebSocket connection closed in destructor.";
    }
}

WebSocket& WebSocket::Instance()
{
    static WebSocket instance;
    return instance;
}

void WebSocket::Connect(const QString& host, int port, CString& user, CString& password, CString& database)
{
    const QString url_str { QString("ws://%1:%2").arg(host, QString::number(port)) };
    server_url_ = QUrl(url_str);
    reconnect_attempts_ = 0;

    if (socket_.state() != QAbstractSocket::UnconnectedState) {
        socket_.abort();
    }

    login_info_.user = user;
    login_info_.password = password;
    login_info_.database = database;

    socket_.open(server_url_);
}

void WebSocket::RConnected()
{
    is_connected_ = true;
    reconnect_attempts_ = 0;

    QJsonObject login_data {};
    login_data["user"] = login_info_.user;
    login_data["password"] = login_info_.password;
    login_data["database"] = login_info_.database;

    SendMessage("login", login_data);
}

void WebSocket::RErrorOccurred(QAbstractSocket::SocketError error)
{
    is_connected_ = false;
    qWarning() << "WebSocket error:" << error << "-" << socket_.errorString();
}

void WebSocket::SetReconnect(bool enabled, int max_attempts)
{
    reconnect_enabled_ = enabled;
    max_reconnect_attempts_ = max_attempts;
}

void WebSocket::SendMessage(const QString& type, const QJsonObject& data)
{
    if (!is_connected_) {
        qWarning() << "Cannot send message: WebSocket is not connected";
        return;
    }

    QJsonObject message;
    message[kMsgType] = type;
    message[kMsgData] = data;
    SendRawJson(message);
}

void WebSocket::SendRawJson(const QJsonObject& obj)
{
    if (!is_connected_) {
        qWarning() << "Cannot send raw JSON: WebSocket is not connected";
        return;
    }

    QJsonDocument doc(obj);
    socket_.sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

void WebSocket::RDisconnected()
{
    is_connected_ = false;
    if (reconnect_enabled_ && reconnect_attempts_ != max_reconnect_attempts_) {
        reconnect_timer_.start();
    }
}

void WebSocket::RTryReconnect()
{
    ++reconnect_attempts_;
    qDebug() << "Trying to reconnect (" << reconnect_attempts_ << ")";
    socket_.open(server_url_);
}

void WebSocket::RSendHeartbeat()
{
    if (!is_connected_) {
        return;
    }

    QJsonObject ping;
    ping[kMsgType] = QStringLiteral("heartbeat");
    ping[kMsgData] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    SendRawJson(ping);
}

void WebSocket::RTextMessageReceived(const QString& message)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "Invalid JSON message:" << message;
        return;
    }

    QJsonObject obj = doc.object();
    QString type = obj.value(kMsgType).toString();
    QJsonObject data = obj.value(kMsgData).toObject();

    if (type == "login") {
        is_connected_ = data.value("status").toBool();
        emit SLoginResult(is_connected_);
        qDebug() << "Login result:" << (is_connected_ ? "success" : "failed");
    }
}
