/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCCNETWORKCONNECTION_H
#define LCCNETWORKCONNECTION_H

#include <QObject>
#include <QTcpSocket>

#include "lccqioconnection.h"
#include "lcc-gridconnect.h"

class LCCNetworkConnection : public LCCQIoConnection
{
    Q_OBJECT
public:
    explicit LCCNetworkConnection(QObject *parent = nullptr);

    ~LCCNetworkConnection();

    void connectToRemote(QHostAddress addr, uint16_t port);

Q_SIGNALS:

private Q_SLOTS:
    void stateChanged(QAbstractSocket::SocketState state);

};

#endif // LCCNETWORKCONNECTION_H
