#pragma once

#include "multicast.h"
#include "samples.h"

#include <QJsonObject>
#include <deque>

class Device;
class WebSocket;

using DeviceDeque = QVector<Device*>;

class DeviceManager : public QObject
{
    Q_OBJECT

public:
    DeviceManager();

    void process(const MulticastRxPacket& rx);
    Device* findDevice(const QString &name);
    const DeviceDeque& getDevices() const;
    bool activeClients() const;
    bool allDevicesInRunningState() const;

signals:
    void signalMulticastTx(MulticastTxPacket& tx);
    void signalIdle();

public slots:
    void slotSendTimeSample(const QString &client);
    void slotSampleRunStatusUpdate(const QString &client, bool active);
    void slotConnectionLost(const QString &client);
    void slotWebsocketTransmit(const QJsonObject& json);
    void slotNewWebsocketConnection();

private:
    DeviceManager(const DeviceManager&);
    DeviceDeque m_deviceDeque;
    Samples m_samples;
    bool m_multicastTime = false;
    QVector<QString> m_activeClients;
    WebSocket* m_webSocket;
};

