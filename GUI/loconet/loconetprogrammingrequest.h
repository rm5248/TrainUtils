/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LOCONETPROGRAMMINGREQUEST_H
#define LOCONETPROGRAMMINGREQUEST_H

#include <cstdint>

#include "loconet_buffer.h"

/**
 * Builder for a LocoNet programmer task request (OPC 0xEF, slot 0x7C).
 */
class LoconetProgrammingRequest {
public:
    enum class Mode {
        ServiceTrack,        // Direct byte on the service/programming track
        OpsModeNoFeedback,   // Ops-mode (mainline) programming, no ACK feedback
        OpsModeWithFeedback, // Ops-mode (mainline) programming with ACK feedback
    };

    LoconetProgrammingRequest();

    LoconetProgrammingRequest& setMode(Mode mode);
    LoconetProgrammingRequest& setWrite(bool isWrite);
    LoconetProgrammingRequest& setCv(int cv);          // 1-based DCC CV number
    LoconetProgrammingRequest& setData(uint8_t data);
    LoconetProgrammingRequest& setLocoAddress(int address); // Ops-mode only

    loconet_message buildMessage() const;

    bool isWrite() const { return m_isWrite; }
    bool isServiceMode() const { return m_mode == Mode::ServiceTrack; }

private:
    Mode    m_mode;
    bool    m_isWrite;
    int     m_cv;
    uint8_t m_data;
    int     m_locoAddress;
};

#endif // LOCONETPROGRAMMINGREQUEST_H
