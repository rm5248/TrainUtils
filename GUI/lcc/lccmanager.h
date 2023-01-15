/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCCMANAGER_H
#define LCCMANAGER_H

#include <QObject>
#include <QHostAddress>
#include <memory>

#include "lccconnection.h"

class LCCManager : public QObject
{
    Q_OBJECT
public:
    explicit LCCManager(QObject *parent = nullptr);

    /**
     * Create a new LCC connection over the network to the specified host.
     * If a connection with this name already exists, return an invalid shared pointer.
     *
     * @param connectionName Optional connection name.  If no name is provided, auto-number(LCC1, LCC2, etc.)
     * @param addr
     * @param port
     * @return
     */
    std::shared_ptr<LCCConnection> createNewNetworkLCC(QString connectionName, QHostAddress addr, uint16_t port);

    /**
     * Get an LCC connection using the previously-specified name.  If the connection
     * name already exists, return an invalid shared pointer.
     *
     * @param connectionName
     * @return
     */
    std::shared_ptr<LCCConnection> getConnectionByName(QString connectionName);

Q_SIGNALS:

private:
    QMap<QString,std::shared_ptr<LCCConnection>> m_lccConnections;
    int m_nextConnNumber;
};

#endif // LCCMANAGER_H
