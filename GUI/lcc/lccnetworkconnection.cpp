/* SPDX-License-Identifier: GPL-2.0 */
#include <QHostAddress>
#include <QTimer>

#include <log4cxx/logger.h>
#include <fmt/format.h>

#include "lccnetworkconnection.h"

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui.lcc.LCCNetworkConnection" );

LCCNetworkConnection::LCCNetworkConnection(QObject *parent) : LCCQIoConnection(parent)
{
    QTcpSocket* socket = new QTcpSocket(this);
    m_ioDevice = socket;
    connect( socket, &QAbstractSocket::stateChanged,
             this, &LCCNetworkConnection::stateChanged);

    updateQIODeviceConnections();
}

LCCNetworkConnection::~LCCNetworkConnection(){
}

void LCCNetworkConnection::connectToRemote(QHostAddress addr, uint16_t port){
    LOG4CXX_DEBUG_FMT(logger, "LCC Connecting to {}:{}", addr.toString().toStdString(), port );
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(m_ioDevice);
    socket->connectToHost(addr, port);
}

void LCCNetworkConnection::stateChanged(QAbstractSocket::SocketState state){
    LOG4CXX_DEBUG_FMT(logger, "LCC socket state: {}", state);

    if(state == QAbstractSocket::ConnectedState){
        connectedToSystem();
        generateAlias();
    }else if(state == QAbstractSocket::UnconnectedState ||
             state == QAbstractSocket::ClosingState){
        // TODO we should probably try to reconnect if we become disconnected
        disconnectedFromSystem();
    }
}


