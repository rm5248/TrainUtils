/* SPDX-License-Identifier: GPL-2.0 */
#include "lccmemorydisplay.h"
#include "ui_lccmemorydisplay.h"
#include "lcc/lccconnection.h"
#include <lcc-memory.h>
#include <log4cxx/logger.h>
#include <fmt/format.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui.lcc.LCCMemoryDisplay" );

lccmemorydisplay::lccmemorydisplay(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::lccmemorydisplay)
{
    ui->setupUi(this);

    ui->memorySpaceCombo->addItem("Manual Input");
    ui->memorySpaceCombo->addItem("Configuration Definition(0xFF)");
    ui->memorySpaceCombo->addItem("All Memory(0xFE)");
    ui->memorySpaceCombo->addItem("Configuration(0xFD)");
}

lccmemorydisplay::~lccmemorydisplay()
{
    delete ui;
}

void lccmemorydisplay::setLCCConnection(std::shared_ptr<LCCConnection> lcc){
    m_connection = lcc;

    connect(m_connection.get(), &LCCConnection::incomingDatagram,
            this, &lccmemorydisplay::incomingDatagram);
    connect(m_connection.get(), &LCCConnection::datagramReceivedOK,
            this, &lccmemorydisplay::datagramReceivedOK);
    connect(m_connection.get(), &LCCConnection::datagramRejected,
            this, &lccmemorydisplay::datagramRejected);
}

void lccmemorydisplay::on_readMemory_clicked()
{
    bool ok;
    int alias = ui->aliasInput->text().toInt(&ok, 16);
    if(!ok){
        return;
    }

    int space;
    if(ui->memorySpaceCombo->currentIndex() == 0){
        space = ui->memorySpaceInput->text().toInt(&ok, 16);
        if(!ok){
            return;
        }
    }else if(ui->memorySpaceCombo->currentIndex() == 1){
        space = 0xFF;
    }else if(ui->memorySpaceCombo->currentIndex() == 2){
        space = 0xFE;
    }else if(ui->memorySpaceCombo->currentIndex() == 3){
        space = 0xFD;
    }

    uint32_t startingAddress = ui->startingAddressInput->text().toUInt(&ok, 16);
    if(!ok){
        return;
    }

    int length = ui->lengthInput->text().toInt(&ok, 16);
    if(!ok){
        return;
    }

    m_connection->readSingleMemoryBlock(alias, space, startingAddress, length);
}

void lccmemorydisplay::on_memorySpaceCombo_activated(int index)
{
    if(index == 0){
        ui->memorySpaceInput->setEnabled(true);
    }else{
        ui->memorySpaceInput->setEnabled(false);
    }
}

void lccmemorydisplay::incomingDatagram(QByteArray datagramData){
    LOG4CXX_DEBUG_FMT(logger, "Got a datagram of {} bytes", datagramData.length());

    // TODO display the data as a hexdump
    std::string data;
    for(uint8_t byte : datagramData){
        data.append(fmt::format("0x{:X} ", byte));
    }
    LOG4CXX_DEBUG_FMT(logger, "{}", data);
}

void lccmemorydisplay::datagramReceivedOK(uint8_t flags){
    LOG4CXX_DEBUG_FMT(logger, "Datagram RX OK!");
}

void lccmemorydisplay::datagramRejected(uint16_t error_code, QByteArray optional_data){
    LOG4CXX_DEBUG_FMT(logger, "Datagram Rejected!  code: 0x{:X}", error_code);
}
