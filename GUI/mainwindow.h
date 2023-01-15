/* SPDX-License-Identifier: GPL-2.0 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QHostAddress>

#include "DockManager.h"

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

private:
    void addSubmenusLCCConnection(QMenu* parentMenu, QString connectionName);

private:
    Ui::MainWindow *ui;
    ads::CDockManager* m_dockManager;
    TrainUtilsState* m_state;
};
#endif // MAINWINDOW_H
