/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LOCONETSERIALCONNECTION_H
#define LOCONETSERIALCONNECTION_H

#include <QObject>
#include <QSerialPort>

#include "loconetconnection.h"
#include "loconet_buffer.h"

class LoconetSerialConnection : public LoconetConnection
{
    Q_OBJECT
public:
    explicit LoconetSerialConnection(QObject *parent = nullptr);

    void setSerialPortName(QString port);

    void writeData(uint8_t* data, int len);

    bool open();

Q_SIGNALS:

private Q_SLOTS:
    void dataAvailable();

protected:
    void doSave(QSettings& settings);

private:
    QSerialPort m_serialPort;
};

#endif // LOCONETSERIALCONNECTION_H
