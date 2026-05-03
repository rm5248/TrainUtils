/* SPDX-License-Identifier: GPL-2.0 */
#include "loconetprogrammingrequest.h"

LoconetProgrammingRequest::LoconetProgrammingRequest()
    : m_mode(Mode::ServiceTrack)
    , m_isWrite(false)
    , m_cv(1)
    , m_data(0)
    , m_locoAddress(0)
{}

LoconetProgrammingRequest& LoconetProgrammingRequest::setMode(Mode mode) {
    m_mode = mode;
    return *this;
}

LoconetProgrammingRequest& LoconetProgrammingRequest::setWrite(bool isWrite) {
    m_isWrite = isWrite;
    return *this;
}

LoconetProgrammingRequest& LoconetProgrammingRequest::setCv(int cv) {
    m_cv = cv;
    return *this;
}

LoconetProgrammingRequest& LoconetProgrammingRequest::setData(uint8_t data) {
    m_data = data;
    return *this;
}

LoconetProgrammingRequest& LoconetProgrammingRequest::setLocoAddress(int address) {
    m_locoAddress = address;
    return *this;
}

loconet_message LoconetProgrammingRequest::buildMessage() const {
    loconet_message msg{};
    msg.opcode = LN_OPC_SLOT_WRITE_DATA; // 0xEF

    // PCMD: D5=1 (byte mode always), D6=write, mode bits
    uint8_t pcmd = 0x20; // D5 byte mode
    if (m_isWrite)
        pcmd |= 0x40; // D6 write
    switch (m_mode) {
    case Mode::ServiceTrack:
        pcmd |= 0x08; // TY0=1, OpsMode=0
        break;
    case Mode::OpsModeNoFeedback:
        pcmd |= 0x04; // OpsMode=1, TY1=0, TY0=0
        break;
    case Mode::OpsModeWithFeedback:
        pcmd |= 0x0C; // OpsMode=1, TY0=1
        break;
    }

    // JMRI sets the lower two reserved bits for some reason?
    // pcmd |= 0x03;

    // CV is 1-based in DCC; protocol uses 0-based 10-bit index
    int cvIndex = m_cv - 1;
    uint8_t cvl = cvIndex & 0x7F;
    uint8_t cv7 = (cvIndex >> 7) & 1;
    uint8_t cv8 = (cvIndex >> 8) & 1;
    uint8_t cv9 = (cvIndex >> 9) & 1;
    uint8_t d7  = (m_data >> 7) & 1;
    uint8_t data7 = m_data & 0x7F;
    // CVH: <0,0,CV9,CV8,0,0,D7,CV7>
    uint8_t cvh = (cv9 << 5) | (cv8 << 4) | (d7 << 1) | cv7;

    // Ops-mode loco address split into 7-bit halves
    uint8_t hopsa = (m_locoAddress >> 7) & 0x7F;
    uint8_t lopsa = m_locoAddress & 0x7F;

    // Populate slot_data fields (opcode already set above)
    msg.slot_data.len      = 0x0E; // 14-byte message
    msg.slot_data.slot     = 0x7C; // programmer slot
    msg.slot_data.stat     = pcmd;
    msg.slot_data.addr1    = 0;
    msg.slot_data.speed    = hopsa; // HOPSA
    msg.slot_data.dir_funcs = lopsa; // LOPSA
    msg.slot_data.track    = 0;    // TRK: let command station use current state
    msg.slot_data.stat2    = cvh;  // CVH
    msg.slot_data.addr2    = cvl;  // CVL
    msg.slot_data.sound    = data7; // DATA7
    msg.slot_data.id1      = 0;
    msg.slot_data.id2      = 0;

    // Checksum: XOR of all bytes must equal 0xFF
    // bytes: opcode, data[0..11], then CHK at data[12]
    uint8_t chk = 0xFF;
    chk ^= msg.opcode;
    for (int i = 0; i < 12; i++)
        chk ^= msg.data[i];
    msg.data[12] = chk;

    return msg;
}
