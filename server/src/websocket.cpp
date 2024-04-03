
#include "log.h"
#include "websocket.h"
#include "systemtime.h"
#include "i2c_access.h"

#include <QJsonDocument>
#include <QCoreApplication>
#include <QtWebSockets/QWebSocket>
#include <sstream>

const int DELTAS_PER_HOUR = 4;

void WS_ClientPeriodMeasurements::add(double offset_us)
{
    m_offsets.push_back(offset_us);
}

// ---------------------------------------------------


void WS_Measurements::add(const QString &id, double offset_us)
{
    m_lastPeriod[id].add(offset_us);

    // since the clients might deliver measurements at different rates
    // its easier to maintain the largest diff on the go.
    double max = std::numeric_limits<double>::lowest();
    double min = std::numeric_limits<double>::max();

    for (const auto& period_measurement : m_lastPeriod)
    {
        const auto& offsets = period_measurement.m_offsets;
        if (offsets.empty())
        {
            continue;
        }
        min = qMin(offsets.back(), min);
        max = qMax(offsets.back(), max);
    }

    m_largestDiff = qMax(max - min, m_largestDiff);
}


WebSocketJson WS_Measurements::finalizePeriod(int64_t msec)
{
    m_measurements.append({msec, m_largestDiff});

    while (m_measurements.size() > HOURS_IN_WEEK * DELTAS_PER_HOUR)
    {
        m_measurements.removeFirst();
    }

    m_largestDiff = std::numeric_limits<double>::lowest();
    m_lastPeriod.clear();

    auto json = new QJsonObject();
    (*json)["from"] = "server";
    (*json)["command"] = "max_delta_offset";
    (*json)["time"] = QString::number(msec);
    (*json)["value"] = m_largestDiff;

    double mean_us = 0.0;
    double peak_us = 0.0;
    if (m_measurements.size())
    {
        for (auto hour : m_measurements)
        {
            mean_us += hour.m_value / m_measurements.size();
            peak_us = qMax(peak_us, hour.m_value);
        }
    }
    (*json)["mean_us"] = mean_us;
    (*json)["peak_us"] = peak_us;

    return WebSocketJson(json);
}


WebSocketJson WS_Measurements::getJson() const
{
    auto json = new QJsonObject();
    (*json)["from"] = "server";
    (*json)["command"] = "max_delta_offset_history";

    double mean_us = 0.0;
    double peak_us = 0.0;
    QString time_ms = "[";
    QString value = "[";
    if (m_measurements.size())
    {
        for (auto hour : m_measurements)
        {
            time_ms += QString::number(hour.m_sec) + ",";
            value += QString::number(hour.m_value) + ",";
            mean_us += hour.m_value / m_measurements.size();
            peak_us = qMax(peak_us, hour.m_value);
        }
        time_ms.chop(1);
        value.chop(1);
    }
    time_ms += "]";
    value += "]";

    (*json)["time"] = time_ms;
    (*json)["value"] = value;
    (*json)["mean_us"] = mean_us;
    (*json)["peak_us"] = peak_us;

    return WebSocketJson(json);
}

// ---------------------------------------------------


WebSocket::WebSocket(uint16_t port)
{
    m_webSocket = new QWebSocketServer("twitse", QWebSocketServer::NonSecureMode);
    bool success = m_webSocket->listen(QHostAddress::Any, port);

    if (success)
    {
        connect(m_webSocket, &QWebSocketServer::newConnection, this, &WebSocket::slotNewConnection);
        trace->info("websocket server started at port {}", port);
    }
    else
    {
        delete m_webSocket;
        m_webSocket = nullptr;
        trace->critical("websocket server on port {} failed", port);
    }

    m_startTime_ms = SystemTime::getWallClock_ns() / NS_IN_MSEC;
    m_period = m_startTime_ms / MSEC_IN_HOUR;
}


WebSocket::~WebSocket()
{
    m_webSocket->close();
    delete m_webSocket;
}


