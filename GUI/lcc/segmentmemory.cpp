/* SPDX-License-Identifier: GPL-2.0 */
#include "segmentmemory.h"
#include "addressspacereadreply.h"
#include "addressspacereply.h"
#include "addressspacewritereply.h"
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
        Q_EMIT memoryReadFailed(0, QString("Memory space 0x%1 is invalid").arg(m_segment_id, 2, 16, QLatin1Char('0')));
        return;
    }

    if(m_currentReadState == MemoryReadState::Read_Space_Info ||
        m_currentReadState == MemoryReadState::Reading_Memory){
        LOG4CXX_ERROR(logger, "Can't read memory: request already in process");
        return;
    }
    m_currentReadState = MemoryReadState::Read_Space_Info;
    m_currentOffset = 0;
    m_data.clear();
    m_hasData = false;

    m_reply = m_conn->queryAddressSpaceInformation(m_alias, m_segment_id);
    if(!m_reply){
        m_currentReadState = MemoryReadState::Not_Read_Yet;
        Q_EMIT memoryReadFailed(0, "Unable to send address space information request");
        return;
    }

    connect(m_reply, &AddressSpaceReply::finished,
            this, &SegmentMemory::addressSpaceFinished);

    m_currentReadState = MemoryReadState::Read_Space_Info;
}

void SegmentMemory::addressSpaceFinished(){
    m_reply->deleteLater();

    if(m_reply->isError()){
        LOG4CXX_ERROR_FMT(logger, "Unable to get address space information for space {:X}: {}",
                          m_segment_id, m_reply->errorMessage().toStdString());
        m_currentReadState = MemoryReadState::Not_Read_Yet;
        uint16_t errorCode = m_reply->errorCode();
        QString message = m_reply->errorMessage();
        m_reply = nullptr;
        Q_EMIT memoryReadFailed(errorCode, message);
        return;
    }

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
        m_currentReadState = MemoryReadState::Not_Read_Yet;
        m_reply = nullptr;
        Q_EMIT memoryReadFailed(0, "Invalid address space: ending address less than starting address");
        return;
    }

    if(m_currentReadState == MemoryReadState::Read_Space_Info && m_reply->space() == m_segment_id){
        m_starting_address = m_reply->lowAddress();
        m_ending_address = m_reply->highAddress();
        if(!m_reply->exists() && m_reply->highAddress() == 0){
            // Nothing to read
            m_length = 0;
        }else{
            // The highest address is the address of the last byte in the space
            m_length = m_ending_address - m_starting_address + 1;
        }
        m_currentOffset = 0;
        m_currentReadState = MemoryReadState::Reading_Memory;

        // Now let's trigger a read of the entire segment
        readNextBlock();
    }

    m_reply = nullptr;
}

void SegmentMemory::addressSpaceRead(){
    LOG4CXX_DEBUG_FMT(logger, "Got address space read");

    m_readReply->deleteLater();

    if(m_readReply->isError()){
        if(m_currentReadState != MemoryReadState::Reading_Memory){
            return;
        }
        LOG4CXX_ERROR_FMT(logger, "Unable to read memory space {:X}: {}",
                          m_segment_id, m_readReply->errorMessage().toStdString());
        m_currentReadState = MemoryReadState::Not_Read_Yet;
        Q_EMIT memoryReadFailed(m_readReply->errorCode(), m_readReply->errorMessage());
        return;
    }

    if(m_currentReadState == MemoryReadState::Reading_Memory && m_readReply->space() == m_segment_id){
        const QVector<uint8_t> blockData = m_readReply->data();
        m_currentOffset += blockData.length();
        m_data.append(blockData);

        Q_EMIT downloadProgress(m_data.size(), m_length);

        if(blockData.isEmpty()){
            // The node gave us nothing - don't keep asking for the same block forever
            LOG4CXX_WARN_FMT(logger, "Empty read reply for space {:X} at offset {}: stopping read", m_segment_id, m_currentOffset);
            m_length = m_currentOffset;
        }

        readNextBlock();
    }
}

