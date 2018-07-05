
#include "log.h"
#include "websocket.h"
#include "systemtime.h"

#include <QJsonDocument>
#include <QtWebSockets/QWebSocket>


WebSocket::WebSocket(uint16_t port)
{
    m_webSocket = new QWebSocketServer("twitse", QWebSocketServer::NonSecureMode);
    bool success = m_webSocket->listen(QHostAddress::Any, port);

    if (success)
    {
        connect(m_webSocket, &QWebSocketServer::newConnection, this, &WebSocket::slotNewConnection);
        trace->info("websocket started at port {}", port);
    }
    else
    {
        delete m_webSocket;
        m_webSocket = nullptr;
        trace->critical("websocket failed");
    }
}


WebSocket::~WebSocket()
{
    delete m_webSocket;
}


void WebSocket::slotSend(const QString &clientName, double offset_us)
{
    auto json = new QJsonObject;
    (*json)["name"] = clientName;
    int64_t now_ms = s_systemTime->getWallClock() / NS_IN_MSEC;
    (*json)["time"] = QString::number(now_ms);
    (*json)["value"] = QString::number(offset_us);

    QJsonDocument doc(*json);

    for (QWebSocket* socket : m_clients)
    {
        socket->sendTextMessage(doc.toJson());
    }

    m_webSocketHistory.append(WebSocketJson(json));

    while (m_webSocketHistory.size())
    {
        int age = now_ms - (*m_webSocketHistory.first())["time"].toString().toLongLong();
        const int minuttes = 10;
        if (age > 60000 * minuttes)
        {
            m_webSocketHistory.pop_front();
        }
        else
        {
            break;
        }
    }
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

    for (auto measurement : m_webSocketHistory)
    {
        QJsonDocument doc(*measurement);
        m_clients[0]->sendTextMessage(doc.toJson());
    }
}


void WebSocket::slotTextMessageReceived(const QString& message)
{
    trace->info("message : {}", message.toStdString());
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
