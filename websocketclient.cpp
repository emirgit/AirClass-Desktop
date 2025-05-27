#include "websocketclient.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>

WebSocketClient::WebSocketClient(QObject *parent)
    : QObject(parent)
    , m_connected(false)
    , m_reconnectAttempts(0)
{
    // Connect WebSocket signals to slots
    connect(&m_webSocket, &QWebSocket::connected, this, &WebSocketClient::onConnected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &WebSocketClient::onDisconnected);
    connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &WebSocketClient::onTextMessageReceived);
    connect(&m_webSocket, &QWebSocket::errorOccurred, this, &WebSocketClient::onError);
    connect(&m_webSocket, &QWebSocket::pong, this, &WebSocketClient::onPong);

    // Setup ping timer with a shorter interval for more reliable connection
    connect(&m_pingTimer, &QTimer::timeout, this, &WebSocketClient::sendPing);
    m_pingTimer.setInterval(PING_INTERVAL);

    // Setup reconnect timer
    connect(&m_reconnectTimer, &QTimer::timeout, this, &WebSocketClient::tryReconnect);
    m_reconnectTimer.setInterval(RECONNECT_INTERVAL);

    // Start ping timer immediately to keep connection alive
    m_pingTimer.start();
    m_reconnectTimer.setSingleShot(true); // Ensure it only runs once per attempt
    m_reconnectTimer.setInterval(RECONNECT_INTERVAL);
    // m_reconnectTimer.start(); // Start the reconnect timer
    // qDebug() << "WebSocketClient initialized, ping timer started with interval:" << PING_INTERVAL / 1000 << "seconds";
    // m_webSocket.setPingInterval(PING_INTERVAL); // Set ping interval for WebSocket
    // m_webSocket.setPongTimeout(PING_INTERVAL + 5000); // Set a timeout for pong response
    // qDebug() << "WebSocketClient initialized, pong timeout set to:" << (PING_INTERVAL + 5000) / 1000 << "seconds";

    m_serverUrl = "ws://localhost:8082"; // Initialize server URL as empty
    // Connect the WebSocket to the server URL if provided
    //qDebug() << "Connecting to server atilkter:" << m_serverUrl;
    //m_webSocket.open(QUrl(m_serverUrl));
    //qDebug() << "WebSocketClient initialized, connecting to server at:" << m_serverUrl;
    // send deneme mesage
    //m_webSocket.sendTextMessage("WebSocketClient initialized, connecting to server at: " + m_serverUrl);
    //qDebug() << "WebSocketClient initialized, mesaj gonderildi:" << m_serverUrl;
}

WebSocketClient::~WebSocketClient()
{
    m_pingTimer.stop();
    m_reconnectTimer.stop();
    if (m_connected) {
        m_webSocket.close();
    }
}

void WebSocketClient::connectToServer(const QString &url)
{
    qDebug() << "Attempting to connect to server at:" << url;
    if (m_connected  || m_webSocket.state() == QAbstractSocket::ConnectedState) {
        qDebug() << "Already connected to server";
        emit connected(); // Re-emit connected signal to update UI
        return;
    }

    qDebug() << "Connecting to server at222:" << url;

    m_serverUrl = url;
    qDebug() << "Connecting to server at:" << url;
    
    // Reset reconnection attempts when explicitly connecting
    m_reconnectAttempts = 0;
    m_reconnectTimer.stop(); // Stop any existing reconnection timer
    m_webSocket.open(QUrl(url));
    qDebug() << "WebSocketClient CONNN to server at BAKKK:" << url;
}

void WebSocketClient::disconnect()
{
    m_pingTimer.stop();
    m_reconnectTimer.stop();
    m_reconnectAttempts = MAX_RECONNECT_ATTEMPTS; // Prevent auto-reconnect
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
    return m_connected && m_webSocket.state() == QAbstractSocket::ConnectedState;
}

void WebSocketClient::onConnected()
{
    qDebug() << "WebSocket connected to server at:" << m_serverUrl;
    m_connected = true;
    m_reconnectAttempts = 0;
    m_reconnectTimer.stop();

    qDebug() << "WebSocket state after connection:" << m_webSocket.state();
    
    // Start ping timer immediately after connection
    m_pingTimer.start();
    
    qDebug() << "Connected to WebSocket server. Ping timer started.";
    // emit connected();
}

void WebSocketClient::onDisconnected()
{
    qDebug() << "Disconnected, WebSocket state:" << m_webSocket.state();
    m_connected = false;
    m_pingTimer.stop();
    emit disconnected();
    if (m_reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
        m_reconnectTimer.start();
    }
}

void WebSocketClient::tryReconnect()
{
    if (m_connected) {
        qDebug() << "Reconnection attempt skipped. WebSocket is already connected.";
        m_reconnectTimer.stop();
        return;
    }

    m_reconnectAttempts++;
    qDebug() << "Attempting to reconnect... (Attempt" << m_reconnectAttempts << "of" << MAX_RECONNECT_ATTEMPTS << ")";
    
    if (m_reconnectAttempts <= MAX_RECONNECT_ATTEMPTS) {
        qDebug() << "Reconnecting to server at:" << m_serverUrl;
        m_webSocket.open(QUrl(m_serverUrl));
    } else {
        qDebug() << "Maximum reconnection attempts reached. Stopping reconnection timer.";
        m_reconnectTimer.stop();
    }
}

void WebSocketClient::sendPing()
{
    if (m_connected) {
        qDebug() << "Sending ping to server to keep connection alive.";
        m_webSocket.ping();
    } else {
        qDebug() << "Ping not sent. WebSocket is not connected.";
    }
}

void WebSocketClient::onPong(quint64 elapsedTime, const QByteArray &payload)
{
    if (m_connected) {
        qDebug() << "Pong received, elapsed time:" << elapsedTime << "ms, payload size:" << payload.size();
        // Reset ping timer on successful pong
        m_pingTimer.start();
    }
}

void WebSocketClient::onTextMessageReceived(const QString &message)
{
    qDebug() << "Raw message received from server:" << message;
    qDebug() << "Processing message...";
    
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
        
        qDebug() << "Gesture received:" << gestureType << "from client:" << clientId << "at" << timestamp;
        emit gestureReceived(gestureType, clientId, timestamp);
    }
    else if (type == "page_navigation") {
        QString action = obj["action"].toString();
        QString clientId = obj["client_id"].toString();
        QString timestamp = obj["timestamp"].toString();
        
        qDebug() << "Page navigation action received:" << action << "from client:" << clientId << "at" << timestamp;
        emit pageNavigationReceived(action, clientId, timestamp);
    }
    else if (type == "drawing") {
        double x = obj["x"].toDouble();
        double y = obj["y"].toDouble();
        bool isStart = obj["isStart"].toBool();
        QString color = obj["color"].toString();
        int width = obj["width"].toInt();
        
        qDebug() << "Emitting drawing signal:" << x << y << isStart << color << width;
        emit drawingReceived(x, y, isStart, color, width);
    }
    else {
        // For other message types, emit the general messageReceived signal
        qDebug() << "Unhandled message type received:" << type;
        emit messageReceived(message);
    }
}

void WebSocketClient::onError(QAbstractSocket::SocketError error)
{
    QString errorMessage = m_webSocket.errorString();
    qWarning() << "WebSocket error occurred. Error code:" << error << "Message:" << errorMessage;
    emit this->error(errorMessage);
}
