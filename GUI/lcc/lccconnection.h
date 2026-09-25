/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCCCONNECTION_H
#define LCCCONNECTION_H

#include <QObject>
#include <QMap>
#include <memory>

#include "lcc.h"
#include "lcc-network-info.h"
#include "../systemconnection.h"
#include "addressspacereply.h"

class LCCNode;

/**
 * Represents a connection to the LCC bus.
 *
 * Requests/responses are based off the QNetworkAccessManager API.  Create
 * a request, get an object handle for the response.  A signal will be emitted
 * when the response is ready.
 */
class LCCConnection : public SystemConnection
{
    Q_OBJECT
public:
    explicit LCCConnection(QObject *parent = nullptr);

    ~LCCConnection();

    std::shared_ptr<Turnout> getDCCTurnout(int switch_num);

    void setSimpleNodeInformation(QString manufacturer,
                                  QString model,
                                  QString hwVersion,
                                  QString swVersion);

    void setSimpleNodeNameDescription(QString nodeName,
                                      QString nodeDescription);

    void sendEvent(uint64_t event_id);

    void refreshNetwork();

    struct lcc_node_info* lccNodeInfoForID(uint64_t node_id);

    /**
     * Read a single memory block(up to 64 bytes in length).
     *
     * @param alias
     * @param space
     * @param starting_address
     * @param len
     */
    void readSingleMemoryBlock(int alias, int space, uint32_t starting_address, int len);

    /**
     * Query a node for information about the address space.
     *
     * @param alias
     * @param space
     */
    AddressSpaceReply* queryAddressSpaceInformation(int alias, int space);

    std::shared_ptr<LCCNode> lccNodeForID(uint64_t node_id);

    void setCDI(QString cdi);

Q_SIGNALS:
    void incomingRawFrame(lcc_can_frame* frame);
    void incomingEvent(uint64_t event_id);
    void newNodeDiscovered(uint64_t node_id);
    void nodeInformationUpdated(uint64_t node_id);
    void incomingDatagram(uint16_t source_alias, QByteArray datagramData);
    void datagramReceivedOK(uint16_t source_alias, uint8_t flags);
    void datagramRejected(uint16_t source_alias, uint16_t error_code, QByteArray optional_data);
    void addressSpaceRequestFinished(AddressSpaceReply* rep);

protected:
    QString connectionType();

private:
    static void remote_memory_informationCB(struct lcc_remote_memory_context* ctx, uint16_t alias, int exists, int readonly, uint8_t address_space, uint32_t lowest_address, uint32_t highest_address, const char* message);
    void remoteMemoryInformation(uint16_t alias, int exists, int readonly, uint8_t address_space, uint32_t lowest_address, uint32_t highest_address, const char* message);

protected:
    struct lcc_context* m_lcc;
    struct lcc_network_info* m_lccNetwork;
    QMap<uint64_t,std::shared_ptr<LCCNode>> m_nodes;
    QByteArray m_cdi;
    QVector<AddressSpaceReply*> m_inflight_replies;
};

#endif // LCCCONNECTION_H
