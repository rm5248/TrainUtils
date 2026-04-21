/* SPDX-License-Identifier: GPL-2.0 */
#include "loconetprogrammer.h"
#include "loconetconnection.h"

#include <log4cxx/logger.h>
#include <fmt/format.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("traingui.loconet.LoconetProgrammer");

LoconetProgrammer::LoconetProgrammer(LoconetConnection* connection, QObject *parent)
    : QObject{parent}
    , m_connection(connection)
{
    connect(connection, &LoconetConnection::incomingLoconetMessage,
            this, &LoconetProgrammer::incomingMessage);
}

std::shared_ptr<LoconetProgrammingResult> LoconetProgrammer::submitProgrammingRequest(const LoconetProgrammingRequest& request) {
    auto result = std::make_shared<LoconetProgrammingResult>();

    PendingRequest pending;
    pending.request = request;
    pending.result = result;

    m_queue.enqueue(pending);
    LOG4CXX_DEBUG_FMT(logger, "Programming request queued (queue depth: {})", m_queue.size());

    if (m_state == State::Idle){
        sendCurrentRequest();
    }

    return result;
}

void LoconetProgrammer::sendCurrentRequest() {
    if (m_queue.isEmpty()) {
        m_state = State::Idle;
        return;
    }

    m_current = m_queue.dequeue();
    m_state = State::WaitingForLack;

    loconet_message msg = m_current.request.buildMessage();
    LOG4CXX_DEBUG_FMT(logger, "Sending programming request to programmer slot");
    m_connection->sendMessage(msg);
}

void LoconetProgrammer::completeCurrentRequest(LoconetProgrammingResult::CompletedReason reason, uint8_t data) {
    m_current.result->complete(reason, data);
    sendCurrentRequest();
}

void LoconetProgrammer::incomingMessage(loconet_message message) {
    if (m_state == State::WaitingForLack) {
        // Expecting LACK: opcode=0xB4, lopc=0x7F (= 0xEF & 0x7F)
        if (message.opcode != LN_OPC_LONG_ACK || message.ack.lopc != 0x7F)
            return;

        switch (message.ack.ack) {
        case 0x01:
            // Task accepted — wait for E7 completion reply
            LOG4CXX_DEBUG_FMT(logger, "Programming task accepted, waiting for completion");
            m_state = State::WaitingForCompletion;
            break;
        case 0x40:
            // Accepted blind — no E7 reply will follow
            LOG4CXX_DEBUG_FMT(logger, "Programming task accepted blind (no completion reply)");
            completeCurrentRequest(LoconetProgrammingResult::CompletedReason::Success);
            break;
        case 0x00:
            LOG4CXX_WARN_FMT(logger, "Programmer busy, request aborted");
            completeCurrentRequest(LoconetProgrammingResult::CompletedReason::ProgrammerBusy);
            break;
        default:
            // 0x7F = not implemented, or unknown
            LOG4CXX_WARN_FMT(logger, "Programming not implemented (LACK ack=0x{:X})", message.ack.ack);
            completeCurrentRequest(LoconetProgrammingResult::CompletedReason::NotImplemented);
            break;
        }
        return;
    }

    if (m_state == State::WaitingForCompletion) {
        // Expecting E7 reply on the programmer slot
        if (message.opcode != LN_OPC_SLOT_READ_DATA || message.slot_data.slot != 0x7C)
            return;

        uint8_t pstat = message.slot_data.stat;
        LOG4CXX_DEBUG_FMT(logger, "Programming task completed, PSTAT=0x{:X}", pstat);

        if (pstat & 0x01) {
            completeCurrentRequest(LoconetProgrammingResult::CompletedReason::NoDecoderDetected);
        } else if (pstat & 0x02) {
            completeCurrentRequest(LoconetProgrammingResult::CompletedReason::NoWriteAck);
        } else if (pstat & 0x04) {
            completeCurrentRequest(LoconetProgrammingResult::CompletedReason::FailedToDetectReadAck);
        } else if (pstat & 0x08) {
            completeCurrentRequest(LoconetProgrammingResult::CompletedReason::UserAborted);
        } else {
            // Success — reconstruct full 8-bit data from DATA7 and D7 in CVH
            uint8_t data7 = message.slot_data.sound;
            uint8_t d7    = (message.slot_data.stat2 >> 1) & 1;
            uint8_t data  = data7 | static_cast<uint8_t>(d7 << 7);
            completeCurrentRequest(LoconetProgrammingResult::CompletedReason::Success, data);
        }
    }
}
