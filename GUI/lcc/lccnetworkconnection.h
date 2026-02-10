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

    void setRemote(QHostAddress addr, uint16_t port);

    bool open();

Q_SIGNALS:

private Q_SLOTS:
    void stateChanged(QAbstractSocket::SocketState state);

private:
    QHostAddress m_addr;
    uint16_t m_port;
};

#endif // LCCNETWORKCONNECTION_H
