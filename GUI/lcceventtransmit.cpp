/* SPDX-License-Identifier: GPL-2.0 */
#include "lcceventtransmit.h"
#include "ui_lcceventtransmit.h"
#include "lcc/lccconnection.h"

LCCEventTransmit::LCCEventTransmit(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LCCEventTransmit)
{
    ui->setupUi(this);
}

LCCEventTransmit::~LCCEventTransmit()
{
    delete ui;
}

void LCCEventTransmit::setLCCConnection(std::shared_ptr<LCCConnection> lcc){
    m_lccConnection = lcc;
}

void LCCEventTransmit::on_sendEvent_clicked()
{
    QString event = ui->eventInput->text();
    bool ok;
    uint64_t event_id;

    event = event.replace(".", "");
    event_id = event.toULongLong( &ok, 16 );

    if(ok){
        m_lccConnection->sendEvent(event_id);
    }
}

