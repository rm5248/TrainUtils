/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCCNODE_H
#define LCCNODE_H

#include <QObject>
#include <QByteArray>

#include "cdi/cdi.h"

struct lcc_node_info;
struct lcc_context;

class LCCConnection;
class AddressSpaceReply;
class AddressSpaceReadReply;
class SegmentMemory;
class UpdateCompleteReply;

/**
 * Represents a single LCC node.
 */
class LCCNode : public QObject
{
    Q_OBJECT
public:
    explicit LCCNode(lcc_node_info* inf, LCCConnection* conn, QObject *parent = nullptr);

    bool valid() const;

    /**
     * Read the entire CDI for this node.
     * Listen for the cdiRead signal to know when the CDI is available.
     *
     * @return The SegmentMemory that represents the CDI for this node.  This is used to get a download percentage
     */
    SegmentMemory* readCDI();

    /**
     * The parsed CDI data.
     *
     * @return
     */
    CDI cdi() const;

    /**
     * Check to see if the CDI has been read and parsed for this node.
     *
     * @return
     */
    bool hasCDI() const;

    /**
     * Use a CDI that we already have(e.g. from another node of the same model)
     * instead of reading it from the node.  Emits cdiRead.
     */
    void setCDI(const CDI& cdi);

    /**
     * Tell the node that we are done changing its configuration.  Send this after writing.
     *
     * @return Can be used to find out if the node accepted it.  Deletes itself when finished.
     */
    UpdateCompleteReply* sendUpdateComplete();

    /**
     * Check to see if we have all memory read from this node.
     *
     * @return
     */
    bool hasAllMemory();

    /**
     * Get the raw memory for the specified segment.
     *
     * @param segment
     * @return
     */
    SegmentMemory* segmentMemory(uint8_t segment);

Q_SIGNALS:
    /**
     * Emitted when the CDI has been read.
     */
    void cdiRead();
    void cdiReadFailure(uint16_t error_code, QString error_string);

private Q_SLOTS:
    void cdiReady();

private:
    LCCConnection* m_conn;
    lcc_node_info* m_nodeInfo;
    CDI m_cdi;
    bool m_hasCDI = false;

    // Memory information
    QVector<SegmentMemory*> m_memories;
};

#endif // LCCNODE_H
