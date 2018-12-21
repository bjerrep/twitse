#include "devicemanager.h"
#include "device.h"
#include "log.h"
#include "websocket.h"

#include <QTcpServer>

DeviceManager::DeviceManager()
{
    connect(&m_samples, &Samples::signalSendTimeSample,
            this, &DeviceManager::slotSendTimeSample);
    connect(&m_samples, &Samples::signalSampleRunStatusUpdate,
            this, &DeviceManager::slotSampleRunStatusUpdate);

    m_webSocket = new WebSocket(12343);
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
        connect(newDevice, &Device::signalWebsocketTransmit, m_webSocket, &WebSocket::slotTransmit);
        connect(m_webSocket, &WebSocket::signalNewWebsocketConnection, this, &DeviceManager::slotNewWebsocketConnection);

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
    m_webSocket->slotTransmit(json);
}
