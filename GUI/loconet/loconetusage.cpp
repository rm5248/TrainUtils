/* SPDX-License-Identifier: GPL-2.0 */
#include "loconetusage.h"

#include <QTextStream>

LoconetUsage::LoconetUsage(QObject *parent)
    : QObject{parent},
      m_currentNumPackets(0),
      m_collecting(true)
{
    m_calculateTimer.setInterval(1000);
    m_calculateTimer.start();

    connect(&m_calculateTimer, &QTimer::timeout,
            this, &LoconetUsage::calculateNumPackets);
}

void LoconetUsage::incomingLoconetMessage(loconet_message message){
    if(!m_collecting){
        return;
    }
    m_currentNumPackets++;
}

void LoconetUsage::calculateNumPackets(){
    PacketInfo pi;
    pi.numReceived = m_currentNumPackets;
    pi.second = QDateTime::currentDateTime();

    m_packetInfo.push_back(pi);

    m_currentNumPackets = 0;
}


void LoconetUsage::clear(){
    m_packetInfo.clear();
}

bool LoconetUsage::logStatsToFile(std::filesystem::path path){
    QFile f(path.c_str());
    bool ret = f.open(QIODevice::WriteOnly);
    if(!ret){
        return ret;
    }

    QTextStream ts(&f);

    ts << "Time,NumPackets\n";
    for(PacketInfo pi : m_packetInfo){
        ts << pi.second.toString(Qt::ISODate) << "," << pi.numReceived << "\n";
    }

    return true;
}

void LoconetUsage::collectStats(bool collect){
    if(collect){
        m_calculateTimer.start();
    }else{
        m_calculateTimer.stop();
    }
}
