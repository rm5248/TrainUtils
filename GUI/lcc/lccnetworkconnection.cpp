/* SPDX-License-Identifier: GPL-2.0 */
#include <QHostAddress>
#include <QTimer>
#include <QSettings>

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

void LCCNetworkConnection::setRemote(QHostAddress addr, uint16_t port){
    LOG4CXX_DEBUG_FMT(logger, "LCC set remote to {}:{}", addr.toString().toStdString(), port );
    m_addr = addr;
    m_port = port;
}

bool LCCNetworkConnection::open(){
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(m_ioDevice);
    socket->connectToHost(m_addr, m_port);

    return true;
}

void LCCNetworkConnection::stateChanged(QAbstractSocket::SocketState state){
    LOG4CXX_DEBUG_FMT(logger, "LCC socket state: {}", (int)state);

    if(state == QAbstractSocket::ConnectedState){
        connectedToSystem();
        generateAlias();
    }else if(state == QAbstractSocket::UnconnectedState ||
             state == QAbstractSocket::ClosingState){
        // TODO we should probably try to reconnect if we become disconnected
        disconnectedFromSystem();
    }
}

void LCCNetworkConnection::doSave(QSettings &settings){

}