void SegmentMemory::readNextBlock(){
    if(m_currentOffset >= m_length){
        m_currentReadState = MemoryReadState::Memory_Complete;
        m_hasData = true;
        Q_EMIT downloadProgress(m_data.size(), m_data.size());
        Q_EMIT memoryReady();
        return;
    }

    uint32_t lenToRead = m_length - m_currentOffset;
    if(lenToRead > 64){
        lenToRead = 64;
    }

    m_readReply = m_conn->readSingleMemoryBlock(m_alias, m_segment_id, m_starting_address + m_currentOffset, lenToRead);
    if(m_readReply){
        connect(m_readReply, &AddressSpaceReadReply::finished,
                this, &SegmentMemory::addressSpaceRead);
    }else{
        m_currentReadState = MemoryReadState::Not_Read_Yet;
        Q_EMIT memoryReadFailed(0, "Unable to send memory read request");
    }
}

namespace {
/**
 * Tracks a multi-block read or write
 */
struct RangeOperation {
    int outstanding = 0;
    bool failed = false;
};
}

void SegmentMemory::updateData(uint32_t address, const QVector<uint8_t>& data){
    if(address < m_starting_address){
        return;
    }

    qsizetype index = address - m_starting_address;
    for(qsizetype x = 0; x < data.size() && index + x < m_data.size(); x++){
        m_data[index + x] = data[x];
    }
}

bool SegmentMemory::readRange(uint32_t address, int length){
    if(length <= 0){
        return false;
    }

    auto op = std::make_shared<RangeOperation>();
    for(int offset = 0; offset < length; offset += 64){
        int blockLen = std::min(64, length - offset);
        uint32_t blockAddress = address + offset;
        AddressSpaceReadReply* rep = m_conn->readSingleMemoryBlock(m_alias, m_segment_id, blockAddress, blockLen);
        if(!rep){
            // Blocks that were already queued will still report back
            if(op->outstanding == 0){
                return false;
            }
            op->failed = true;
            Q_EMIT rangeReadFailed(address, length, 0, "Unable to send memory read request");
            break;
        }

        op->outstanding++;
        connect(rep, &AddressSpaceReadReply::finished,
                this, [this, rep, op, address, length, blockAddress](){
            rep->deleteLater();
            op->outstanding--;

            if(rep->isError()){
                if(!op->failed){
                    op->failed = true;
                    Q_EMIT rangeReadFailed(address, length, rep->errorCode(), rep->errorMessage());
                }
                return;
            }

            updateData(blockAddress, rep->data());
            if(op->outstanding == 0 && !op->failed){
                Q_EMIT rangeRead(address, length);
            }
        });
    }

    return true;
}

bool SegmentMemory::writeRange(uint32_t address, const QVector<uint8_t>& data){
    if(data.isEmpty()){
        return false;
    }

    auto op = std::make_shared<RangeOperation>();
    int length = data.size();
    for(int offset = 0; offset < length; offset += 64){
        QVector<uint8_t> block = data.mid(offset, 64);
        uint32_t blockAddress = address + offset;
        AddressSpaceWriteReply* rep = m_conn->writeSingleMemoryBlock(m_alias, m_segment_id, blockAddress, block);
        if(!rep){
            if(op->outstanding == 0){
                return false;
            }
            op->failed = true;
            Q_EMIT rangeWriteFailed(address, length, 0, "Unable to send memory write request");
            break;
        }

        op->outstanding++;
        connect(rep, &AddressSpaceWriteReply::finished,
                this, [this, rep, op, address, length, blockAddress, block](){
            rep->deleteLater();
            op->outstanding--;

            if(rep->isError()){
                LOG4CXX_ERROR_FMT(logger, "Unable to write memory space {:X} address {}: {}",
                                  m_segment_id, blockAddress, rep->errorMessage().toStdString());
                if(!op->failed){
                    op->failed = true;
                    Q_EMIT rangeWriteFailed(address, length, rep->errorCode(), rep->errorMessage());
                }
                return;
            }

            // The node now has this data
            updateData(blockAddress, block);
            if(op->outstanding == 0 && !op->failed){
                Q_EMIT rangeWritten(address, length);
            }
        });
    }

    return true;
}
