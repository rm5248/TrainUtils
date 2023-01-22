/* SPDX-License-Identifier: GPL-2.0 */
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "trainutils_state.h"
#include "lcc/lccmanager.h"
#include "mdns/mdnsmanager.h"
#include "lcctrafficmonitor.h"
#include "loconet/loconetmanager.h"
#include "loconet/loconetconnection.h"
#include "loconettrafficmonitor.h"
#include "loconetslotmonitor.h"
#include "systemconnectionstatuswidget.h"

#include <QInputDialog>
#include <QHostAddress>
#include <QSerialPortInfo>
#include <log4cxx/logger.h>
#include <fmt/format.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui.mainwindow" );


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_dockManager = new ads::CDockManager(this);

    connect(ui->menu_loconet_connect_to, &QMenu::aboutToShow,
            this, &MainWindow::scanForLoconetConnections);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setTrainUtilsState(TrainUtilsState* state){
    m_state = state;

    connect(m_state->mdnsManager, &MDNSManager::lccServerFound,
            this, &MainWindow::lccServerFound);
    connect(m_state->mdnsManager, &MDNSManager::lccServerLeft,
            this, &MainWindow::lccServerLeft);
    connect(m_state->mdnsManager, &MDNSManager::loconetServerFound,
            this, &MainWindow::loconetServerFound);
    connect(m_state->mdnsManager, &MDNSManager::loconetServerLeft,
            this, &MainWindow::loconetServerLeft);
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

    std::shared_ptr<LCCConnection> conn = m_state->lccManager->createNewNetworkLCC(QString(), addr, port);
    if(conn){
        QMenu* menu = ui->menuLCC->addMenu(conn->name());
        addSubmenusLCCConnection(menu, conn->name());
        newConnectionMade(conn);
    }
}

void MainWindow::lccServerFound(QString serviceName, QHostAddress address, uint16_t port){
    QAction* before = nullptr;
    for( QAction* action : ui->menu_lcc_Connect_To->actions() ){
        if(action->objectName().compare("action_lcc_Serial_Connections") == 0){
            break;
        }
        before = action;
    }

    QAction* newAction = new QAction(serviceName, this);
    newAction->setProperty( "traingui_servicename", serviceName );
    newAction->setProperty( "traingui_address", address.toString() );
    newAction->setProperty( "traingui_port", port );
    connect(newAction, &QAction::triggered,
            this, [newAction,this](){
        connectToLCC(newAction);
    });
    ui->menu_lcc_Connect_To->insertAction(before, newAction);
}

void MainWindow::lccServerLeft(QString serviceName){

}

void MainWindow::connectToLCC(QAction* action){
    action->deleteLater();
    ui->menu_lcc_Connect_To->removeAction(action);

    QHostAddress addr(action->property("traingui_address").toString());
    uint16_t port = action->property("traingui_port").toInt();
    std::shared_ptr<LCCConnection> conn = m_state->lccManager->createNewNetworkLCC(QString(), addr, port);
    if(conn){
        QMenu* menu = ui->menuLCC->addMenu(conn->name());
        addSubmenusLCCConnection(menu, conn->name());
        newConnectionMade(conn);
    }
}

void MainWindow::addSubmenusLCCConnection(QMenu *parentMenu, QString connectionName){
    QAction* action = parentMenu->addAction("Traffic Monitor");
    connect(action, &QAction::triggered,
            [connectionName,this](){
        std::shared_ptr<LCCConnection> lccConn = m_state->lccManager->getConnectionByName(connectionName);
        ads::CDockWidget* DockWidget = new ads::CDockWidget(QString("%1 - Traffic Monitor").arg(lccConn->name()));
        LCCTrafficMonitor* trafficMonitor = new LCCTrafficMonitor(this);
        trafficMonitor->setLCCConnection(lccConn);
        DockWidget->setWidget(trafficMonitor);
        m_dockManager->addDockWidget(ads::TopDockWidgetArea, DockWidget);
    });
}

void MainWindow::loconetServerFound(QString serviceName, QHostAddress address, uint16_t port){
    QAction* before = nullptr;
    for( QAction* action : ui->menu_loconet_connect_to->actions() ){
        if(action->objectName().compare("action_loconet_serial_connections") == 0){
            break;
        }
        before = action;
    }

    QAction* newAction = new QAction(serviceName, this);
    newAction->setProperty( "traingui_servicename", serviceName );
    newAction->setProperty( "traingui_address", address.toString() );
    newAction->setProperty( "traingui_port", port );
    connect(newAction, &QAction::triggered,
            this, [newAction,this](){
        connectToLoconetServer(newAction);
    });
    ui->menu_loconet_connect_to->insertAction(before, newAction);
}

