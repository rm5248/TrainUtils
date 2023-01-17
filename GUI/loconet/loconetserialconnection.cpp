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

//    QSerialPortInfo infToOpen;

//    for(QSerialPortInfo& inf : QSerialPortInfo::availablePorts()){
//        if(inf.portName() == port){
//            infToOpen = inf;
//            break;
//        }
//    }

    m_serialPort.setPortName(port);

    if(!m_serialPort.open(QIODevice::ReadWrite)){
        LOG4CXX_ERROR_FMT(logger, "Can't open {}: {}",
                          port.toStdString(),
                          m_serialPort.errorString().toStdString() );
        return;
    }
}

void LoconetSerialConnection::writeData(QByteArray ba){
    if(m_serialPort.isOpen()){
        m_serialPort.write(ba);
    }
}

void LoconetSerialConnection::dataAvailable(){
    QByteArray ba = m_serialPort.readAll();

    for(uint8_t byte : ba){
        ln_incoming_byte(m_locoContext, byte);
    }
}
