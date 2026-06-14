/* SPDX-License-Identifier: GPL-2.0 */
#ifndef SPEEDOCONNECTION_H
#define SPEEDOCONNECTION_H

#include <QObject>
#include <QSerialPort>

#include "../systemconnection.h"

class SpeedoConnection : public SystemConnection
{
    Q_OBJECT
public:
    explicit SpeedoConnection(QObject *parent = nullptr);

    std::shared_ptr<Turnout> getDCCTurnout(int switch_num) override;
    void load(QSettings& settings) override;
    bool open() override;
    void setSerialPortName(QString name);

    static QStringList getAvailableConnections();

protected:
    void doSave(QSettings& settings) override;
    QString connectionType() override;

Q_SIGNALS:
    /**
     * Update the speed.  Kilometers per hour
     *
     * @param speed
     */
    void speedUpdated(int speed);

private Q_SLOTS:
    void incomingData();

private:
    QString m_serialPortName;
    QSerialPort m_port;
};

#endif // SPEEDOCONNECTION_H
