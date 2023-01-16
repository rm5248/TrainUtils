/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LOCONETCONNECTION_H
#define LOCONETCONNECTION_H

#include <QObject>
#include <QTimer>

#include "loconet_buffer.h"

class LoconetConnection : public QObject
{
    Q_OBJECT
public:
    explicit LoconetConnection(QObject *parent = nullptr);
    ~LoconetConnection();

    void setName(QString name);
    QString name() const;

Q_SIGNALS:
    void incomingRawPacket(QByteArray);
    void incomingLoconetMessage(loconet_message message);

protected:
    virtual void writeData(QByteArray ba) = 0;

private:
    static void writeCB( struct loconet_context* ctx, uint8_t* data, int len );

protected:
    QString m_name;
    struct loconet_context* m_locoContext;
};

#endif // LOCONETCONNECTION_H
