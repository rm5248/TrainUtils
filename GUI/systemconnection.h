/* SPDX-License-Identifier: GPL-2.0 */
#ifndef SYSTEMCONNECTION_H
#define SYSTEMCONNECTION_H

#include <QObject>

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

    bool isConnected() const;

Q_SIGNALS:
    void systemNameChanged();
    void isConnectedChanged();

protected:
    void connectedToSystem();
    void disconnectedFromSystem();

private:
    QString m_name;
    bool m_isConnected = false;
};

#endif // SYSTEMCONNECTION_H
