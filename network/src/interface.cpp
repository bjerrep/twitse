#include "interface.h"

#include <QNetworkInterface>


QHostAddress Interface::getLocalAddress(Interface::IfType /*ifType*/)
{
    QList<QHostAddress> address_list = QNetworkInterface::allAddresses();

    for (int i = 0; i < address_list.size(); ++i)
    {
        if (address_list.at(i) != QHostAddress::LocalHost && address_list.at(i).toIPv4Address())
        {
            return address_list.at(i);
        }
    }
    return QHostAddress(QHostAddress::LocalHost);
}
