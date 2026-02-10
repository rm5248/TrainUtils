/* SPDX-License-Identifier: GPL-2.0 */
#ifndef SYSTEMCONNECTION_H
#define SYSTEMCONNECTION_H

#include <QObject>

#include "common/turnout.h"

/**
 * Represents an abstract connection to a system of some kind.
 */
class SystemConnection : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY systemNameChanged)
    Q_PROPERTY(bool connected READ isConnected NOTIFY isConnectedChanged)

public:
    explicit SystemConnection(QObject *parent = nullptr);

    virtual ~SystemConnection();

    void setName(QString name);
    QString name() const;
    QString errorString() const;
    virtual bool open() = 0;

    bool isConnected() const;

    /**
     * Get a turnout associated with the given DCC switch number.
     *
     * @param switch_num The switch number, 1-2048.
     * @return
     */
    virtual std::shared_ptr<Turnout> getDCCTurnout(int switch_num) = 0;

Q_SIGNALS:
    void systemNameChanged();
    void isConnectedChanged();
    void unableToConnectToSystem();

protected:
    void connectedToSystem();
    void disconnectedFromSystem();

private:
    QString m_name;
    bool m_isConnected = false;

protected:
    QString m_error;
};

#endif // SYSTEMCONNECTION_H
