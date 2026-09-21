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
#include "lcceventtransmit.h"
#include "lccnetworkview.h"
#include "loconetswitchcontrol.h"
#include "loconetcvprogrammer.h"
#include "throttledisplay.h"
#include "loconet/loconetthrottle.h"
#include "lccmemorydisplay.h"
#include "lcc/cdi/cdieditorwidget.h"
#include "panels/imguipanelwidget.h"
#include "panels/panelstorage.h"
#include "systemconnection.h"
#include "speedo/speedoconnection.h"
#include "speedmatcher.h"
#include "speedometerdisplay.h"

#include <QInputDialog>
#include <QHostAddress>
#include <QSerialPortInfo>
#include <QErrorMessage>
#include <log4cxx/logger.h>
#include <fmt/format.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui.mainwindow" );


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ads::CDockManager::setAutoHideConfigFlags(ads::CDockManager::DefaultAutoHideConfig);
    m_dockManager = new ads::CDockManager(this);

    // Not in mainwindow.ui (no actionOpenPanel there to auto-connect to) --
    // added the same way the per-panel/per-connection submenus below are,
    // programmatically, since it only needs to exist once at the top level.
    QAction* openPanelAction = ui->menuPanels->addAction(tr("Open panel..."));
    connect(openPanelAction, &QAction::triggered, this, &MainWindow::onOpenPanelTriggered);

    connect(ui->menu_loconet_connect_to, &QMenu::aboutToShow,
            this, &MainWindow::scanForLoconetConnections);
    connect(ui->menu_speedo_connect_to, &QMenu::aboutToShow,
            this, &MainWindow::scanForSpeedoConnections);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setTrainUtilsState(TrainUtilsState* state){
    m_state = state;

    for(const ConnectionInfo& connInfo : m_state->connectionFiles){
        QString connectionFile = connInfo.connectionFileAbsolutePath;
        QAction* action = ui->menuConnect_To->addAction(connInfo.connectionName);
        connect(action, &QAction::triggered,
                [this,connectionFile](){
            std::shared_ptr<SystemConnection> conn = SystemConnection::createfromINI(connectionFile, m_state);
            if(!conn){
                return;
            }
            this->m_state->m_connections.append(conn);

            LoconetConnection* lnConnection = qobject_cast<LoconetConnection*>(conn.get());
            if(lnConnection){
                QMenu* menu = ui->menuLoconet->addMenu(conn->name());
                addSubmenusLoconetConnection(menu, conn->name());
            }
            newConnectionMade(conn);

            // TODO figure out the connection type, add the appropriate menus
        });
    }

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

    action = parentMenu->addAction("Send Event");
    connect(action, &QAction::triggered,
            [connectionName,this](){
        std::shared_ptr<LCCConnection> lccConn = m_state->lccManager->getConnectionByName(connectionName);
        ads::CDockWidget* DockWidget = new ads::CDockWidget(QString("%1 - Event Transmit").arg(lccConn->name()));
        LCCEventTransmit* eventTransmit = new LCCEventTransmit(this);
        eventTransmit->setLCCConnection(lccConn);
        DockWidget->setWidget(eventTransmit);
        m_dockManager->addDockWidget(ads::TopDockWidgetArea, DockWidget);
    });

    action = parentMenu->addAction("Network View");
    connect(action, &QAction::triggered,
            [connectionName,this](){
        std::shared_ptr<LCCConnection> lccConn = m_state->lccManager->getConnectionByName(connectionName);
        ads::CDockWidget* DockWidget = new ads::CDockWidget(QString("%1 - Network").arg(lccConn->name()));
        LCCNetworkView* networkView = new LCCNetworkView(this);
        networkView->setLCCConnection(lccConn);
        DockWidget->setWidget(networkView);
        m_dockManager->addDockWidget(ads::TopDockWidgetArea, DockWidget);
    });

    action = parentMenu->addAction("Memory Display");
    connect(action, &QAction::triggered,
            [connectionName,this](){
        std::shared_ptr<LCCConnection> lccConn = m_state->lccManager->getConnectionByName(connectionName);
        ads::CDockWidget* DockWidget = new ads::CDockWidget(QString("%1 - Memory Display").arg(lccConn->name()));
        lccmemorydisplay* memoryDisplay = new lccmemorydisplay(this);
        memoryDisplay->setLCCConnection(lccConn);
        DockWidget->setWidget(memoryDisplay);
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
    std::shared_ptr<LoconetConnection> loconetConn = m_state->loconetManager->getConnectionByName(connectionName);
    connect(loconetConn.get(), &SystemConnection::systemNameChanged,
            [loconetConn,parentMenu](){
        parentMenu->setTitle(loconetConn->name());
    });

    QAction* actionRename = parentMenu->addAction("Rename connection");
    connect(actionRename, &QAction::triggered,
        [connectionName,this](){
        std::shared_ptr<LoconetConnection> loconetConn = m_state->loconetManager->getConnectionByName(connectionName);
        bool ok = false;
        QString newName = QInputDialog::getText(this, "Input new connection name",
            "New connection name:",
            QLineEdit::Normal,
            loconetConn->name(),
            &ok);
        if(ok){
            loconetConn->setName(newName);
        }
    });

    QAction* actionSave = parentMenu->addAction("Save connection");
    connect(actionSave, &QAction::triggered,
        [connectionName,this](){
        std::shared_ptr<LoconetConnection> loconetConn = m_state->loconetManager->getConnectionByName(connectionName);
        loconetConn->save();
    });

    QAction* actionTrafficMonitor = parentMenu->addAction("Traffic Monitor");
    connect(actionTrafficMonitor, &QAction::triggered,
            [connectionName,this](){
        std::shared_ptr<LoconetConnection> loconetConn = m_state->loconetManager->getConnectionByName(connectionName);
        ads::CDockWidget* DockWidget = new ads::CDockWidget(m_dockManager,
                                                            QString("%1 - Traffic Monitor").arg(loconetConn->uuid().toString(QUuid::WithoutBraces)));
        QString newName = QString("%1 - Traffic Monitor").arg(loconetConn->name());
        DockWidget->setWindowTitle(newName);
        LoconetTrafficMonitor* trafficMonitor = new LoconetTrafficMonitor(this);
        trafficMonitor->setLoconetConnection(loconetConn);
        DockWidget->setWidget(trafficMonitor);
        m_dockManager->addDockWidget(ads::TopDockWidgetArea, DockWidget);

        connect(loconetConn.get(), &SystemConnection::systemNameChanged,
                [DockWidget,loconetConn](){
            QString newName = QString("%1 - Traffic Monitor").arg(loconetConn->name());
            DockWidget->setWindowTitle(newName);
        });
    });

    QAction* actionSlotMonitor = parentMenu->addAction("Slot Monitor");
    connect(actionSlotMonitor, &QAction::triggered,
            [connectionName,this](){
        std::shared_ptr<LoconetConnection> loconetConn = m_state->loconetManager->getConnectionByName(connectionName);
        ads::CDockWidget* DockWidget = new ads::CDockWidget(m_dockManager,
                                                            QString("%1 - Slot Monitor").arg(loconetConn->uuid().toString(QUuid::WithoutBraces)));
        QString newName = QString("%1 - Slot Monitor").arg(loconetConn->name());
        DockWidget->setWindowTitle(newName);
        LoconetSlotMonitor* slotMonitor = new LoconetSlotMonitor(this);
        slotMonitor->setLoconetConnection(loconetConn);
        DockWidget->setWidget(slotMonitor);
        m_dockManager->addDockWidget(ads::TopDockWidgetArea, DockWidget);

        connect(loconetConn.get(), &SystemConnection::systemNameChanged,
            [DockWidget,loconetConn](){
                QString newName = QString("%1 - Slot Monitor").arg(loconetConn->name());
                DockWidget->setWindowTitle(newName);
        });
    });

    QAction* actionSwitchControl = parentMenu->addAction("Switch Control");
    connect(actionSwitchControl, &QAction::triggered,
            [connectionName,this](){
        std::shared_ptr<LoconetConnection> loconetConn = m_state->loconetManager->getConnectionByName(connectionName);
        ads::CDockWidget* DockWidget = new ads::CDockWidget(m_dockManager,
                                                            QString("%1 - Switch Control").arg(loconetConn->uuid().toString(QUuid::WithoutBraces)));
        QString newName = QString("%1 - Switch Control").arg(loconetConn->name());
        DockWidget->setWindowTitle(newName);
        LoconetSwitchControl* switchControl = new LoconetSwitchControl(this);
        switchControl->setLoconetConnection(loconetConn);
        DockWidget->setWidget(switchControl);
        m_dockManager->addDockWidget(ads::TopDockWidgetArea, DockWidget);

        connect(loconetConn.get(), &SystemConnection::systemNameChanged,
            [DockWidget,loconetConn](){
                QString newName = QString("%1 - Switch Control").arg(loconetConn->name());
                DockWidget->setWindowTitle(newName);
        });
    });

    QAction* actionCvProgrammer = parentMenu->addAction("CV Programmer");
    connect(actionCvProgrammer, &QAction::triggered,
            [connectionName,this](){
        std::shared_ptr<LoconetConnection> loconetConn = m_state->loconetManager->getConnectionByName(connectionName);
        ads::CDockWidget* DockWidget = new ads::CDockWidget(m_dockManager,
                                                            QString("%1 - CV Programmer").arg(loconetConn->uuid().toString(QUuid::WithoutBraces)));
        QString newName = QString("%1 - CV Programmer").arg(loconetConn->name());
        DockWidget->setWindowTitle(newName);
        LoconetCvProgrammer* cvProgrammer = new LoconetCvProgrammer(this);
        cvProgrammer->setLoconetConnection(loconetConn);
        DockWidget->setWidget(cvProgrammer);
        m_dockManager->addDockWidget(ads::TopDockWidgetArea, DockWidget);

        connect(loconetConn.get(), &SystemConnection::systemNameChanged,
            [DockWidget,loconetConn](){
                QString newName = QString("%1 - CV Programmer").arg(loconetConn->name());
                DockWidget->setWindowTitle(newName);
        });
    });

    QAction* actionNewThrottle = parentMenu->addAction("Throttle");
    connect(actionNewThrottle, &QAction::triggered,
            [connectionName,this](){
        std::shared_ptr<LoconetConnection> loconetConn = m_state->loconetManager->getConnectionByName(connectionName);
        ads::CDockWidget* DockWidget = new ads::CDockWidget(m_dockManager,
                                                            QString("%1 - Throttle").arg(loconetConn->uuid().toString(QUuid::WithoutBraces)));
        QString newName = QString("%1 - Throttle").arg(loconetConn->name());
        DockWidget->setWindowTitle(newName);
        ThrottleDisplay* throttleDisp = new ThrottleDisplay(this);
        std::shared_ptr<LoconetThrottle> throttle = loconetConn->newThrottle();
        throttleDisp->setThrottle(throttle);
        DockWidget->setWidget(throttleDisp);
        m_dockManager->addDockWidget(ads::TopDockWidgetArea, DockWidget);

        connect(loconetConn.get(), &SystemConnection::systemNameChanged,
            [DockWidget,loconetConn](){
                QString newName = QString("%1 - Throttle").arg(loconetConn->name());
                DockWidget->setWindowTitle(newName);
        });
    });
}

void MainWindow::addSubmenusSpeedoConnection(QMenu* parentMenu, QString connectionName){
    QAction* actionSpeedDisplay = parentMenu->addAction("Speed Display");
    connect(actionSpeedDisplay, &QAction::triggered,
            [connectionName,this](){
        std::shared_ptr<SystemConnection> systemConn = TrainUtils::connectionByName(m_state, connectionName);
        if(!systemConn){
            return;
        }
        std::shared_ptr<SpeedoConnection> speedoConn = qSharedPointerObjectCast<SpeedoConnection>(systemConn);
        if(!speedoConn){
            return;
        }
        ads::CDockWidget* DockWidget = new ads::CDockWidget(m_dockManager,
                                                            QString("%1 - Throttle").arg(speedoConn->uuid().toString(QUuid::WithoutBraces)));
        QString newName = QString("%1 - Speed Display").arg(speedoConn->name());
        DockWidget->setWindowTitle(newName);
        SpeedometerDisplay* speedoDisplay = new SpeedometerDisplay(this);
        DockWidget->setWidget(speedoDisplay);
        m_dockManager->addDockWidget(ads::TopDockWidgetArea, DockWidget);

        connect(speedoConn.get(), &SpeedoConnection::speedUpdated,
                speedoDisplay, &SpeedometerDisplay::incomingSpeedMeasurement);
        connect(speedoConn.get(), &SystemConnection::systemNameChanged,
            [DockWidget,speedoConn](){
                QString newName = QString("%1 - Speed Display").arg(speedoConn->name());
                DockWidget->setWindowTitle(newName);
        });
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

void MainWindow::scanForSpeedoConnections(){
    // Remove all options in the list and re-populate
    QList<QAction*> toRemove;
    bool remove = false;
    for(QAction* action : ui->menu_speedo_connect_to->actions()){
        if(action->objectName().compare("action_speedo_serial_connections") == 0){
            remove = true;
            continue;
        }

        if(remove){
            toRemove.push_back(action);
        }
    }

    for(QAction* action : toRemove){
        ui->menu_speedo_connect_to->removeAction(action);
        action->deleteLater();
    }

    for(QString& str : SpeedoConnection::getAvailableConnections()){
        QAction* newAction = ui->menu_speedo_connect_to->addAction(str);
        connect(newAction, &QAction::triggered,
                [newAction,this](){
            connectToSpeedo(newAction);
        });
    }
}

void MainWindow::connectToLoconetSerial(QAction* requestAction){
    std::shared_ptr<LoconetConnection> conn = m_state->loconetManager->createNewLocalLoconet(QString(), requestAction->text());
    if(conn && conn->isConnected()){
        QMenu* menu = ui->menuLoconet->addMenu(conn->name());
        addSubmenusLoconetConnection(menu, conn->name());
        newConnectionMade(conn);
    }else{
        QErrorMessage* msg = new QErrorMessage(this);
        QString errorMsg = QString("Unable to open serial port: %1")
                               .arg(conn->errorString());
        msg->showMessage(errorMsg);
        connect(msg, &QErrorMessage::finished,
                [msg](int){
                    msg->deleteLater();
        });
    }
}

void MainWindow::connectToSpeedo(QAction *requestAction){
    std::shared_ptr<SpeedoConnection> conn = std::make_shared<SpeedoConnection>();

//    conn->setName(connectionName);
    conn->setSerialPortName(requestAction->text());
    conn->open();
    if(conn && conn->isConnected()){
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
    m_state->m_connections.push_back(conn);
}


void MainWindow::on_action_loconet_manual_Serial_triggered()
{
    QList<QSerialPortInfo> allInfos = QSerialPortInfo::availablePorts();
    QStringList portNames;

    for(QSerialPortInfo inf : allInfos){
        portNames.push_back( inf.portName() );
    }

    QString serial = QInputDialog::getItem(this, "Select Serial Port", "Select Loconet serial port", portNames );
    if(serial.isNull() || serial.isEmpty()){
        return;
    }

    std::shared_ptr<LoconetConnection> conn = m_state->loconetManager->createNewLocalLoconet(QString(), serial);
    if(conn){
        QMenu* menu = ui->menuLoconet->addMenu(conn->name());
        addSubmenusLoconetConnection(menu, conn->name());
        newConnectionMade(conn);
    }
}


void MainWindow::on_actionNewPanel_triggered()
{
    ads::CDockWidget* dockWidget = new ads::CDockWidget("Panel");
    ImguiPanelWidget* panel = new ImguiPanelWidget(m_state);
    dockWidget->setWidget(panel);
    m_dockManager->addDockWidget(ads::TopDockWidgetArea, dockWidget);

    newPanelAdded(panel, dockWidget);
}

void MainWindow::on_actionCdiEditor_triggered()
{
    ads::CDockWidget* dockWidget = new ads::CDockWidget("CDI Editor");
    CdiEditorWidget* editor = new CdiEditorWidget(this);
    dockWidget->setWidget(editor);
    m_dockManager->addDockWidget(ads::TopDockWidgetArea, dockWidget);
}

void MainWindow::onOpenPanelTriggered()
{
    const QStringList names = PanelStorage::savedPanelNames();
    const QString name = QInputDialog::getItem(this, "Open Panel", "Select a panel to open", names, 0, false);
    if(name.isEmpty()){
        return;
    }

    ads::CDockWidget* dockWidget = new ads::CDockWidget("Panel");
    ImguiPanelWidget* panel = new ImguiPanelWidget(m_state);
    if(!panel->load(name)){
        LOG4CXX_ERROR_FMT(logger, "Could not load panel '{}'", name.toStdString());
    }
    dockWidget->setWindowTitle(panel->getName());
    dockWidget->setWidget(panel);
    m_dockManager->addDockWidget(ads::TopDockWidgetArea, dockWidget);

    newPanelAdded(panel, dockWidget);
}

void MainWindow::newPanelAdded(ImguiPanelWidget* panel, ads::CDockWidget* dockWidget){
    LOG4CXX_DEBUG_FMT(logger, "Added new panel");
    m_panels.push_back(panel);

    QMenu* menu = ui->menuPanels->addMenu(panel->getName());
    QAction* actionRename = menu->addAction("Rename panel");
    connect(actionRename, &QAction::triggered,
        [panel, menu, dockWidget, this](){
        QString newName = QInputDialog::getText(this, "New Name", "Input new name of panel", QLineEdit::Normal, panel->getName());
        if(newName.isEmpty()){
            return;
        }
        panel->setName(newName);
        menu->setTitle(newName);
        dockWidget->setWindowTitle(newName);
    });

    QAction* actionSave = menu->addAction("Save panel");
    connect(actionSave, &QAction::triggered,
        [panel, this](){
        if(!panel->save()){
            LOG4CXX_ERROR_FMT(logger, "Could not save panel '{}'", panel->getName().toStdString());
        }
    });

    QAction* actionSaveAs = menu->addAction("Save panel as...");
    connect(actionSaveAs, &QAction::triggered,
        [panel, menu, dockWidget, this](){
        QString newName = QInputDialog::getText(this, "Save As", "Input name to save panel as", QLineEdit::Normal, panel->getName());
        if(newName.isEmpty()){
            return;
        }
        if(panel->saveAs(newName)){
            menu->setTitle(newName);
            dockWidget->setWindowTitle(newName);
        }else{
            LOG4CXX_ERROR_FMT(logger, "Could not save panel as '{}'", newName.toStdString());
        }
    });
}


void MainWindow::on_action_speedo_Manual_Serial_triggered()
{
    QList<QSerialPortInfo> allInfos = QSerialPortInfo::availablePorts();
    QStringList portNames;

    for(QSerialPortInfo inf : allInfos){
        portNames.push_back( inf.portName() );
    }

    QString serial = QInputDialog::getItem(this, "Select Serial Port", "Select Speedometer serial port", portNames );
    if(serial.isNull() || serial.isEmpty()){
        return;
    }

    std::shared_ptr<SpeedoConnection> conn = std::shared_ptr<SpeedoConnection>(new SpeedoConnection());
    if(conn){
        conn->setName("Speedo");
        conn->setSerialPortName(serial);
        conn->open();
        QMenu* menu = ui->menuSpeedometer->addMenu(conn->name());
        addSubmenusSpeedoConnection(menu, conn->name());
        newConnectionMade(conn);
    }
}


void MainWindow::on_actionSpeed_Matching_triggered()
{
    ads::CDockWidget* DockWidget = new ads::CDockWidget(m_dockManager,
                                                        QString("Speed Matching"));
    DockWidget->setWindowTitle("Speed Matching");
    SpeedMatcher* speedMatch = new SpeedMatcher(m_state, this);
    DockWidget->setWidget(speedMatch);
    m_dockManager->addDockWidget(ads::TopDockWidgetArea, DockWidget);

}

