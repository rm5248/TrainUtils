/* SPDX-License-Identifier: GPL-2.0 */
#include "lccnode.h"
#include "lcc-memory.h"
#include "lcc-node-info.h"
#include "lcc-datagram.h"
#include "lcc-remote-memory.h"
#include "lccconnection.h"
#include "segmentmemory.h"

#include <log4cxx/logger.h>
#include <fmt/format.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui.lcc.LCCNode" );

LCCNode::LCCNode(lcc_node_info* inf, LCCConnection* conn, QObject *parent) :
    QObject(parent),
    m_nodeInfo(inf),
    m_conn(conn)
{
}

bool LCCNode::valid() const{
    return m_nodeInfo != nullptr;
}

void LCCNode::readCDI(){
    if(m_nodeInfo == nullptr){
        return;
    }

    SegmentMemory* cdi_mem = segmentMemory(LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION);
    if(cdi_mem == nullptr){
        uint16_t alias = lcc_node_info_get_alias(m_nodeInfo);
        cdi_mem = new SegmentMemory(m_conn, alias, LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION, this);
        m_memories.push_back(cdi_mem);

        connect(cdi_mem, &SegmentMemory::memoryReady,
                this, &LCCNode::cdiReady);
    }
    cdi_mem->readAllMemory();
}

CDI LCCNode::cdi() const{
    return m_cdi;
}

SegmentMemory* LCCNode::segmentMemory(uint8_t segment){
    for(SegmentMemory* mem : m_memories){
        if(mem->segmentId() == segment){
            return mem;
        }
    }

    return nullptr;
}

void LCCNode::cdiReady(){
    SegmentMemory* cdi_segment = segmentMemory(255);
    const QVector<uint8_t> data = cdi_segment->data();
    QXmlStreamReader rdr(QByteArray::fromRawData((const char*)data.data(), data.length()));
    m_cdi = CDI::createFromXML(&rdr);
    LOG4CXX_DEBUG_FMT(logger, "Read entire CDI!");

    // Create segments for each segment found in the CDI, if it does not already exist
    for(const Segment& seg : m_cdi.segments()){
        SegmentMemory* current = segmentMemory(seg.space());
        if(current){
            continue;
        }

        LOG4CXX_DEBUG_FMT(logger, "Creating segment {}", seg.space());
        uint16_t alias = lcc_node_info_get_alias(m_nodeInfo);
        current = new SegmentMemory(m_conn, alias, seg.space(), this);
        m_memories.push_back(current);
    }

    Q_EMIT cdiRead();
}
