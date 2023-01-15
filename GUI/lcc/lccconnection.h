/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCCCONNECTION_H
#define LCCCONNECTION_H

#include <QObject>

#include "lcc.h"

class LCCConnection : public QObject
{
    Q_OBJECT
public:
    explicit LCCConnection(QObject *parent = nullptr);

    ~LCCConnection();

    void setName(QString name);
    QString name() const;

Q_SIGNALS:

protected:
    QString m_name;
    struct lcc_context* m_lcc;
};

#endif // LCCCONNECTION_H
