/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LOCONETUSAGE_H
#define LOCONETUSAGE_H

#include <QObject>
#include <QVector>
#include <QTimer>
#include <QDateTime>
#include <QFile>
#include <filesystem>

#include "loconet_buffer.h"

/**
 * Calculate the usage of loconet(packets per second).
 * Optionally save to file for later analysis.
 *
 * Each second, the number of packets received that second is saved
 * off locally to a vector.
 *
 */
class LoconetUsage : public QObject
{
    Q_OBJECT
public:
    struct PacketInfo{
        int numReceived;
        QDateTime second;
    };

    explicit LoconetUsage(QObject *parent = nullptr);

    bool logStatsToFile(std::filesystem::path path);
    void clear();

Q_SIGNALS:

public Q_SLOTS:
    void collectStats(bool collect);
    void incomingLoconetMessage(loconet_message message);

private Q_SLOTS:
    void calculateNumPackets();

private:
    QVector<PacketInfo> m_packetInfo;
    int m_currentNumPackets;
    QTimer m_calculateTimer;
    bool m_collecting;
};

#endif // LOCONETUSAGE_H
