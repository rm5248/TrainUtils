/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCCQIOCONNECTION_H
#define LCCQIOCONNECTION_H

#include <QObject>
#include <QIODevice>

#include "lccconnection.h"
#include "lcc-gridconnect.h"

/**
 * Represents an LCC connection that uses a QIODevice as the connection
 */
class LCCQIoConnection : public LCCConnection
{
    Q_OBJECT
public:
    explicit LCCQIoConnection(QObject *parent = nullptr);

    ~LCCQIoConnection();

Q_SIGNALS:

private Q_SLOTS:
    void incomingData();
    void generateAliasDone();

protected:
    void updateQIODeviceConnections();
    void generateAlias();

private:
    static void writeLCCFrameCB(lcc_context* context, lcc_can_frame* frame);
    void writeLCCFrame(lcc_can_frame* frame);
    static void gridconnectLCCFrameParsedCB(lcc_gridconnect* context, lcc_can_frame* frame);
    void gridconnectLCCFrameParsed(lcc_can_frame* frame);

protected:
    lcc_gridconnect* m_lccGrid;
    QIODevice* m_ioDevice;
};

#endif // LCCQIOCONNECTION_H
