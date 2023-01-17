/* SPDX-License-Identifier: GPL-2.0 */
#include <QApplication>
#include <QTimer>

#include "mainwindow.h"
#include "trainutils_state.h"
#include "lcc/lccmanager.h"
#include "mdns/mdnsmanager.h"
#include "loconet/loconetmanager.h"

#include <log4cxx-qt/messagehandler.h>
#include <log4cxx-qt/configuration.h>
#include <log4cxx/logger.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui" );

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    log4cxx::qt::Configuration::configureFromFileAndWatch({"/home/robert/train-stuff/TrainUtils/GUI"}, {"log4cxx.xml"});

    qInstallMessageHandler(log4cxx::qt::messageHandler);

    LOG4CXX_INFO(logger, "Train GUI starting up" );

    LCCManager lccManager;
    MDNSManager mdnsManager;
    LoconetManager loconetManager;

    MainWindow w;
    w.show();

    TrainUtilsState programState;
    programState.lccManager = &lccManager;
    programState.mdnsManager = &mdnsManager;
    programState.loconetManager = &loconetManager;

    w.setTrainUtilsState(&programState);

    return a.exec();
}
