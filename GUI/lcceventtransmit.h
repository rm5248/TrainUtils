/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCCEVENTTRANSMIT_H
#define LCCEVENTTRANSMIT_H

#include <QWidget>
#include <memory>

namespace Ui {
class LCCEventTransmit;
}

class LCCConnection;

class LCCEventTransmit : public QWidget
{
    Q_OBJECT

public:
    explicit LCCEventTransmit(QWidget *parent = nullptr);
    ~LCCEventTransmit();

    void setLCCConnection(std::shared_ptr<LCCConnection> lcc);

private Q_SLOTS:
    void on_sendEvent_clicked();

private:
    Ui::LCCEventTransmit *ui;
    std::shared_ptr<LCCConnection> m_lccConnection;
};

#endif // LCCEVENTTRANSMIT_H
