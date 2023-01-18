/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LOCONETCONNECTION_H
#define LOCONETCONNECTION_H

#include <QObject>
#include <QTimer>
#include <QQueue>

#include "loconet_buffer.h"

class LoconetConnection : public QObject
{
    Q_OBJECT
public:
    explicit LoconetConnection(QObject *parent = nullptr);
    ~LoconetConnection();

    void setName(QString name);
    QString name() const;

    void sendMessage(loconet_message msg);

Q_SIGNALS:
    void incomingRawPacket(QByteArray);
    void incomingLoconetMessage(loconet_message message);

private Q_SLOTS:
    void sendNextMessage();

protected:
    virtual void writeData(uint8_t* data, int len) = 0;

private:
    static void writeCB( struct loconet_context* ctx, uint8_t* data, int len );

protected:
    QString m_name;
    struct loconet_context* m_locoContext;

private:
    QQueue<loconet_message> m_sendQueue;
    QTimer m_sendTimer;
};

#endif // LOCONETCONNECTION_H
