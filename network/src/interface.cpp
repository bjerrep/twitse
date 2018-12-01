#include "interface.h"

#include <QNetworkInterface>


QHostAddress Interface::getLocalAddress(Interface::IfType /*ifType*/)
{
    QList<QHostAddress> address_list = QNetworkInterface::allAddresses();

    for (const auto& address : address_list)
    {
        if (address != QHostAddress::LocalHost && address.toIPv4Address())
        {
            return address;
        }
    }
    return QHostAddress(QHostAddress::LocalHost);
}