void MainWindow::loconetServerLeft(QString serviceName){
    // TODO remove from UI
}

void MainWindow::connectToLoconetServer(QAction* requestAction){
    requestAction->deleteLater();
    ui->menu_lcc_Connect_To->removeAction(requestAction);

    QHostAddress addr(requestAction->property("traingui_address").toString());
    uint16_t port = requestAction->property("traingui_port").toInt();
    std::shared_ptr<LoconetConnection> conn = m_state->loconetManager->createNewNetworkLoconet(QString(), addr, port);
    if(conn){
        QMenu* menu = ui->menuLoconet->addMenu(conn->name());
        addSubmenusLoconetConnection(menu, conn->name());
        newConnectionMade(conn);
    }
}

void MainWindow::addSubmenusLoconetConnection(QMenu* parentMenu, QString connectionName){
    QAction* actionTrafficMonitor = parentMenu->addAction("Traffic Monitor");
    connect(actionTrafficMonitor, &QAction::triggered,
            [connectionName,this](){
        std::shared_ptr<LoconetConnection> loconetConn = m_state->loconetManager->getConnectionByName(connectionName);
        ads::CDockWidget* DockWidget = new ads::CDockWidget(QString("%1 - Traffic Monitor").arg(loconetConn->name()));
        LoconetTrafficMonitor* trafficMonitor = new LoconetTrafficMonitor(this);
        trafficMonitor->setLoconetConnection(loconetConn);
        DockWidget->setWidget(trafficMonitor);
        m_dockManager->addDockWidget(ads::TopDockWidgetArea, DockWidget);
    });

    QAction* actionSlotMonitor = parentMenu->addAction("Slot Monitor");
    connect(actionSlotMonitor, &QAction::triggered,
            [connectionName,this](){
        std::shared_ptr<LoconetConnection> loconetConn = m_state->loconetManager->getConnectionByName(connectionName);
        ads::CDockWidget* DockWidget = new ads::CDockWidget(QString("%1 - Slot Monitor").arg(loconetConn->name()));
        LoconetSlotMonitor* slotMonitor = new LoconetSlotMonitor(this);
        slotMonitor->setLoconetConnection(loconetConn);
        DockWidget->setWidget(slotMonitor);
        m_dockManager->addDockWidget(ads::TopDockWidgetArea, DockWidget);
    });
}

void MainWindow::scanForLoconetConnections(){
    // Remove all options in the list and re-populate
    QList<QAction*> toRemove;
    bool remove = false;
    for(QAction* action : ui->menu_loconet_connect_to->actions()){
        if(action->objectName().compare("action_loconet_serial_connections") == 0){
            remove = true;
            continue;
        }

        if(remove){
            toRemove.push_back(action);
        }
    }

    for(QAction* action : toRemove){
        ui->menu_loconet_connect_to->removeAction(action);
        action->deleteLater();
    }

    for(QString& str : m_state->loconetManager->getAvailableLocalSerialPortConnections()){
        QAction* newAction = ui->menu_loconet_connect_to->addAction(str);
        connect(newAction, &QAction::triggered,
                [newAction,this](){
            connectToLoconetSerial(newAction);
        });
    }
}

void MainWindow::connectToLoconetSerial(QAction* requestAction){
    std::shared_ptr<LoconetConnection> conn = m_state->loconetManager->createNewLocalLoconet(QString(), requestAction->text());
    if(conn){
        QMenu* menu = ui->menuLoconet->addMenu(conn->name());
        addSubmenusLoconetConnection(menu, conn->name());
        newConnectionMade(conn);
    }
}

void MainWindow::on_action_lcc_Manual_Serial_triggered()
{
    QList<QSerialPortInfo> allInfos = QSerialPortInfo::availablePorts();
    QStringList portNames;

    for(QSerialPortInfo inf : allInfos){
        portNames.push_back( inf.portName() );
    }

    QString serial = QInputDialog::getItem(this, "Select Serial Port", "Select LCC serial port", portNames );
    if(serial.isNull() || serial.isEmpty()){
        return;
    }

    std::shared_ptr<LCCConnection> conn = m_state->lccManager->createNewLocalLCC(QString(), serial);
    if(conn){
        QMenu* menu = ui->menuLCC->addMenu(conn->name());
        addSubmenusLCCConnection(menu, conn->name());
        newConnectionMade(conn);
    }
}

void MainWindow::newConnectionMade(std::shared_ptr<SystemConnection> conn){
    SystemConnectionStatusWidget* newStatusWidget = new SystemConnectionStatusWidget();
    newStatusWidget->setSystemConnection(conn);
    ui->statusbar->addPermanentWidget(newStatusWidget);
}

