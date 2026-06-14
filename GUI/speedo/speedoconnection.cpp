/* SPDX-License-Identifier: GPL-2.0 */
#include "speedoconnection.h"

#include <QSerialPortInfo>
#include <QRegularExpression>

#include <log4cxx/logger.h>
#include <fmt/format.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui.SpeedoConnection" );


SpeedoConnection::SpeedoConnection(QObject *parent)
    : SystemConnection{parent}
{
    connect(&m_port, &QIODevice::readyRead,
            this, &SpeedoConnection::incomingData);
}

std::shared_ptr<Turnout> SpeedoConnection::getDCCTurnout(int){
    return std::shared_ptr<Turnout>();
}

void SpeedoConnection::load(QSettings& settings){

}

void SpeedoConnection::doSave(QSettings& settings){

}

QString SpeedoConnection::connectionType(){
    return "speedo";
}

bool SpeedoConnection::open(){
    if(m_port.isOpen()){
        disconnectedFromSystem();
        m_port.close();
    }

    m_port.setPortName(m_serialPortName);
    if(m_port.open(QIODevice::ReadWrite)){
        connectedToSystem();
        return true;
    }
    return false;
}

void SpeedoConnection::setSerialPortName(QString name){
    m_serialPortName = name;
}

void SpeedoConnection::incomingData(){
    // sample line: *0000;V4.0%
    static QRegularExpression parse_regex("\\*(\\d\\d\\d\\d);V(\\d\\.\\d)%");
    int counts = 0;
    double percent = 0;

    // NOTE: this assumes that all the data is read at once(which is a pretty reasonable assumption for USB based devices)
    QByteArray ba = m_port.readAll().trimmed();
    LOG4CXX_TRACE_FMT(logger, "Incoming data: {}", ba.toStdString());

    QRegularExpressionMatch match = parse_regex.match(ba);
    if(!match.hasMatch()){
        LOG4CXX_TRACE_FMT(logger, "Unable to parse data: ignoring");
        return;
    }

    bool ok;
    counts = match.captured(1).toInt(&ok);
    if(!ok){
        LOG4CXX_DEBUG_FMT(logger, "Unable to parse speed");
        return;
    }
    percent = match.captured(2).toDouble(&ok);
    if(!ok){
        LOG4CXX_DEBUG_FMT(logger, "Unable to parse percent");
        return;
    }

    // calculate speed
    // from JMRI: SpeedoConsoleFrame
    // calculate kph: r/sec * circumference converted to hours and kph in scaleFace()
    double circ = (float) ((5.95 + 0.9) * M_PI); // KPF-Zeller
    int thisScale = 87; // HO
    double sampleSpeed = ((counts / 8.) * circ * 3600 / 1.0E6 * thisScale);
    LOG4CXX_DEBUG_FMT(logger, "Speed is: {:.1f} kph percent is {}", sampleSpeed, percent);
}

QStringList SpeedoConnection::getAvailableConnections(){
    QStringList ret;

    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();

    for(QSerialPortInfo& inf : ports){
        LOG4CXX_DEBUG_FMT(logger, "Port name: {} Manufacturer: {} Description: {}",
                          inf.portName().toStdString(),
                          inf.manufacturer().toStdString(),
                          inf.description().toStdString());

//        if(inf.portName().startsWith( "loconet_")){
//            ret.push_back( inf.portName() );
//            continue;
//        }

//        if(inf.manufacturer().compare("Digitrax Inc.") == 0){
//            ret.push_back(inf.portName());
//        }
    }

    return ret;
}
