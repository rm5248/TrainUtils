/* SPDX-License-Identifier: GPL-2.0 */
#include <QStandardPaths>
#include <QTimer>

#include "systemconnection.h"
#include "loconet/loconetconnection.h"
#include "loconet/loconetmanager.h"
#include "trainutils_state.h"

#include <log4cxx/logger.h>
#include <fmt/format.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui.SystemConnection" );

SystemConnection::SystemConnection(QObject *parent) : QObject(parent){
    m_uuid = QUuid::createUuid();
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

QString SystemConnection::errorString() const{
    return m_error;
}

QUuid SystemConnection::uuid(){
    return m_uuid;
}

void SystemConnection::save(){
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString fileName = QString("%1/%2.ini").arg(configDir).arg(m_name);
    QSettings settingsFile(fileName, QSettings::Format::IniFormat);

    settingsFile.beginGroup("connection");
    settingsFile.setValue("name", m_name);
    settingsFile.setValue("uuid", m_uuid);
    settingsFile.setValue("type", connectionType());
    settingsFile.endGroup();

    doSave(settingsFile);
}

std::shared_ptr<SystemConnection> SystemConnection::createfromINI(QString fileLocation, TrainUtilsState* state){
    LOG4CXX_DEBUG_FMT(logger, "Create system connection from file name {}", fileLocation.toStdString());

    QSettings settings(fileLocation, QSettings::IniFormat);
    std::shared_ptr<SystemConnection> retval;
    QString connectionType = settings.value("connection/type").toString();

    if(connectionType == "loconet"){
        retval = state->loconetManager->createFromSettings(settings);
    }

    if(retval){
        QTimer::singleShot(50, [retval](){
            retval->open();
        });
    }

    if(!retval){
        LOG4CXX_ERROR_FMT(logger, "Unable to create new system connection!");
    }

    return retval;
}

void SystemConnection::load(QSettings &settings){
    m_name = settings.value("connection/name").toString();
    m_uuid = settings.value("connection/uuid").toUuid();
}
