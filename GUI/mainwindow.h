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
class ImguiPanelWidget;

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
    void connectToSpeedo(QAction* requestAction);

    void scanForLoconetConnections();
    void scanForSpeedoConnections();

    void on_action_lcc_Manual_Serial_triggered();

    void on_action_loconet_manual_Serial_triggered();

    void on_actionNewPanel_triggered();
    void onOpenPanelTriggered();

    void on_actionCdiEditor_triggered();

    void on_action_speedo_Manual_Serial_triggered();

    void on_actionSpeed_Matching_triggered();

private:
    void addSubmenusLCCConnection(QMenu* parentMenu, QString connectionName);
    void addSubmenusLoconetConnection(QMenu* parentMenu, QString connectionName);
    void addSubmenusSpeedoConnection(QMenu* parentMenu, QString connectionName);
    void newConnectionMade(std::shared_ptr<SystemConnection> conn);
    void newPanelAdded(ImguiPanelWidget* panel, ads::CDockWidget* dockWidget);

private:
    Ui::MainWindow *ui;
    ads::CDockManager* m_dockManager;
    TrainUtilsState* m_state;
    QVector<ImguiPanelWidget*> m_panels;
};
#endif // MAINWINDOW_H
