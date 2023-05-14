/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCCMEMORYDISPLAY_H
#define LCCMEMORYDISPLAY_H

#include <QWidget>
#include <memory>

namespace Ui {
class lccmemorydisplay;
}

class LCCConnection;

class lccmemorydisplay : public QWidget
{
    Q_OBJECT

public:
    explicit lccmemorydisplay(QWidget *parent = nullptr);
    ~lccmemorydisplay();

    void setLCCConnection(std::shared_ptr<LCCConnection> lcc);

private Q_SLOTS:
    void on_memorySpaceCombo_activated(int index);
    void on_readMemory_clicked();
    void incomingDatagram(QByteArray datagramData);
    void datagramReceivedOK(uint8_t flags);
    void datagramRejected(uint16_t error_code, QByteArray optional_data);

private:
    Ui::lccmemorydisplay *ui;
    std::shared_ptr<LCCConnection> m_connection;
};

#endif // LCCMEMORYDISPLAY_H
