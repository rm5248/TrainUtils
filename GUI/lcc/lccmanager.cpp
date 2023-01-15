/* SPDX-License-Identifier: GPL-2.0 */
#include "lccmanager.h"
#include "lccnetworkconnection.h"

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
    newConn->connectToRemote(addr, port);
    newConn->setName(connectionName);

    m_lccConnections[connectionName] = newConn;

    return newConn;
}

std::shared_ptr<LCCConnection> LCCManager::getConnectionByName(QString connectionName){
    if(m_lccConnections.find(connectionName) != m_lccConnections.end()){
        return std::shared_ptr<LCCConnection>();
    }

    return m_lccConnections[connectionName];
}
