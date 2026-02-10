/* SPDX-License-Identifier: GPL-2.0 */
#include <QFile>
#include "lccmanager.h"
#include "lccnetworkconnection.h"
#include "lccserialconnection.h"

static void setHardcodedInformation(std::shared_ptr<LCCConnection> conn){
    conn->setSimpleNodeInformation("TrainUtils", "TrainUtilsGUI", "0", "0");

    QFile f(":/sample-cdi.xml");
    if(f.open(QIODevice::ReadOnly)){
        QString cdiStr = f.readAll();
        conn->setCDI(cdiStr);
    }
}

LCCManager::LCCManager(QObject *parent) : QObject(parent)
{
    m_nextConnNumber = 1;
}

std::shared_ptr<LCCConnection> LCCManager::createNewNetworkLCC(QString connectionName, QHostAddress addr, uint16_t port){
    if(connectionName.isNull() || connectionName.isEmpty()){
        connectionName = QString("LCC%1").arg(m_nextConnNumber);
        m_nextConnNumber++;
    }
    if(m_lccConnections.find(connectionName) != m_lccConnections.end()){
        return std::shared_ptr<LCCConnection>();
    }

    std::shared_ptr<LCCNetworkConnection> newConn = std::make_shared<LCCNetworkConnection>();
    newConn->setRemote(addr, port);
    newConn->setName(connectionName);
    newConn->open();
    setHardcodedInformation(newConn);

    m_lccConnections[connectionName] = newConn;

    return newConn;
}

std::shared_ptr<LCCConnection> LCCManager::createNewLocalLCC(QString connectionName, QString serialPort){
    if(connectionName.isNull() || connectionName.isEmpty()){
        connectionName = QString("LCC%1").arg(m_nextConnNumber);
        m_nextConnNumber++;
    }
    if(m_lccConnections.find(connectionName) != m_lccConnections.end()){
        return std::shared_ptr<LCCConnection>();
    }

    std::shared_ptr<LCCSerialConnection> newConn = std::make_shared<LCCSerialConnection>();
    newConn->connectToSerialPort(serialPort);
    newConn->setName(connectionName);
    setHardcodedInformation(newConn);

    m_lccConnections[connectionName] = newConn;

    return newConn;
}

std::shared_ptr<LCCConnection> LCCManager::getConnectionByName(QString connectionName){
    if(m_lccConnections.find(connectionName) == m_lccConnections.end()){
        return std::shared_ptr<LCCConnection>();
    }

    return m_lccConnections[connectionName];
}
