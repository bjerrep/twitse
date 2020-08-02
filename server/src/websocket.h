#pragma once

#include "multicast.h"
#include <QtWebSockets/QWebSocketServer>
#include <QJsonObject>


using WebSocketJson = QSharedPointer<QJsonObject>;

class QWebSocket;


class WS_ClientPeriodMeasurements
{
public:
    WS_ClientPeriodMeasurements() {}

    void add(double offset_us);

    QVector<double> m_offsets;
};


class WS_Measurements
{
public:
    void add(const QString &id, double offset_us);

    WebSocketJson finalizePeriod(int64_t m_sec);

    WebSocketJson getJson() const;

    struct i_struct
    {
        int64_t m_sec;
        double m_value;
    };

    double m_largestDiff = std::numeric_limits<double>::lowest();
    QMap<QString, WS_ClientPeriodMeasurements> m_lastPeriod;
    QVector<i_struct> m_measurements;
};


class WebSocket : public QObject
{
    Q_OBJECT

public:
    WebSocket(uint16_t port);
    ~WebSocket();

    void slotNewOffsetMeasurement(const QString& id, double offset_us, double mean_abs_dev, double rms);
    void broadcast(const QByteArray &data);
    void transmit(const QJsonObject& json);

private:
    void sendWallOffset();

signals:
    void signalNewWebsocketConnection();
    void webSocketClientRequest(QString command);

public slots:
    void slotNewConnection();
    void slotTextMessageReceived(const QString&);
    void slotDisconnected();

private:
    QWebSocketServer* m_webSocket;
    QVector<QWebSocket*> m_clients;
    QVector<WebSocketJson> m_webSocketHistory;
    WS_Measurements m_longTermMeasurements;
    int64_t m_period = 0;
    int64_t m_startTime_ms = 0;
};
