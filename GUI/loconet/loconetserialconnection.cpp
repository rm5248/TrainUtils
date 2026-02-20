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

bool LoconetSerialConnection::open(){
    if(!m_serialPort.open(QIODevice::ReadWrite)){
        m_error = m_serialPort.errorString();
        LOG4CXX_ERROR_FMT(logger, "Can't open {}: {}",
                          m_serialPort.portName().toStdString(),
                          m_serialPort.errorString().toStdString() );

        return false;
    }

    connectedToSystem();
    return true;
}

void LoconetSerialConnection::doSave(QSettings &settings){
    QSerialPortInfo inf(m_serialPort);

    settings.beginGroup("loconet");
    settings.setValue("type", "serial");
    settings.setValue("port", m_serialPort.portName());
    if(inf.hasProductIdentifier()){
        settings.setValue("pid", inf.productIdentifier());
    }
    if(inf.hasVendorIdentifier()){
        settings.setValue("vid", inf.vendorIdentifier());
    }
    if(inf.serialNumber().size() > 1){
        settings.setValue("serial_number", inf.serialNumber());
    }
    if(inf.description().size() > 1){
        settings.setValue("description", inf.description());
    }
    settings.endGroup();
}
