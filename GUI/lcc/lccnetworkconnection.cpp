/* SPDX-License-Identifier: GPL-2.0 */
#include <QHostAddress>
#include <QTimer>

#include <log4cxx/logger.h>
#include <fmt/format.h>

#include "lccnetworkconnection.h"
#include "lcc-print.h"

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui.lcc.LCCNetworkConnection" );

LCCNetworkConnection::LCCNetworkConnection(QObject *parent) : LCCConnection(parent)
{
    m_lccGrid = lcc_gridconnect_new();

    lcc_context_set_userdata(m_lcc, this);
    lcc_context_set_write_function(m_lcc, LCCNetworkConnection::writeLCCFrameCB);
    lcc_gridconnect_set_userdata(m_lccGrid, this);
    lcc_gridconnect_set_frame_parsed(m_lccGrid, LCCNetworkConnection::gridconnectLCCFrameParsedCB);

    connect( &m_socket, &QAbstractSocket::stateChanged,
             this, &LCCNetworkConnection::stateChanged);
    connect( &m_socket, &QAbstractSocket::readyRead,
             this, &LCCNetworkConnection::incomingData );
}

LCCNetworkConnection::~LCCNetworkConnection(){
    lcc_gridconnect_free(m_lccGrid);
}

void LCCNetworkConnection::connectToRemote(QHostAddress addr, uint16_t port){
    LOG4CXX_DEBUG_FMT(logger, "LCC Connecting to {}:{}", addr.toString().toStdString(), port );
    m_socket.connectToHost(addr, port);
}

void LCCNetworkConnection::stateChanged(QAbstractSocket::SocketState state){
    LOG4CXX_DEBUG_FMT(logger, "LCC socket state: {}", state);

    if(state == QAbstractSocket::ConnectedState){
        int ret = lcc_context_generate_alias(m_lcc);
        if(ret != LCC_OK){
            LOG4CXX_ERROR_FMT(logger, "LCC can't generate alias: {}", ret);
            return;
        }

        QTimer::singleShot(400, this, &LCCNetworkConnection::generateAliasDone);
    }
}

void LCCNetworkConnection::generateAliasDone(){
    int ret = lcc_context_claim_alias(m_lcc);
    if(ret != LCC_OK){
        LOG4CXX_DEBUG_FMT(logger, "Could not claim alias, trying again");
        ret = lcc_context_generate_alias(m_lcc);
        if(ret != LCC_OK){
            LOG4CXX_ERROR_FMT(logger, "LCC can't generate alias: {}", ret);
            return;
        }

        QTimer::singleShot(400, this, &LCCNetworkConnection::generateAliasDone);
        return;
    }

    LOG4CXX_DEBUG_FMT(logger, "Alias claimed.  Alias: 0x{:X}", lcc_context_alias(m_lcc));
}

void LCCNetworkConnection::incomingData(){
    QByteArray data = m_socket.readAll();

    int ret = lcc_gridconnect_incoming_data( m_lccGrid, data.data(), data.length() );
    if( ret != 0 ){
        LOG4CXX_ERROR_FMT(logger, "LCC error: {}", ret);
    }
}

void LCCNetworkConnection::writeLCCFrameCB(lcc_context* context, lcc_can_frame* frame){
    LCCNetworkConnection* conn = static_cast<LCCNetworkConnection*>(lcc_context_user_data(context));
    conn->writeLCCFrame(frame);
}

void LCCNetworkConnection::writeLCCFrame(lcc_can_frame* frame){
    char out_buffer[128];

    int ret = lcc_canframe_to_gridconnect(frame, out_buffer, sizeof(out_buffer));
    LOG4CXX_DEBUG_FMT(logger, "Write the following frame: {}", out_buffer);
    if(ret != LCC_OK){
        LOG4CXX_ERROR_FMT(logger, "LCC error: {}", ret);
        return;
    }

    QByteArray ba(out_buffer);
    ba.append('\n');

    m_socket.write(ba);
}

void LCCNetworkConnection::gridconnectLCCFrameParsedCB(lcc_gridconnect* context, lcc_can_frame* frame){
    LCCNetworkConnection* conn = static_cast<LCCNetworkConnection*>(lcc_gridconnect_user_data(context));
    conn->gridconnectLCCFrameParsed(frame);
}

void LCCNetworkConnection::gridconnectLCCFrameParsed(lcc_can_frame* frame){
    if(logger->isDebugEnabled()){
        LOG4CXX_DEBUG_FMT(logger, "Parsed frame:");
        lcc_decode_frame(frame, stderr, 0);
        fflush(stderr);
    }

    lcc_context_incoming_frame(m_lcc, frame);
}
