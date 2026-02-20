/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCCSERIALCONNECTION_H
#define LCCSERIALCONNECTION_H

#include <QObject>
#include <QSerialPort>

#include "lccconnection.h"
#include "lcc-gridconnect.h"
#include "lccqioconnection.h"

class LCCSerialConnection : public LCCQIoConnection
{
    Q_OBJECT
public:
    explicit LCCSerialConnection(QObject *parent = nullptr);

    ~LCCSerialConnection();

    void connectToSerialPort(QString serialPort);
    bool open();

protected:
    void doSave(QSettings& settings);

};

#endif // LCCSERIALCONNECTION_H
