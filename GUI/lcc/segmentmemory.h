/* SPDX-License-Identifier: GPL-2.0 */
#ifndef SEGMENTMEMORY_H
#define SEGMENTMEMORY_H

#include <QObject>
#include <span>

class LCCNode;
class LCCConnection;
class AddressSpaceReply;
class AddressSpaceReadReply;

class SegmentMemory : public QObject
{
    Q_OBJECT
public:
    explicit SegmentMemory(LCCConnection* conn, uint16_t m_alias, uint8_t segment_id, QObject *parent = nullptr);

    uint8_t segmentId();
    uint32_t startingAddress();
    uint32_t highAddress();
    QVector<uint8_t> data();
    bool hasMemory();
    void readAllMemory();

Q_SIGNALS:
    void memoryReady();

private Q_SLOTS:
    void addressSpaceFinished();
    void addressSpaceRead();

private:
    enum class MemoryReadState{
        Not_Read_Yet,
        Read_Space_Info,
        Reading_Memory,
        Memory_Complete,
    };

    LCCConnection* m_conn;
    uint16_t m_alias;
    uint8_t m_segment_id;
    uint32_t m_starting_address;
    uint32_t m_ending_address;
    uint32_t m_currentOffset;
    MemoryReadState m_currentReadState = MemoryReadState::Not_Read_Yet;
    QVector<uint8_t> m_data;
    bool m_hasData = false;
    AddressSpaceReply* m_reply = nullptr;
    AddressSpaceReadReply* m_readReply = nullptr;
    bool m_invalid = false;
};

#endif // SEGMENTMEMORY_H
