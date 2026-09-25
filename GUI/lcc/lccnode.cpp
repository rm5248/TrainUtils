/* SPDX-License-Identifier: GPL-2.0 */
#include "lccnode.h"
#include "lcc-memory.h"
#include "lcc-node-info.h"
#include "lcc-datagram.h"
#include "lcc-remote-memory.h"
#include "lccconnection.h"

#include <log4cxx/logger.h>
#include <fmt/format.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui.lcc.LCCNode" );

LCCNode::LCCNode(lcc_node_info* inf, LCCConnection* conn, QObject *parent) :
    QObject(parent),
    m_nodeInfo(inf),
    m_conn(conn),
    m_hasCDI(false),
    m_cdiSize(-1)
{
    m_rawcdi.reserve(1024);
}

bool LCCNode::valid() const{
    return m_nodeInfo != nullptr;
}

bool LCCNode::hasCDI() const{
    return m_hasCDI;
}

void LCCNode::readCDI(){
    if(m_nodeInfo == nullptr){
        return;
    }
    if(m_cdiReadState != CDIReadState::Not_Read_Yet){
        LOG4CXX_ERROR(logger, "Can't read CDI: request already in process");
        return;
    }

    uint16_t alias = lcc_node_info_get_alias(m_nodeInfo);

    m_reply = m_conn->queryAddressSpaceInformation(alias, LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION);
    if(!m_reply){
        return;
    }

    connect(m_reply, &AddressSpaceReply::finished,
        this, &LCCNode::addressSpaceFinished);

    m_cdiReadState = CDIReadState::Read_Space_Info;
}

QString LCCNode::rawCDI() const{
    return m_rawcdi;
}

CDI LCCNode::cdi() const{
    return m_cdi;
}

void LCCNode::addressSpaceFinished(){
    uint16_t alias = lcc_node_info_get_alias(m_nodeInfo);

    m_reply->deleteLater();

    LOG4CXX_DEBUG_FMT(logger, "Address space information: Space {:X} exists? {} low address {} high address {}",
        m_reply->space(),
        m_reply->exists(),
        m_reply->lowAddress(),
                      m_reply->highAddress());


    if(!m_reply->exists() && m_reply->highAddress() != 0){
        LOG4CXX_WARN(logger, "Memory segment does not exist but high address is set: assuming it actually does exist");
    }


    if(m_cdiReadState == CDIReadState::Read_Space_Info && m_reply->space() == 255){
        m_cdiSize = m_reply->highAddress();
        m_cdiCurrentOffset = 0;
        m_cdiReadState = CDIReadState::Reading_CDI;

        // Now let's trigger a read of the entire CDI
        m_readReply = m_conn->readSingleMemoryBlock(alias, 255, m_cdiCurrentOffset, 64);
        if(m_readReply){
            connect(m_readReply, &AddressSpaceReadReply::finished,
                    this, &LCCNode::addressSpaceRead);
        }
    }

    m_reply = nullptr;
}

void LCCNode::addressSpaceRead(){
    uint16_t alias = lcc_node_info_get_alias(m_nodeInfo);

    LOG4CXX_DEBUG_FMT(logger, "Got address space read");

    m_readReply->deleteLater();

    if(m_cdiReadState == CDIReadState::Reading_CDI && m_readReply->space() == 255){
        m_cdiCurrentOffset += m_readReply->data().length();
        m_rawcdi.append(m_readReply->data());

        uint32_t lenToRead = m_cdiSize - m_cdiCurrentOffset;
        if(lenToRead > 64){
            lenToRead = 64;
        }

        if(lenToRead > 0){
            // Read the next block of the CDI
            m_readReply = m_conn->readSingleMemoryBlock(alias, 255, m_cdiCurrentOffset, lenToRead);
            if(m_readReply){
                connect(m_readReply, &AddressSpaceReadReply::finished,
                        this, &LCCNode::addressSpaceRead);
            }
        }else{
            m_cdiReadState = CDIReadState::CDI_Complete;
            m_hasCDI = true;
            LOG4CXX_DEBUG_FMT(logger, "Read entire CDI!");
            Q_EMIT cdiRead();
        }
    }
}
