/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCCCONNECTION_H
#define LCCCONNECTION_H

#include <QObject>

#include "lcc.h"
#include "../systemconnection.h"

class LCCConnection : public SystemConnection
{
    Q_OBJECT
public:
    explicit LCCConnection(QObject *parent = nullptr);

    ~LCCConnection();

    void setSimpleNodeInformation(QString manufacturer,
                                  QString model,
                                  QString hwVersion,
                                  QString swVersion);

    void setSimpleNodeNameDescription(QString nodeName,
                                      QString nodeDescription);

    void sendEvent(uint64_t event_id);

Q_SIGNALS:
    void incomingRawFrame(lcc_can_frame* frame);
    void incomingEvent(uint64_t event_id);

protected:
    struct lcc_context* m_lcc;
};

#endif // LCCCONNECTION_H
