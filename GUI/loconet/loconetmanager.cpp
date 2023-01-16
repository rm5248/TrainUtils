/* SPDX-License-Identifier: GPL-2.0 */
#include "loconetmanager.h"
#include "loconetnetworkconnection.h"

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

}

std::shared_ptr<LoconetConnection> LoconetManager::getConnectionByName(QString connectionName){
    if(m_loconetConnections.find(connectionName) == m_loconetConnections.end()){
        return std::shared_ptr<LoconetConnection>();
    }

    return m_loconetConnections[connectionName];
}
