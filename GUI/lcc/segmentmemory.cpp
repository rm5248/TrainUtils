/* SPDX-License-Identifier: GPL-2.0 */
#include "segmentmemory.h"
#include "addressspacereadreply.h"
#include "addressspacereply.h"
#include "lccconnection.h"
#include "lcc-memory.h"

#include <log4cxx/logger.h>
#include <fmt/format.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui.lcc.SegmentMemory" );

SegmentMemory::SegmentMemory(LCCConnection* conn, uint16_t alias, uint8_t segment_id, QObject *parent)
    : QObject{parent},
    m_conn(conn),
    m_alias(alias),
    m_segment_id(segment_id),
    m_starting_address(0),
    m_ending_address(0)
{
}

uint8_t SegmentMemory::segmentId(){
    return m_segment_id;
}

uint32_t SegmentMemory::startingAddress(){
    return m_starting_address;
}

uint32_t SegmentMemory::highAddress(){
    return m_ending_address;
}

QVector<uint8_t> SegmentMemory::data(){
    return m_data;
}

bool SegmentMemory::hasMemory(){
    return m_hasData;
}

void SegmentMemory::readAllMemory(){
    if(m_invalid){
        return;
    }

    if(m_currentReadState == MemoryReadState::Read_Space_Info ||
        m_currentReadState == MemoryReadState::Reading_Memory){
        LOG4CXX_ERROR(logger, "Can't read memory: request already in process");
        return;
    }
    m_currentReadState = MemoryReadState::Read_Space_Info;
    m_currentOffset = 0;

    m_reply = m_conn->queryAddressSpaceInformation(m_alias, LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION);
    if(!m_reply){
        return;
    }

    connect(m_reply, &AddressSpaceReply::finished,
            this, &SegmentMemory::addressSpaceFinished);

    m_currentReadState = MemoryReadState::Read_Space_Info;
}

void SegmentMemory::addressSpaceFinished(){
    m_reply->deleteLater();

    LOG4CXX_DEBUG_FMT(logger, "Address space information: Space {:X} exists? {} low address {} high address {}",
                      m_reply->space(),
                      m_reply->exists(),
                      m_reply->lowAddress(),
                      m_reply->highAddress());


    if(!m_reply->exists() && m_reply->highAddress() != 0){
        LOG4CXX_WARN(logger, "Memory segment does not exist but high address is set: assuming it actually does exist");
    }

    if(m_reply->highAddress()< m_reply->lowAddress()){
        LOG4CXX_ERROR(logger, "Ending address less than starting address: invalid");
        m_invalid = true;
        return;
    }

    if(m_currentReadState == MemoryReadState::Read_Space_Info && m_reply->space() == m_segment_id){
        m_starting_address = m_reply->lowAddress();
        m_ending_address = m_reply->highAddress();
        m_currentOffset = 0;
        m_currentReadState = MemoryReadState::Reading_Memory;

        uint32_t lenToRead = (m_ending_address - m_starting_address) - m_currentOffset;
        if(lenToRead > 64){
            lenToRead = 64;
        }

        // Now let's trigger a read of the entire segment
        m_readReply = m_conn->readSingleMemoryBlock(m_alias, m_segment_id, m_currentOffset, lenToRead);
        if(m_readReply){
            connect(m_readReply, &AddressSpaceReadReply::finished,
                    this, &SegmentMemory::addressSpaceRead);
        }
    }

    m_reply = nullptr;
}

void SegmentMemory::addressSpaceRead(){
    LOG4CXX_DEBUG_FMT(logger, "Got address space read");

    m_readReply->deleteLater();

    if(m_currentReadState == MemoryReadState::Reading_Memory && m_readReply->space() == m_segment_id){
        m_currentOffset += m_readReply->data().length();
        m_data.append(m_readReply->data());

        uint32_t lenToRead = (m_ending_address - m_starting_address) - m_currentOffset;
        if(lenToRead > 64){
            lenToRead = 64;
        }

        if(lenToRead > 0){
            // Read the next block of the memory segment
            m_readReply = m_conn->readSingleMemoryBlock(m_alias, m_segment_id, m_currentOffset, lenToRead);
            if(m_readReply){
                connect(m_readReply, &AddressSpaceReadReply::finished,
                        this, &SegmentMemory::addressSpaceRead);
            }
        }else{
            m_currentReadState = MemoryReadState::Memory_Complete;
            m_hasData = true;
            Q_EMIT memoryReady();
        }
    }
}
