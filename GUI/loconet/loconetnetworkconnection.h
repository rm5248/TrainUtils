/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LOCONETNETWORKCONNECTION_H
#define LOCONETNETWORKCONNECTION_H

#include <QObject>
#include <QTcpSocket>

#include "loconetconnection.h"
#include "loconet-tcp.h"

class LoconetNetworkConnection : public LoconetConnection
{
    Q_OBJECT
public:
    explicit LoconetNetworkConnection(QObject *parent = nullptr);

    ~LoconetNetworkConnection();

    void connectToRemote(QHostAddress addr, uint16_t port);

Q_SIGNALS:

private Q_SLOTS:
    void stateChanged(QAbstractSocket::SocketState state);
    void incomingData();

protected:
    void writeData(uint8_t* data, int len);

private:
    void writeDataTCP(uint8_t* data, int len);
    void incomingRawData(const std::vector<uint8_t>& vec);

private:
    QTcpSocket m_socket;
    loconet::LoconetTCP m_loconetTCP;
    loconet_message m_messageBuffer;
};

#endif // LOCONETNETWORKCONNECTION_H
