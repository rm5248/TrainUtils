/* SPDX-License-Identifier: GPL-2.0 */
#include "loconetmanager.h"
#include "loconetnetworkconnection.h"
#include "loconetserialconnection.h"

#include <QSerialPort>
#include <QSerialPortInfo>

#include <log4cxx/logger.h>
#include <fmt/format.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui.loconet.LoconetManager" );

LoconetManager::LoconetManager(QObject *parent) : QObject(parent)
{
    m_nextConnNumber = 1;
}

std::shared_ptr<LoconetConnection> LoconetManager::createNewNetworkLoconet(QString connectionName, QHostAddress addr, uint16_t port){
    if(connectionName.isNull() || connectionName.isEmpty()){
        connectionName = QString("Loconet%1").arg(m_nextConnNumber);
        m_nextConnNumber++;
    }
    if(m_loconetConnections.find(connectionName) != m_loconetConnections.end()){
        return std::shared_ptr<LoconetConnection>();
    }

    std::shared_ptr<LoconetNetworkConnection> newConn = std::make_shared<LoconetNetworkConnection>();
    newConn->connectToRemote(addr, port);
    newConn->setName(connectionName);

    m_loconetConnections[connectionName] = newConn;

    return newConn;
}

std::shared_ptr<LoconetConnection> LoconetManager::createNewLocalLoconet(QString connectionName, QString serialPort){
    if(connectionName.isNull() || connectionName.isEmpty()){
        connectionName = QString("Loconet%1").arg(m_nextConnNumber);
        m_nextConnNumber++;
    }
    if(m_loconetConnections.find(connectionName) != m_loconetConnections.end()){
        return std::shared_ptr<LoconetConnection>();
    }

    std::shared_ptr<LoconetSerialConnection> conn = std::make_shared<LoconetSerialConnection>();

    conn->setName(connectionName);
    conn->setSerialPortName(serialPort);

    m_loconetConnections[connectionName] = conn;

    return conn;
}

std::shared_ptr<LoconetConnection> LoconetManager::getConnectionByName(QString connectionName){
    if(m_loconetConnections.find(connectionName) == m_loconetConnections.end()){
        return std::shared_ptr<LoconetConnection>();
    }

    return m_loconetConnections[connectionName];
}

QStringList LoconetManager::getAvailableLocalSerialPortConnections(){
    QStringList ret;

    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();

    for(QSerialPortInfo& inf : ports){
        LOG4CXX_DEBUG_FMT(logger, "Port name: {} Manufacturer: {} Description: {}",
                          inf.portName().toStdString(),
                          inf.manufacturer().toStdString(),
                          inf.description().toStdString());

        if(inf.portName().startsWith( "loconet_")){
            ret.push_back( inf.portName() );
            continue;
        }

        if(inf.manufacturer().compare("Digitrax Inc.") == 0){
            ret.push_back(inf.portName());
        }
    }

    return ret;
}
