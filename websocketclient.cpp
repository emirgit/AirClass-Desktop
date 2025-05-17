#include "websocketclient.h"
#include <QDebug>

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
    qDebug() << "Message received from server:" << message;
    emit messageReceived(message);
}

void WebSocketClient::onError(QAbstractSocket::SocketError error)
{
    QString errorMessage = m_webSocket.errorString();
    qWarning() << "WebSocket error:" << error << "-" << errorMessage;
    emit this->error(errorMessage);
}
