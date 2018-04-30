#pragma once

#include <QHostAddress>

class Interface
{
public:

    enum IfType {
        WIRED_ONLY,
        ANY
    };

    static QHostAddress getLocalAddress(IfType = IfType::ANY);
};

