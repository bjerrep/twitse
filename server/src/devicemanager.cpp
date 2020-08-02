#include "devicemanager.h"
#include "device.h"
#include "log.h"
#include "websocket.h"
#include "globals.h"
#include "i2c_access.h"

#include <QTcpServer>
#include <QJsonArray>

DeviceManager::DeviceManager()
{
    connect(&m_samples, &Samples::signalSendTimeSample,
            this, &DeviceManager::slotSendTimeSample);
    connect(&m_samples, &Samples::signalSampleRunStatusUpdate,
            this, &DeviceManager::slotSampleRunStatusUpdate);
}


void DeviceManager::initialize()
{
    m_webSocket = new WebSocket(g_websocketPort);

    connect(m_webSocket, &WebSocket::webSocketClientRequest, this, &DeviceManager::slotWebSocketRequest);
}


Device* DeviceManager::findDevice(const QString& name)
{
    for(auto device : m_deviceDeque)
    {
        if (device->m_name == name)
        {
            return device;
        }
    }
    return nullptr;
}


const DeviceDeque &DeviceManager::getDevices() const
{
    return m_deviceDeque;
}


bool DeviceManager::allDevicesInRunningState() const
{
    for(auto device : m_deviceDeque)
    {
        if (device->m_initState != RUNNING)
        {
            return false;
        }
    }
    return true;
}


bool DeviceManager::idle() const
{
    return m_activeClients.empty();
}


WebSocket* DeviceManager::webSocket()
{
    return m_webSocket;
}


void DeviceManager::process(const MulticastRxPacket& rx)
{
    QString from = rx.value("from");
    QString command = rx.value("command");

    if (command == "connect")
    {
        trace->info("[{:<8}] connection request on {}", from.toStdString(), rx.value("endpoint").toStdString());
        if (findDevice(from))
        {
            trace->warn("[{:<8}] already registered - connection request ignored", from.toStdString());
            return;
        }
        Device* newDevice = new Device(this, from);
        m_deviceDeque.append(newDevice);

        connect(newDevice, &Device::signalRequestSamples, &m_samples, &Samples::slotRequestSamples);
        connect(newDevice, &Device::signalConnectionLost, this, &DeviceManager::slotConnectionLost);
        connect(newDevice, &Device::signalNewOffsetMeasurement, m_webSocket, &WebSocket::slotNewOffsetMeasurement);
        connect(m_webSocket, &WebSocket::signalNewWebsocketConnection, this, &DeviceManager::slotNewWebsocketConnection);
        connect(&newDevice->m_lock, &Lock::signalNewLockQuality, this, &DeviceManager::slotNewLockQuality);

        QJsonObject json;
        json["to"] = from;
        json["command"] = "serveraddress";
        json["tcpaddress"] = newDevice->m_serverAddress;
        json["tcpport"] = QString::number(newDevice->m_server->serverPort());
        MulticastTxPacket udp(json);
        emit signalMulticastTx(udp);
    }
}


void DeviceManager::slotSendTimeSample(const QString& client)
{
    Device* device = findDevice(client);
    if (device)
    {
        device->sendServerTimeUDP();
    }
    else
    {
        trace->warn("internal error #0080");
    }
}


void DeviceManager::slotSampleRunStatusUpdate(const QString& client, bool active)
{
    if (active)
    {
        m_activeClients.append(client);
    }
    else
    {
        m_activeClients.removeAll(client);

        Device* device = findDevice(client);
        if (device)
        {
            device->getClientOffset();
        }
        if (m_activeClients.empty())
        {
            emit signalIdle();
        }
    }
}


void DeviceManager::slotConnectionLost(const QString& client)
{
    for (int i = 0; i < m_deviceDeque.size(); i++)
    {
        if (m_deviceDeque.at(i)->m_name == client)
        {
            m_samples.removeClient(client);
            Device* device = m_deviceDeque.takeAt(i);
            device->deleteLater();
            trace->warn(RED "{} connection lost, removing client. Clients connected: {}" RESET,
                        client.toStdString(),
                        m_deviceDeque.size());
            return;
        }
    }

    trace->error("got connection lost on unknown client '{}'", client.toStdString());
}


void DeviceManager::slotNewWebsocketConnection()
{
    for(auto device : m_deviceDeque)
    {
        device->slotSendStatus();
    }
}


void DeviceManager::slotWebsocketTransmit(const QJsonObject &json)
{
    m_webSocket->transmit(json);
}


void DeviceManager::slotWebSocketRequest(QString request)
{
    if (request == "get_vctcxo_dac")
    {
        sendVctcxoDac("server", QString::number(I2C_Access::I2C()->getVCTCXO_DAC()));

        QJsonObject json;
        json["to"] = "all";
        json["command"] = "control";
        json["action"] = "vctcxodac";
        json["value"] = "get";
        MulticastTxPacket udp(json);
        emit signalMulticastTx(udp);
    }
    else if (request == "get_client_lock_quality")
    {
        slotNewLockQuality("*");
    }
}


/// Send the given dac value to the web page through the websocket
///
void DeviceManager::sendVctcxoDac(const QString& from, const QString& value)
{
    trace->trace("sending vctcxo dac, {}={}", from.toStdString(), value.toStdString());

    QJsonObject json;
    json["name"] = "server";
    json["command"] = "vctcxodac";
    json["from"] = from;
    json["value"] = value;

    QJsonDocument doc(json);
    m_webSocket->broadcast(doc.toJson());
}


void DeviceManager::slotNewLockQuality(const QString& name)
{
    for(auto device : m_deviceDeque)
    {
        if (device->m_name == name || name == "*")
        {
            auto json = QJsonObject();
            (json)["from"] = device->m_name;
            (json)["command"] = "device_lock_quality";
            (json)["value"] = device->m_lock.getQuality();
            m_webSocket->transmit(json);
        }
    }
}
