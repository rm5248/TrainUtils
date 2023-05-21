/* SPDX-License-Identifier: GPL-2.0 */
#include "lccnode.h"
#include "lcc-memory.h"
#include "lcc-node-info.h"
#include "lccconnection.h"

#include <log4cxx/logger.h>
#include <fmt/format.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui.lcc.LCCNode" );

LCCNode::LCCNode(lcc_context* lcc, lcc_node_info* inf, LCCConnection* conn, QObject *parent) :
    QObject(parent),
    m_lcc(lcc),
    m_nodeInfo(inf),
    m_hasCDI(false),
    m_cdiSize(-1)
{
    m_rawcdi.reserve(1024);
    connect(conn, &LCCConnection::incomingDatagram,
            this, &LCCNode::datagramRx);
}

bool LCCNode::valid() const{
    return m_lcc != nullptr && m_nodeInfo != nullptr;
}

bool LCCNode::hasCDI() const{
    return m_hasCDI;
}

void LCCNode::readCDI(){
    if(m_nodeInfo == nullptr){
        return;
    }
    uint16_t alias = lcc_node_info_get_alias(m_nodeInfo);

    lcc_memory_get_address_space_information(m_lcc, alias, LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION);
}

void LCCNode::datagramRx(QByteArray ba){
    uint8_t read_reply_b0 = ba[0];
    uint8_t status_byte = ba[1];
    uint32_t starting_address = (ba[2] << 24) |
                                (ba[3] << 16) |
                                (ba[4] << 8) |
                                (ba[5] << 0);

    if(read_reply_b0 != 0x20){
        // This is not a read reply for a datagram, it is something else.
        assert(false);
        return;
    }

    if(status_byte == 0x86 ||
            status_byte == 0x87){
        handleGetAddressSpaceInformationReply(ba);
    }else if(status_byte >= 0x50 &&
             status_byte <= 0x5B){
        handleDatagramRead(ba);
    }
}

void LCCNode::handleDatagramRead(QByteArray ba){
    uint8_t read_reply_status = ba[1];
    int space = 0;
    int starting_byte = 6;
    bool is_error = read_reply_status >= 0x58;

    if(read_reply_status == 0x50 ||
            read_reply_status == 0x58){
        // space in byte 6
        space = ba[6];
        starting_byte = 7;
    }else if(read_reply_status == 0x51 ||
             read_reply_status == 0x59){
        space = LCC_MEMORY_SPACE_CONFIGURATION_SPACE;
    }else if(read_reply_status == 0x52 ||
             read_reply_status == 0x5A){
        space = LCC_MEMORY_SPACE_ALL_MEMORY;
    }else if(read_reply_status == 0x53 ||
             read_reply_status == 0x5B){
        space = LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION;
    }

    if(space == LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION && !is_error){
        int totalNumBytes = ba.size() - starting_byte;
        for(int x = 0; x < totalNumBytes; x++){
            char c = ba[starting_byte + x];
            if(c == 0){
                continue;
            }
            m_rawcdi.append(c);
        }

        LOG4CXX_TRACE_FMT(logger, "total num bytes: {} size: {} starting byte: {}", totalNumBytes, ba.size(), starting_byte);
        if(totalNumBytes <= 64 && (m_cdiCurrentOffset < m_cdiSize)){
            m_cdiCurrentOffset += totalNumBytes;
            uint16_t alias = lcc_node_info_get_alias(m_nodeInfo);
            int bytesRemaining = m_cdiSize - m_cdiCurrentOffset;
            int bytesToTx = 64;
            if(bytesRemaining < 64){
                bytesToTx = bytesRemaining;
            }
            lcc_memory_read_single_transfer(m_lcc, alias, LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION, m_cdiCurrentOffset, bytesToTx);
        }else{
            m_hasCDI = true;
            QXmlStreamReader reader;
            reader.addData(m_rawcdi);
            m_cdi = CDI::createFromXML(&reader);
            Q_EMIT cdiRead();
        }
    }else if(space == LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION && is_error){
        LOG4CXX_ERROR_FMT(logger, "Unable to read data correctly");
    }
}

void LCCNode::handleGetAddressSpaceInformationReply(QByteArray ba){
    int stat = ba[1];
    uint8_t addressSpace = ba[2];
    uint32_t highestAddress =
            (ba[3] << 24) |
            (ba[4] << 16) |
            (ba[5] << 8) |
            (ba[6] << 0);
    int flags = ba[7];
    uint32_t lowestAddress = 0;

    if(stat == 0x86){
        LOG4CXX_DEBUG_FMT(logger, "Space 0x{:X} not available", addressSpace);
        return;
    }

    if(flags & (0x01 << 1)){
        lowestAddress =
                (ba[8] << 24) |
                (ba[9] << 16) |
                (ba[10] << 8) |
                (ba[11] << 0);
    }

    LOG4CXX_TRACE_FMT(logger, "Space: 0x{:X} highest: 0x{:X} lowest: 0x{:X} flags: 0x{:X}",
                      addressSpace,
                      highestAddress,
                      lowestAddress,
                      flags);

    if(addressSpace == LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION){
        uint16_t alias = lcc_node_info_get_alias(m_nodeInfo);
        m_cdiSize = highestAddress;

        m_cdiCurrentOffset = 0;
        lcc_memory_read_single_transfer(m_lcc, alias, LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION, 0, 64);
    }
}

QString LCCNode::rawCDI() const{
    return m_rawcdi;
}

CDI LCCNode::cdi() const{
    return m_cdi;
}
