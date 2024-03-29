/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LOCONETCONNECTION_H
#define LOCONETCONNECTION_H

#include <QObject>
#include <QTimer>
#include <QQueue>
#include <memory>

#include "loconet_buffer.h"
#include "loconet_turnout.h"
#include "../systemconnection.h"

class LoconetThrottle;

class LoconetConnection : public SystemConnection
{
    Q_OBJECT
public:
    explicit LoconetConnection(QObject *parent = nullptr);
    ~LoconetConnection();

    struct loconet_context* loconetContext();

    /**
     * A generic 'send message' method.
     * @param msg
     */
    void sendMessage(loconet_message msg);

    void throwTurnout(int switch_num);
    void closeTurnout(int switch_num);

    /**
     * Create a new throttle attached to this connection.
     *
     * @return
     */
    std::shared_ptr<LoconetThrottle> newThrottle();

Q_SIGNALS:
    void incomingRawPacket(QByteArray);
    void incomingLoconetMessage(loconet_message message);

private Q_SLOTS:
    void sendNextMessage();

protected:
    virtual void writeData(uint8_t* data, int len) = 0;

private:
    static void writeCB( struct loconet_context* ctx, uint8_t* data, int len );
    static void incomingLoconetCB(struct loconet_context* ctx, struct loconet_message* msg);
    void incomingLoconet(struct loconet_message* msg);

protected:
    struct loconet_context* m_locoContext;
    struct loconet_turnout_manager* m_switchManager;

private:
    QQueue<loconet_message> m_sendQueue;
    QTimer m_sendTimer;
    QVector<std::shared_ptr<LoconetThrottle>> m_throttles;
};

#endif // LOCONETCONNECTION_H
