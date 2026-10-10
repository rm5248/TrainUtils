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

    /**
     * Read part of the memory from the node, updating our copy of it.
     * Emits rangeRead or rangeReadFailed when done.
     *
     * @return false if the read could not be started
     */
    bool readRange(uint32_t address, int length);

    /**
     * Write data to the node.  Our copy of the memory is updated as the node
     * accepts the data.  Emits rangeWritten or rangeWriteFailed when done.
     *
     * @return false if the write could not be started
     */
    bool writeRange(uint32_t address, const QVector<uint8_t>& data);

Q_SIGNALS:
    void memoryReady();
    /**
     * Emitted when reading the memory fails.
     *
     * @param error_code The LCC error code, or 0 if the failure was local
     * @param message A description of the failure
     */
    void memoryReadFailed(uint16_t error_code, QString message);
    void downloadProgress(uint64_t bytes_received, uint64_t bytes_total);

    void rangeRead(uint32_t address, int length);
    void rangeReadFailed(uint32_t address, int length, uint16_t error_code, QString message);
    void rangeWritten(uint32_t address, int length);
    void rangeWriteFailed(uint32_t address, int length, uint16_t error_code, QString message);

private Q_SLOTS:
    void addressSpaceFinished();
    void addressSpaceRead();

private:
    void readNextBlock();
    void updateData(uint32_t address, const QVector<uint8_t>& data);

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
    uint32_t m_length = 0;
    uint32_t m_currentOffset;
    MemoryReadState m_currentReadState = MemoryReadState::Not_Read_Yet;
    QVector<uint8_t> m_data;
    bool m_hasData = false;
    AddressSpaceReply* m_reply = nullptr;
    AddressSpaceReadReply* m_readReply = nullptr;
    bool m_invalid = false;
};

#endif // SEGMENTMEMORY_H
