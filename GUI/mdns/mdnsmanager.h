/* SPDX-License-Identifier: GPL-2.0 */
#ifndef MDNSMANAGER_H
#define MDNSMANAGER_H

#include <QObject>
#include <QHostAddress>


class MDNSManager : public QObject
{
    Q_OBJECT
protected:
    explicit MDNSManager(QObject *parent = nullptr);

public:
    ~MDNSManager();

Q_SIGNALS:
    void lccServerFound(QString serviceName, QHostAddress address, uint16_t port);
    void lccServerLeft(QString serviceName);
    void loconetServerFound(QString serviceName, QHostAddress address, uint16_t port);
    void loconetServerLeft(QString serviceName);

};

#endif // MDNSMANAGER_H
