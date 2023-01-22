/* SPDX-License-Identifier: GPL-2.0 */
#include "systemconnection.h"

SystemConnection::SystemConnection(QObject *parent) : QObject(parent){

}

SystemConnection::~SystemConnection(){

}

void SystemConnection::setName(QString name){
    if(name != m_name){
        m_name = name;
        Q_EMIT systemNameChanged();
    }
}

QString SystemConnection::name() const{
    return m_name;
}

bool SystemConnection::isConnected() const{
    return m_isConnected;
}

void SystemConnection::connectedToSystem(){
    if(m_isConnected){
        return;
    }

    m_isConnected = true;
    Q_EMIT isConnectedChanged();
}

void SystemConnection::disconnectedFromSystem(){
    if(!m_isConnected){
        return;
    }

    m_isConnected = false;
    Q_EMIT isConnectedChanged();
}
