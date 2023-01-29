/* SPDX-License-Identifier: GPL-2.0 */
#include "loconetserialconnection.h"

#include <QSerialPortInfo>

#include <log4cxx/logger.h>
#include <fmt/format.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui.loconet.LoconetSerialConnection" );

LoconetSerialConnection::LoconetSerialConnection(QObject *parent) : LoconetConnection(parent)
{
    connect(&m_serialPort, &QSerialPort::readyRead,
            this, &LoconetSerialConnection::dataAvailable);
}

void LoconetSerialConnection::setSerialPortName(QString port){
    if(m_serialPort.isOpen()){
        m_serialPort.close();
    }

    m_serialPort.setPortName(port);

    if(!m_serialPort.open(QIODevice::ReadWrite)){
        LOG4CXX_ERROR_FMT(logger, "Can't open {}: {}",
                          port.toStdString(),
                          m_serialPort.errorString().toStdString() );
        return;
    }

    connectedToSystem();
}

void LoconetSerialConnection::writeData(uint8_t* data, int len){
    if(m_serialPort.isOpen()){
        m_serialPort.write((const char*)data, len);
    }
}

void LoconetSerialConnection::dataAvailable(){
    QByteArray ba = m_serialPort.readAll();

    for(uint8_t byte : ba){
        loconet_context_incoming_byte(m_locoContext, byte);
    }

    loconet_context_process(m_locoContext);
}
