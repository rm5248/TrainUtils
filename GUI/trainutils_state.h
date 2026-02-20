/* SPDX-License-Identifier: GPL-2.0 */
#ifndef TRAINUTILS_STATE_H
#define TRAINUTILS_STATE_H

#include <memory>
#include <QVector>
#include <QString>

class SystemConnection;

namespace DBus::Qt{
class QtDispatcher;
}

class LCCManager;
class MDNSManager;
class LoconetManager;

struct TrainUtilsState{
    LCCManager* lccManager;
    MDNSManager* mdnsManager;
    LoconetManager* loconetManager;
    QVector<std::shared_ptr<SystemConnection>> m_connections;
    QVector<QString> connectionINIFileNames;
};

#endif // TRAINUTILS_STATE_H
