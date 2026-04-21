/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LOCONETPROGRAMMINGRESULT_H
#define LOCONETPROGRAMMINGRESULT_H

#include <QObject>

/**
 * Represents the result of a programming operation on Loconet.
 * The programmingResultCompleted() signal fires when the task finishes.
 */
class LoconetProgrammingResult : public QObject
{
    Q_OBJECT
public:
    enum class CompletedReason {
        NotYetCompleted,
        Success,
        UserAborted,
        FailedToDetectReadAck,
        NoWriteAck,
        NoDecoderDetected,
        NotImplemented,
        ProgrammerBusy,
    };

    explicit LoconetProgrammingResult(QObject *parent = nullptr);

    CompletedReason completedReason() const;
    bool completed() const;

    /**
     * If this was a read request and completed successfully, the byte read back.
     */
    uint8_t result() const;

Q_SIGNALS:
    void programmingResultCompleted();

private:
    void complete(CompletedReason reason, uint8_t data = 0);

    bool m_completed{false};
    CompletedReason m_reason{CompletedReason::NotYetCompleted};
    uint8_t m_data{0};

    friend class LoconetProgrammer;
};

#endif // LOCONETPROGRAMMINGRESULT_H
