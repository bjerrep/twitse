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

    void slotSendOffsetMeasurement(const QString& clientName, double offset_us, double mean_abs_dev, double rms);

private:
    void sendWallOffset();

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

