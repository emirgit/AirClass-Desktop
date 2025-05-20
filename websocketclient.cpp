#include "websocketclient.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

WebSocketClient::WebSocketClient(QObject *parent)
    : QObject(parent)
    , m_connected(false)
{
    // Connect WebSocket signals to slots
    connect(&m_webSocket, &QWebSocket::connected, this, &WebSocketClient::onConnected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &WebSocketClient::onDisconnected);
    connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &WebSocketClient::onTextMessageReceived);
    connect(&m_webSocket, &QWebSocket::errorOccurred, this, &WebSocketClient::onError);
}

WebSocketClient::~WebSocketClient()
{
    if (m_connected) {
        m_webSocket.close();
    }
}

void WebSocketClient::connectToServer(const QString &url)
{
    if (m_connected) {
        qDebug() << "Already connected to server";
        return;
    }

    qDebug() << "Connecting to server at:" << url;
    m_webSocket.open(QUrl(url));
}

void WebSocketClient::disconnect()
{
    if (!m_connected) {
        qDebug() << "Not connected to any server";
        return;
    }

    qDebug() << "Disconnecting from server";
    m_webSocket.close();
}

void WebSocketClient::sendMessage(const QString &message)
{
    if (!m_connected) {
        qWarning() << "Cannot send message: Not connected to server";
        emit error("Cannot send message: Not connected to server");
        return;
    }

    qDebug() << "Sending message to server:" << message;
    m_webSocket.sendTextMessage(message);
}

bool WebSocketClient::isConnected() const
{
    return m_connected;
}

void WebSocketClient::onConnected()
{
    m_connected = true;
    qDebug() << "Connected to WebSocket server";
    emit connected();
}

void WebSocketClient::onDisconnected()
{
    m_connected = false;
    qDebug() << "Disconnected from WebSocket server";
    emit disconnected();
}

void WebSocketClient::onTextMessageReceived(const QString &message)
{
    qDebug() << "Raw message received from server:" << message;
    
    // Parse the JSON message
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull()) {
        qWarning() << "Invalid JSON message received";
        return;
    }

    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();

    qDebug() << "Processing message type:" << type;

    if (type == "gesture") {
        QString gestureType = obj["gesture_type"].toString();
        QString clientId = obj["client_id"].toString();
        QString timestamp = obj["timestamp"].toString();
        
        qDebug() << "Emitting gesture signal:" << gestureType << "from client:" << clientId;
        emit gestureReceived(gestureType, clientId, timestamp);
    }
    else if (type == "page_navigation") {
        QString action = obj["action"].toString();
        QString clientId = obj["client_id"].toString();
        QString timestamp = obj["timestamp"].toString();
        
        qDebug() << "Emitting page navigation signal:" << action << "from client:" << clientId;
        emit pageNavigationReceived(action, clientId, timestamp);
    }
    else {
        // For other message types, emit the general messageReceived signal
        qDebug() << "Emitting general message signal for type:" << type;
        emit messageReceived(message);
    }
}

void WebSocketClient::onError(QAbstractSocket::SocketError error)
{
    QString errorMessage = m_webSocket.errorString();
    qWarning() << "WebSocket error:" << error << "-" << errorMessage;
    emit this->error(errorMessage);
}
