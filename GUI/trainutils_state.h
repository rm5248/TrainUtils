/* SPDX-License-Identifier: GPL-2.0 */
#ifndef TRAINUTILS_STATE_H
#define TRAINUTILS_STATE_H

#include <memory>
#include <QVector>
#include <QString>

#include "systemconnection.h"

namespace DBus::Qt{
class QtDispatcher;
}

class LCCManager;
class MDNSManager;
class LoconetManager;

struct ConnectionInfo{
    QString connectionName;
    QString connectionFileAbsolutePath;
};

struct TrainUtilsState{
    LCCManager* lccManager;
    MDNSManager* mdnsManager;
    LoconetManager* loconetManager;
    QVector<std::shared_ptr<SystemConnection>> m_connections;
    QVector<ConnectionInfo> connectionFiles;
};

namespace TrainUtils{

std::shared_ptr<SystemConnection> connectionByName(TrainUtilsState*, QString name);

template<class T>
std::shared_ptr<T> connectionByNameAndType(TrainUtilsState* state, QString name){
    for(std::shared_ptr<SystemConnection> conn : state->m_connections){
        if(conn->name() == name){
            return qSharedPointerObjectCast<T>(conn);
        }
    }

    return nullptr;
}

}

#endif // TRAINUTILS_STATE_H
