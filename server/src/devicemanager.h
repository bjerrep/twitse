#pragma once

#include "multicast.h"
#include "samples.h"

#include <QJsonObject>
#include <deque>

class Device;
class WebSocket;

using DeviceDeque = QVector<Device*>;

/// The DeviceManager is a utility class for the Server containing the list of all connected clients.
/// Why it also ended up beeing the owner of the websocket instance is right now a mystery.
///
class DeviceManager : public QObject
{
    Q_OBJECT

public:
    DeviceManager();

    void initialize();

    void process(const MulticastRxPacket& rx);
    Device* findDevice(const QString &name);
    const DeviceDeque& getDevices() const;
    bool allDevicesInRunningState() const;
    bool idle() const;
    WebSocket* webSocket();
    void sendVctcxoDac(const QString& from, const QString& value);

signals:
    void signalMulticastTx(MulticastTxPacket& tx);
    void signalIdle();

public slots:
    void slotSendTimeSample(const QString &client);
    void slotSampleRunStatusUpdate(const QString &client, bool active);
    void slotConnectionLost(const QString &client);
    void slotWebsocketTransmit(const QJsonObject& json);
    void slotNewWebsocketConnection();
    void slotNewLockQuality(const QString& name);
    void slotWebSocketRequest(QString request);

private:
    DeviceManager(const DeviceManager&);
    DeviceDeque m_deviceDeque;
    Samples m_samples;
    bool m_multicastTime = false;
    QVector<QString> m_activeClients;
    WebSocket* m_webSocket;
};
