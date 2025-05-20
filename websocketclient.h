#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include <QObject>
#include <QtWebSockets/QtWebSockets>
#include <QtWebSockets/QWebSocket>
#include <QString>
#include <QUrl>

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
    void messageReceived(const QString &message);
    void error(const QString &errorMessage);
    void gestureReceived(const QString &gestureType, const QString &clientId, const QString &timestamp);
    void pageNavigationReceived(const QString &action, const QString &clientId, const QString &timestamp);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString &message);
    void onError(QAbstractSocket::SocketError error);

private:
    QWebSocket m_webSocket;
    bool m_connected;
};

#endif // WEBSOCKETCLIENT_H
