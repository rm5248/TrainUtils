/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LOCONETPROGRAMMER_H
#define LOCONETPROGRAMMER_H

#include <QObject>
#include <QQueue>
#include <memory>

#include "loconet_buffer.h"
#include "loconetprogrammingresult.h"
#include "loconetprogrammingrequest.h"

class LoconetConnection;

class LoconetProgrammer : public QObject
{
    Q_OBJECT
public:
    explicit LoconetProgrammer(LoconetConnection* connection, QObject *parent = nullptr);

    /**
     * Submit a programming request.  Returns a result object
     * that allows you to get the result of the request.
     */
    std::shared_ptr<LoconetProgrammingResult> submitProgrammingRequest(const LoconetProgrammingRequest& request);

Q_SIGNALS:

private Q_SLOTS:
    void incomingMessage(loconet_message message);

private:
    void sendCurrentRequest();
    void completeCurrentRequest(LoconetProgrammingResult::CompletedReason reason, uint8_t data = 0);

    enum class State { Idle, WaitingForLack, WaitingForCompletion };

    struct PendingRequest {
        LoconetProgrammingRequest request;
        std::shared_ptr<LoconetProgrammingResult> result;
    };

    LoconetConnection* m_connection;
    State m_state{State::Idle};
    QQueue<PendingRequest> m_queue;
    PendingRequest m_current;
};

#endif // LOCONETPROGRAMMER_H
