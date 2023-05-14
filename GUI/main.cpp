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

#if defined(Q_OS_WIN)
#include "mdns/mdnswindows.h"
#elif defined(Q_OS_LINUX)
#include "mdns/mdnsavahi.h"
#endif

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui" );

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    log4cxx::qt::Configuration::configureFromFileAndWatch({"/home/robert/TrainUtils/GUI"}, {"log4cxx.xml"});

    qInstallMessageHandler(log4cxx::qt::messageHandler);

    LOG4CXX_INFO(logger, "Train GUI starting up" );

    LCCManager lccManager;
    LoconetManager loconetManager;

#if defined(Q_OS_WIN)
    MDNSManager* mdnsManager = new MDNSWindows();
#elif defined(Q_OS_LINUX)
    MDNSManager* mdnsManager = new MDNSAvahi();
#endif

    MainWindow w;
    w.show();

    TrainUtilsState programState;
    programState.lccManager = &lccManager;
    programState.mdnsManager = mdnsManager;
    programState.loconetManager = &loconetManager;

    w.setTrainUtilsState(&programState);

    int ret = a.exec();
    delete mdnsManager;
    return ret;
}
