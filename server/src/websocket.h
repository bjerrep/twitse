#pragma once

#include <QtWebSockets/QWebSocketServer>
#include <QJsonObject>

using WebSocketJson = QSharedPointer<QJsonObject>;

class QWebSocket;

class WebSocket : public QObject
{
    Q_OBJECT

public:
    WebSocket(uint16_t port);
    ~WebSocket();

    void slotSend(const QString& clientName, double offset_us);

signals:
    void signalNewWebsocketConnection();

public slots:
    void slotNewConnection();
    void slotTextMessageReceived(const QString&);
    void slotDisconnected();
    void slotTransmit(const QJsonObject& json);

private:
    QWebSocketServer* m_webSocket;
    QVector<QWebSocket*> m_clients;
    QVector<WebSocketJson> m_webSocketHistory;
};

