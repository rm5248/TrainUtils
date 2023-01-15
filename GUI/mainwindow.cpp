/* SPDX-License-Identifier: GPL-2.0 */
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "trainutils_state.h"
#include "lcc/lccmanager.h"

#include <QInputDialog>
#include <QHostAddress>
#include <log4cxx/logger.h>
#include <fmt/format.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui.mainwindow" );


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_dockManager = new ads::CDockManager(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setTrainUtilsState(TrainUtilsState* state){
    m_state = state;
}

void MainWindow::on_actionExit_triggered()
{
    QApplication::exit();
}


void MainWindow::on_action_lcc_Manual_IP_triggered()
{
    QString text = QInputDialog::getText(this, "Input IP/port", "Input the IP/port of the remote LCC system.  Example: 127.0.0.1:12021");
    if( text.isNull() || text.isEmpty() ){
        return;
    }

    QStringList list = text.split(":");
    if( list.size() != 2 ){
        // TODO give feedback to the user
        LOG4CXX_ERROR_FMT(logger, "User did not input IP/port correctly");
        return;
    }

    bool ok = false;
    QHostAddress addr(list[0]);
    int port = list[1].toInt(&ok);

    if(addr.isNull()){
        // TODO give feedback to the user
        LOG4CXX_ERROR_FMT(logger, "User did not input IP");
        return;
    }

    if(!ok){
        // TODO give feedback to the user
        LOG4CXX_ERROR_FMT(logger, "User did not input port");
        return;
    }

    std::shared_ptr<LCCConnection> conn = m_state->lccManager->createNewNetworkLCC("LCC1", addr, port);
    if(conn){
        ui->menuLCC->addAction(conn->name());
    }
}