void WebSocket::slotNewOffsetMeasurement(const QString &id, double offset_us, double mean_abs_dev, double rms)
{
    m_longTermMeasurements.add(id, offset_us);

    int64_t now_ms = SystemTime::getWallClock_ns() / NS_IN_MSEC;

    int64_t period = now_ms / (MSEC_IN_HOUR / DELTAS_PER_HOUR);
    if (period > m_period)
    {
        m_period = period;
        auto json = m_longTermMeasurements.finalizePeriod(now_ms);
        QJsonDocument doc(*json);
        broadcast(doc.toJson());
    }

    auto json = new QJsonObject;
    (*json)["command"] = "current_offset";
    (*json)["from"] = id;
    (*json)["time"] = QString::number(now_ms);
    (*json)["value"] = QString::number(offset_us);
    (*json)["meanabsdev"] = QString::number(mean_abs_dev);
    (*json)["rms"] = QString::number(rms);

    QJsonDocument doc(*json);

    broadcast(doc.toJson());

    m_webSocketHistory.append(WebSocketJson(json));

    while (m_webSocketHistory.size())
    {
        int64_t age_ms = now_ms - (*m_webSocketHistory.first())["time"].toString().toLongLong();
        const int minuttes = 60;
        if (age_ms > 60000 * minuttes)
        {
            m_webSocketHistory.pop_front();
        }
        else
        {
            break;
        }
    }
}


void WebSocket::broadcast(const QByteArray& data)
{
    for (QWebSocket* socket : m_clients)
    {
        socket->sendTextMessage(data);
    }
}


void WebSocket::sendWallOffset()
{
    auto json = new QJsonObject;
    (*json)["from"] = "server";
    (*json)["command"] = "walloffset";

    double diff_sec = (s_systemTime->getRawSystemTime_ns() - SystemTime::getWallClock_ns()) / NS_IN_SEC_F;
    (*json)["value"] = fmt::format("{:.6f}", diff_sec).c_str();

    QJsonDocument doc(*json);
    broadcast(doc.toJson());
}


void WebSocket::slotNewConnection()
{
    QWebSocket* socket = m_webSocket->nextPendingConnection();
    m_clients.append(socket);

    connect(socket, &QWebSocket::textMessageReceived, this, &WebSocket::slotTextMessageReceived);
    connect(socket, &QWebSocket::disconnected, this, &WebSocket::slotDisconnected);

    trace->info("new websocket connection from {}:{}",
                socket->peerAddress().toString().toStdString(),
                socket->peerPort());

    emit signalNewWebsocketConnection();
    emit webSocketClientRequest("get_client_lock_quality");
}


void WebSocket::slotTextMessageReceived(const QString& message)
{
    trace->debug("message : {}", message.toStdString());
    QJsonObject json = QJsonDocument::fromJson(message.toUtf8()).object();
    QString command = json.value("command").toString();
    QString from = json.value("from").toString();

    if (from == "server")
    {
        return;
    }

    if (command == "get_history")
    {
        for (const auto& measurement : m_webSocketHistory)
        {
            QJsonDocument doc(*measurement);
            broadcast(doc.toJson());
        }
    }
    else if (command == "get_max_delta_history")
    {
        QJsonDocument doc(*m_longTermMeasurements.getJson().get());
        broadcast(doc.toJson());
    }
    else if (command == "transient_test")
    {
        s_systemTime->adjustSystemTime_ns(50000);
    }
    else if (command == "server_restart")
    {
        // just exit expecting a systemd script to respawn us
        QCoreApplication::quit();
    }
    else if (command == "get_wall_offset")
    {
        sendWallOffset();
    }
    else if (command == "get_vctcxo_dac" || command == "get_client_lock_quality")
    {
        emit webSocketClientRequest(command);
    }
    else
    {
        trace->warn("unknown command: '{}'", command.toStdString());
    }
}


void WebSocket::slotDisconnected()
{
    for (auto socket : m_clients)
    {
        if (socket == QObject::sender())
        {
            trace->info("websocket connection from {}:{} lost",
                        socket->peerAddress().toString().toStdString(),
                        socket->peerPort());
            m_clients.removeOne(socket);
            return;
        }
    }
}


void WebSocket::transmit(const QJsonObject &json)
{
    QJsonDocument doc(json);
    broadcast(doc.toJson());
}
