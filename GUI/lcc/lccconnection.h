/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCCCONNECTION_H
#define LCCCONNECTION_H

#include <QObject>
#include <QMap>
#include <memory>

#include "lcc.h"
#include "lcc-network-info.h"
#include "../systemconnection.h"

class LCCNode;

class LCCConnection : public SystemConnection
{
    Q_OBJECT
public:
    explicit LCCConnection(QObject *parent = nullptr);

    ~LCCConnection();

    std::shared_ptr<Turnout> getDCCTurnout(int switch_num){}

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
     * Read a single memory block(up to 64? bytes in length).
     *
     * @param alias
     * @param space
     * @param starting_address
     * @param len
     */
    void readSingleMemoryBlock(int alias, int space, uint32_t starting_address, int len);

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

protected:
    QString connectionType();

protected:
    struct lcc_context* m_lcc;
    struct lcc_network_info* m_lccNetwork;
    QMap<uint64_t,std::shared_ptr<LCCNode>> m_nodes;
    QByteArray m_cdi;
};

#endif // LCCCONNECTION_H
