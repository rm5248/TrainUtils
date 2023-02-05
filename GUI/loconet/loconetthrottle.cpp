/* SPDX-License-Identifier: GPL-2.0 */
#include "loconetthrottle.h"

#include "loconetconnection.h"

#include <log4cxx/logger.h>
#include <fmt/format.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui.loconet.Throttle" );

LoconetThrottle::LoconetThrottle(struct loconet_context* context,
                                 QObject *parent) : Throttle(parent)
{
    m_throttle = loconet_throttle_new(context);
    loconet_throttle_set_userdata(m_throttle, this);
}

LoconetThrottle::~LoconetThrottle(){
    loconet_throttle_free(m_throttle);
}

void LoconetThrottle::selectLocomotive(int locomotiveNumber, bool isLong){
    LOG4CXX_DEBUG_FMT(logger, "Select locomotive {}.  Long? {}",
                      locomotiveNumber,
                      isLong);
    int flags = LOCONET_THROTTLE_SELECT_FLAG_AUTO_STEAL;
    if(isLong){
        flags |= LOCONET_THROTTLE_SELECT_FLAG_LONG_ADDR;
    }

    loconet_throttle_select_locomotive(m_throttle, locomotiveNumber, flags);
}

void LoconetThrottle::setSpeed(int speed){
    loconet_throttle_set_speed(m_throttle, speed);
}

void LoconetThrottle::setDirection(LocomotiveDirection dir){
    switch(dir){
    case LocomotiveDirection::Forward:
        loconet_throttle_set_direction(m_throttle, 1);
        break;
    case LocomotiveDirection::Reverse:
        loconet_throttle_set_direction(m_throttle, 0);
        break;
    }
}

void LoconetThrottle::estop(){
    loconet_throttle_estop(m_throttle);
}

void LoconetThrottle::setFunction(int funcNum, bool on){
    loconet_throttle_set_function(m_throttle, funcNum, on);
}

void LoconetThrottle::releaseLocomotive(){
    loconet_throttle_dispatch(m_throttle);
}

void LoconetThrottle::toggleFunction(int funcNum){
    int state = loconet_throttle_get_function_state(m_throttle, funcNum);
    if(state < 0){
        return;
    }

    loconet_throttle_set_function(m_throttle, funcNum, !state);
}
