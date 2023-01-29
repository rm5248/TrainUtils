/* SPDX-License-Identifier: GPL-2.0 */
#include "loconetswitchcontrol.h"
#include "ui_loconetswitchcontrol.h"
#include "loconet/loconetconnection.h"

LoconetSwitchControl::LoconetSwitchControl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LoconetSwitchControl)
{
    ui->setupUi(this);
}

LoconetSwitchControl::~LoconetSwitchControl()
{
    delete ui;
}

void LoconetSwitchControl::setLoconetConnection(std::shared_ptr<LoconetConnection> conn){
    m_conn = conn;
}

void LoconetSwitchControl::on_throwButton_clicked()
{
    bool ok;
    int num = ui->switchNum->text().toInt(&ok);
    if(!num){
        return;
    }
    m_conn->throwSwitch(num);
}


void LoconetSwitchControl::on_closeButton_clicked()
{
    bool ok;
    int num = ui->switchNum->text().toInt(&ok);
    if(!num){
        return;
    }
    m_conn->closeSwitch(num);
}

