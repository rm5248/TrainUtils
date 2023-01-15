/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCCNETWORKCONNECTION_H
#define LCCNETWORKCONNECTION_H

#include <QObject>
#include <QTcpSocket>

#include "lccconnection.h"
#include "lcc-gridconnect.h"

class LCCNetworkConnection : public LCCConnection
{
    Q_OBJECT
public:
    explicit LCCNetworkConnection(QObject *parent = nullptr);

    ~LCCNetworkConnection();

    void connectToRemote(QHostAddress addr, uint16_t port);

Q_SIGNALS:

private Q_SLOTS:
    void stateChanged(QAbstractSocket::SocketState state);
    void incomingData();
    void generateAliasDone();

private:
    static void writeLCCFrameCB(lcc_context* context, lcc_can_frame* frame);
    void writeLCCFrame(lcc_can_frame* frame);
    static void gridconnectLCCFrameParsedCB(lcc_gridconnect* context, lcc_can_frame* frame);
    void gridconnectLCCFrameParsed(lcc_can_frame* frame);

private:
    lcc_gridconnect* m_lccGrid;
    QTcpSocket m_socket;
};

#endif // LCCNETWORKCONNECTION_H
