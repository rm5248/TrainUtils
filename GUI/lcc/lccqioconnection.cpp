/* SPDX-License-Identifier: GPL-2.0 */
#include "lccqioconnection.h"

#include <log4cxx/logger.h>
#include <fmt/format.h>
#include <QTimer>

#include "lcc-print.h"

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui.lcc.LCCQIoConnection" );

LCCQIoConnection::LCCQIoConnection(QObject *parent) : LCCConnection(parent)
{
    m_lccGrid = lcc_gridconnect_new();

    lcc_context_set_userdata(m_lcc, this);
    lcc_context_set_write_function(m_lcc, LCCQIoConnection::writeLCCFrameCB);
    lcc_gridconnect_set_userdata(m_lccGrid, this);
    lcc_gridconnect_set_frame_parsed(m_lccGrid, LCCQIoConnection::gridconnectLCCFrameParsedCB);
}

LCCQIoConnection::~LCCQIoConnection(){
    lcc_gridconnect_free(m_lccGrid);
}

void LCCQIoConnection::generateAliasDone(){
    int ret = lcc_context_claim_alias(m_lcc);
    if(ret != LCC_OK){
        LOG4CXX_DEBUG_FMT(logger, "Could not claim alias, trying again");
        ret = lcc_context_generate_alias(m_lcc);
        if(ret != LCC_OK){
            LOG4CXX_ERROR_FMT(logger, "LCC can't generate alias: {}", ret);
            return;
        }

        QTimer::singleShot(400, this, &LCCQIoConnection::generateAliasDone);
        return;
    }

    LOG4CXX_DEBUG_FMT(logger, "Alias claimed.  Alias: 0x{:X}", lcc_context_alias(m_lcc));
}

void LCCQIoConnection::incomingData(){
    QByteArray data = m_ioDevice->readAll();

    int ret = lcc_gridconnect_incoming_data( m_lccGrid, data.data(), data.length() );
    if( ret != 0 ){
        LOG4CXX_ERROR_FMT(logger, "LCC error: {}", ret);
    }
}

int LCCQIoConnection::writeLCCFrameCB(lcc_context* context, lcc_can_frame* frame){
    LCCQIoConnection* conn = static_cast<LCCQIoConnection*>(lcc_context_user_data(context));
    return conn->writeLCCFrame(frame);
}

int LCCQIoConnection::writeLCCFrame(lcc_can_frame* frame){
    char out_buffer[128];

    int ret = lcc_canframe_to_gridconnect(frame, out_buffer, sizeof(out_buffer));
    LOG4CXX_DEBUG_FMT(logger, "Write the following frame: {}", out_buffer);
    if(ret != LCC_OK){
        LOG4CXX_ERROR_FMT(logger, "LCC error: {}", ret);
        return ret;
    }

    QByteArray ba(out_buffer);
    ba.append('\n');

    if(m_ioDevice->write(ba) < 0){
        return LCC_ERROR_TX;
    }

    return LCC_OK;
}

void LCCQIoConnection::gridconnectLCCFrameParsedCB(lcc_gridconnect* context, lcc_can_frame* frame){
    LCCQIoConnection* conn = static_cast<LCCQIoConnection*>(lcc_gridconnect_user_data(context));
    conn->gridconnectLCCFrameParsed(frame);
}

void LCCQIoConnection::gridconnectLCCFrameParsed(lcc_can_frame* frame){
    if(logger->isDebugEnabled()){
        LOG4CXX_DEBUG_FMT(logger, "Parsed frame:");
        lcc_decode_frame(frame, stderr, 0);
        fflush(stderr);
    }

    Q_EMIT incomingRawFrame(frame);
    lcc_context_incoming_frame(m_lcc, frame);
    lcc_network_incoming_frame(m_lccNetwork, frame);
}

void LCCQIoConnection::updateQIODeviceConnections(){
    LOG4CXX_ASSERT_FMT(logger, m_ioDevice != nullptr, "IODevice is nullptr: programming issue");
    m_ioDevice->setParent(this);

    connect( m_ioDevice, &QIODevice::readyRead,
             this, &LCCQIoConnection::incomingData );
}

void LCCQIoConnection::generateAlias(){
    int ret = lcc_context_generate_alias(m_lcc);
    if(ret != LCC_OK){
        LOG4CXX_ERROR_FMT(logger, "LCC can't generate alias: {}", ret);
        return;
    }

    QTimer::singleShot(400, this, &LCCQIoConnection::generateAliasDone);
}
