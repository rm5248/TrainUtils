/* SPDX-License-Identifier: GPL-2.0 */
#include "loconetprogrammingresult.h"

LoconetProgrammingResult::LoconetProgrammingResult(QObject *parent)
    : QObject{parent}
{}

LoconetProgrammingResult::CompletedReason LoconetProgrammingResult::completedReason() const {
    return m_reason;
}

bool LoconetProgrammingResult::completed() const {
    return m_completed;
}

uint8_t LoconetProgrammingResult::result() const {
    return m_data;
}

void LoconetProgrammingResult::complete(CompletedReason reason, uint8_t data) {
    m_completed = true;
    m_reason = reason;
    m_data = data;
    Q_EMIT programmingResultCompleted();
}
