/* SPDX-License-Identifier: GPL-2.0 */
#include "lccserialconnection.h"

#include <QTimer>

#include <log4cxx/logger.h>
#include <fmt/format.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui.lcc.LCCNetworkConnection" );

LCCSerialConnection::LCCSerialConnection(QObject *parent) : LCCQIoConnection(parent)
{
    QSerialPort* serialPort = new QSerialPort(this);
    m_ioDevice = serialPort;
    updateQIODeviceConnections();
}

LCCSerialConnection::~LCCSerialConnection(){}

void LCCSerialConnection::connectToSerialPort(QString serialPort){
    LOG4CXX_DEBUG_FMT(logger, "LCC Connecting to {}", serialPort.toStdString() );

    QSerialPort* serial = static_cast<QSerialPort*>(m_ioDevice);
    serial->setPortName(serialPort);
}

bool LCCSerialConnection::open(){
    QSerialPort* serial = static_cast<QSerialPort*>(m_ioDevice);
    bool open = serial->open(QIODevice::ReadWrite);
    if(!open){
        m_error = serial->errorString();
        LOG4CXX_WARN_FMT(logger, "Can't connect to port {}:{}",
                         serial->portName().toStdString(),
                         serial->errorString().toStdString());
        return open;
    }

    connectedToSystem();
    generateAlias();

    return open;
}

