/* SPDX-License-Identifier: GPL-2.0 */
#include "loconet-tcp.h"

#include "loconet_buffer.h"

#include <regex>
#include <string_view>

using loconet::LoconetTCP;

static std::regex regex_expressions[] = {
    std::regex("VERSION (.*)[\n\r]$"),
    std::regex("RECEIVE (.*)[\n\r]$"),
    std::regex("SENT (OK|ERROR) (.*)[\n\r]$"),
    std::regex("TIMESTAMP (\\d+)[\n\r]$"),
    std::regex("BREAK (\\d+)[\n\r]$"),
    std::regex("ERROR CHECKSUM (.*)[\n\r]$"),
    std::regex("ERROR LINE (.*)[\n\r]$"),
    std::regex("ERROR MESSAGE (.*)[\n\r]$"),
    std::regex("ERROR (.*)[\n\r]$"),
};

struct LoconetTCP::LoconetTCPPriv{
    struct loconet_context* m_context;
    std::function<void(uint8_t*,int)> writeDataFun;
    std::function<void(std::string)> m_versionCB;
    std::function<void(const std::vector<uint8_t>&)> m_receiveCB;
    std::function<void(bool, std::string)> m_sentCB;
    std::function<void(uint32_t)> m_timestampCB;
    std::function<void(uint32_t)> m_breakCB;
    std::function<void(std::string)> m_errorCB;
    std::function<void(std::string)> m_errorLineCB;
};

LoconetTCP::LoconetTCP() :
    m_priv(std::make_unique<LoconetTCPPriv>()){
    m_priv->m_context = ln_context_new_interlocked(LoconetTCP::writeInterlockFunction);
    ln_set_user_data(m_priv->m_context, this);
}

LoconetTCP::~LoconetTCP(){}

void LoconetTCP::incomingData(void* data, size_t len){
    // For now, we will assume that we get an entire line of data at once and we don't
    // need to buffer the data.
    std::string view((const char*)data, len);

    size_t regex_num;
    bool found = false;
    std::smatch match;
    for( regex_num = 0;
         regex_num < (sizeof(regex_expressions) / sizeof(regex_expressions[0]));
         regex_num++ ){
        if( std::regex_match( view, match, regex_expressions[regex_num] ) ){
            found = true;
            break;
        }
    }

    if( !found ){
        return;
    }

    switch(regex_num){
    case 0: // VERSION
        if(m_priv->m_versionCB){
            m_priv->m_versionCB(match[1]);
        }
        break;
    case 1: // RECEIVE
        handleReceivedData(match[1]);
        break;
    case 2: // SENT
    {
        bool ok = false;
        if(match[1].compare("OK") == 0){
            ok = true;
        }
        if(match.size() == 2){
            if(m_priv->m_sentCB){
               m_priv->m_sentCB(ok, match[2]);
            }
        }else{
            if(m_priv->m_sentCB){
               m_priv->m_sentCB(ok, std::string());
            }
        }
    }
    case 3: // TIMESTAMP
        if(m_priv->m_timestampCB){
            uint32_t timestamp = std::stoi(match[1]);
            m_priv->m_timestampCB(timestamp);
        }
        break;
    case 4: // BREAK
        if(m_priv->m_breakCB){
            uint32_t break_time = std::stoi(match[1]);
            m_priv->m_breakCB(break_time);
        }
        break;
    case 5: // ERROR CHECKSUM
        break;
    case 6: // ERROR LINE
        if(m_priv->m_errorLineCB){
            m_priv->m_errorLineCB(match[1]);
        }
        break;
    case 7: // ERROR MESSAGE
        break;
    case 8: // ERROR
        if(m_priv->m_errorCB){
            m_priv->m_errorCB(match[1]);
        }
        break;
    }
}

void LoconetTCP::handleReceivedData(const std::string &data_str){
    std::regex data_regex;
    std::vector<uint8_t> data;

    std::regex hex_regex("([A-H0-9][A-H0-9])");
    std::sregex_iterator bytes_begin =
        std::sregex_iterator(data_str.begin(), data_str.end(), hex_regex);
    std::sregex_iterator bytes_end = std::sregex_iterator();

    for (std::sregex_iterator i = bytes_begin; i != bytes_end; ++i) {
        int byte = std::stoi(i->str());
        data.push_back( byte & 0xFF );
    }

    // Push data to the loconet context
    for( uint8_t byte : data ){
        ln_incoming_byte( m_priv->m_context, byte );
    }
    if(m_priv->m_receiveCB){
        m_priv->m_receiveCB(data);
    }
}

void LoconetTCP::incomingData(const std::vector<uint8_t>& data){
    incomingData( (void*)data.data(), data.size() );
}

struct loconet_context* LoconetTCP::loconetContext() const{
    return m_priv->m_context;
}

void LoconetTCP::setVersionCallback(std::function<void(std::string)> versionCb){
    m_priv->m_versionCB = versionCb;
}

void LoconetTCP::setReceiveCallback(std::function<void(const std::vector<uint8_t>&)> receiveCB){
    m_priv->m_receiveCB = receiveCB;
}

void LoconetTCP::setSentCallback(std::function<void(bool, std::string)> sentCB){
    m_priv->m_sentCB = sentCB;
}

void LoconetTCP::setTimestampCallback(std::function<void(uint32_t)> timestampCB){
    m_priv->m_timestampCB = timestampCB;
}

void LoconetTCP::setBreakCallback(std::function<void(uint32_t)> breakCB){
    m_priv->m_breakCB = breakCB;
}

void LoconetTCP::setErrorCallback(std::function<void(std::string)> errorCB){
    m_priv->m_errorCB = errorCB;
}

void LoconetTCP::setErrorLineCallback(std::function<void(std::string)> errorLineCB){
    m_priv->m_errorLineCB = errorLineCB;
}

void LoconetTCP::writeInterlockFunction( struct loconet_context* ctx, uint8_t* data, int len ){
    LoconetTCP* tcp = static_cast<LoconetTCP*>(ln_user_data(ctx));

    if(tcp->m_priv->writeDataFun){
        tcp->m_priv->writeDataFun(data, len);
    }
}
