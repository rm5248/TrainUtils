/* SPDX-License-Identifier: GPL-2.0 */
#include "lccnetworkview.h"
#include "ui_lccnetworkview.h"

LCCNetworkView::LCCNetworkView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LCCNetworkView)
{
    ui->setupUi(this);
}

LCCNetworkView::~LCCNetworkView()
{
    delete ui;
}

void LCCNetworkView::setLCCConnection(std::shared_ptr<LCCConnection> lcc){
    m_connection = lcc;
}
