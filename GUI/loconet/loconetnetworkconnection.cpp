/* SPDX-License-Identifier: GPL-2.0 */
#include "loconetnetworkconnection.h"

#include <log4cxx/logger.h>
#include <fmt/format.h>
#include <QHostAddress>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui.loconet.LoconetNetworkConnection" );

LoconetNetworkConnection::LoconetNetworkConnection(QObject *parent) : LoconetConnection(parent)
{
//    m_loconetTCP.setWriteDataFunction(std::bind(&LoconetNetworkConnection::writeDataTCP, this, std::placeholders::_1, std::placeholders::_2));
    m_loconetTCP.setReceiveCallback(std::bind(&LoconetNetworkConnection::incomingRawData, this, std::placeholders::_1));

    connect( &m_socket, &QAbstractSocket::stateChanged,
             this, &LoconetNetworkConnection::stateChanged);
    connect( &m_socket, &QAbstractSocket::readyRead,
             this, &LoconetNetworkConnection::incomingData );
}

LoconetNetworkConnection::~LoconetNetworkConnection(){}

void LoconetNetworkConnection::connectToRemote(QHostAddress addr, uint16_t port){
    LOG4CXX_DEBUG_FMT(logger, "Loconet Connecting to {}:{}", addr.toString().toStdString(), port );
    m_socket.connectToHost(addr, port);
}

void LoconetNetworkConnection::stateChanged(QAbstractSocket::SocketState state){
    LOG4CXX_DEBUG_FMT(logger, "Loconet socket state: {}", state);

    if(state == QAbstractSocket::ConnectedState){
        connectedToSystem();
    }else if(state == QAbstractSocket::UnconnectedState){
        disconnectedFromSystem();
    }
}

void LoconetNetworkConnection::incomingData(){
    QByteArray data = m_socket.readAll();

    m_loconetTCP.incomingData(data.data(), data.size());

    int haveMessage;
    do{
         haveMessage = ln_read_message(m_loconetTCP.loconetContext(), &m_messageBuffer);
    }while(haveMessage == 1);
}

void LoconetNetworkConnection::writeData(uint8_t* data, int len){
    if(m_socket.state() != QAbstractSocket::ConnectedState){
        return;
    }

    m_socket.write((const char*)data, len);
}

void LoconetNetworkConnection::writeDataTCP(uint8_t* data, int len){
    if(m_socket.state() != QAbstractSocket::ConnectedState){
        return;
    }

    m_socket.write((const char*)data, len);
}

void LoconetNetworkConnection::incomingRawData(const std::vector<uint8_t>& vec){
    QByteArray ba;

    for(uint8_t byte : vec){
        ba.push_back(byte);
    }

    Q_EMIT incomingRawPacket(ba);
}
