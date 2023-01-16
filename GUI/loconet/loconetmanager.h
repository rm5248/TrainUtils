/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LOCONETMANAGER_H
#define LOCONETMANAGER_H

#include <QObject>
#include <QHostAddress>
#include <memory>

class LoconetConnection;

class LoconetManager : public QObject
{
    Q_OBJECT
public:
    explicit LoconetManager(QObject *parent = nullptr);

    /**
     * Create a new Loconet connection over the network to the specified host.
     * If a connection with this name already exists, return an invalid shared pointer.
     *
     * @param connectionName Optional connection name.  If no name is provided, auto-number(Loconet1, Loconet2, etc.)
     * @param addr
     * @param port
     * @return
     */
    std::shared_ptr<LoconetConnection> createNewNetworkLoconet(QString connectionName, QHostAddress addr, uint16_t port);

    /**
     * Create a new local Loconet connection connected to a serial port on our computer.
     *
     * @param connectionName
     * @param serialPort
     * @return
     */
    std::shared_ptr<LoconetConnection> createNewLocalLoconet(QString connectionName, QString serialPort);

    /**
     * Get an LCC connection using the previously-specified name.  If the connection
     * name already exists, return an invalid shared pointer.
     *
     * @param connectionName
     * @return
     */
    std::shared_ptr<LoconetConnection> getConnectionByName(QString connectionName);


Q_SIGNALS:

private:
    QMap<QString,std::shared_ptr<LoconetConnection>> m_loconetConnections;
    int m_nextConnNumber;
};

#endif // LOCONETMANAGER_H
