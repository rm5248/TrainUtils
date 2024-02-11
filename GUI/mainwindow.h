/* SPDX-License-Identifier: GPL-2.0 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QHostAddress>

#include "qtadvanceddocking/DockManager.h"
#include "systemconnection.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

struct TrainUtilsState;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void setTrainUtilsState(TrainUtilsState* state);

private Q_SLOTS:
    void on_actionExit_triggered();

    void on_action_lcc_Manual_IP_triggered();

    void lccServerFound(QString serviceName, QHostAddress address, uint16_t port);
    void lccServerLeft(QString serviceName);
    void connectToLCC(QAction* requestAction);

    void loconetServerFound(QString serviceName, QHostAddress address, uint16_t port);
    void loconetServerLeft(QString serviceName);
    void connectToLoconetServer(QAction* requestAction);
    void connectToLoconetSerial(QAction* requestAction);

    void scanForLoconetConnections();

    void on_action_lcc_Manual_Serial_triggered();

    void on_action_loconet_manual_Serial_triggered();

private:
    void addSubmenusLCCConnection(QMenu* parentMenu, QString connectionName);
    void addSubmenusLoconetConnection(QMenu* parentMenu, QString connectionName);
    void newConnectionMade(std::shared_ptr<SystemConnection> conn);

private:
    Ui::MainWindow *ui;
    ads::CDockManager* m_dockManager;
    TrainUtilsState* m_state;
};
#endif // MAINWINDOW_H
