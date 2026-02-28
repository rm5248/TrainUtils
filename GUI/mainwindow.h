/* SPDX-License-Identifier: GPL-2.0 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QHostAddress>

#include "qtadvanceddocking-qt6/DockManager.h"
#include "systemconnection.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

struct TrainUtilsState;
class PanelDisplay;
class PanelToolsWidget;

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

    void on_actionNewPanel_triggered();

private:
    void addSubmenusLCCConnection(QMenu* parentMenu, QString connectionName);
    void addSubmenusLoconetConnection(QMenu* parentMenu, QString connectionName);
    void newConnectionMade(std::shared_ptr<SystemConnection> conn);
    void newPanelAdded(PanelDisplay* panel);

private:
    Ui::MainWindow *ui;
    ads::CDockManager* m_dockManager;
    TrainUtilsState* m_state;
    QVector<PanelDisplay*> m_panels;

    // panel tools
    ads::CDockWidget* m_panelToolboxWidget;
    PanelToolsWidget* m_panelTools;
};
#endif // MAINWINDOW_H
