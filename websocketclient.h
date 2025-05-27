#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include <QObject>
#include <QtWebSockets/QtWebSockets>
#include <QtWebSockets/QWebSocket>
#include <QString>
#include <QUrl>
#include <QTimer>

class WebSocketClient : public QObject
{
    Q_OBJECT
public:
    explicit WebSocketClient(QObject *parent = nullptr);
    ~WebSocketClient();

    void connectToServer(const QString &url);
    void disconnect();
    void sendMessage(const QString &message);
    bool isConnected() const;

signals:
    void connected();
    void disconnected();
    void error(const QString &message);
    void messageReceived(const QString &message);
    void gestureReceived(const QString &gestureType, const QString &clientId, const QString &timestamp);
    void pageNavigationReceived(const QString &action, const QString &clientId, const QString &timestamp);
    void drawingReceived(double x, double y, bool isStart, const QString &color, int width);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString &message);
    void onError(QAbstractSocket::SocketError error);
    void onPong(quint64 elapsedTime, const QByteArray &payload);
    void sendPing();
    void tryReconnect();

private:
    QWebSocket m_webSocket;
    bool m_connected;
    QString m_serverUrl;
    QTimer m_pingTimer;
    QTimer m_reconnectTimer;
    int m_reconnectAttempts;
    static const int MAX_RECONNECT_ATTEMPTS = 50;
    static const int RECONNECT_INTERVAL = 5000; // 5 seconds
    static const int PING_INTERVAL = 30000; // 30 seconds
};

#endif // WEBSOCKETCLIENT_H
