/* SPDX-License-Identifier: GPL-2.0 */
#include <QApplication>
#include <QTimer>
#include <QStandardPaths>
#include <QDir>

#include "mainwindow.h"
#include "trainutils_state.h"
#include "lcc/lccmanager.h"
#include "mdns/mdnsmanager.h"
#include "loconet/loconetmanager.h"

#include <log4cxx-qt/messagehandler.h>
#include <log4cxx-qt/configuration.h>
#include <log4cxx/logger.h>
#include <fmt/format.h>

#if defined(Q_OS_WIN)
#include "mdns/mdnswindows.h"
#elif defined(Q_OS_LINUX)
#include "mdns/mdnsavahi.h"
#endif

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui" );

static void initConfigFiles(TrainUtilsState* state){
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir dir(configDir);
    if(!dir.exists()){
        dir.mkpath(configDir);
    }

    QFileInfoList configFiles = dir.entryInfoList();
    for(QFileInfo inf : configFiles){
        if(inf.isDir()){
            continue;
        }
        QSettings settings(inf.absoluteFilePath(), QSettings::Format::IniFormat);

        LOG4CXX_DEBUG_FMT(logger, "Checking file {}", inf.absoluteFilePath().toStdString());
        if(settings.value("connection/name").isValid()){
            LOG4CXX_DEBUG_FMT(logger, "File is connection file");
            state->connectionINIFileNames.append(inf.fileName());
        }else{
            LOG4CXX_DEBUG_FMT(logger, "File is unknown type");
        }
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);    
    TrainUtilsState programState;

    log4cxx::qt::Configuration::configureFromFileAndWatch({}, {"log4cxx.xml"});

    qInstallMessageHandler(log4cxx::qt::messageHandler);

    LOG4CXX_INFO(logger, "Train GUI starting up" );

    // TODO rename to QtMRI?
    QCoreApplication::setApplicationName("TrainGUI");

    // config directory scanning
    initConfigFiles(&programState);

    LCCManager lccManager;
    LoconetManager loconetManager;

#if defined(Q_OS_WIN)
    MDNSManager* mdnsManager = new MDNSWindows();
#elif defined(Q_OS_LINUX)
    MDNSManager* mdnsManager = new MDNSAvahi();
#endif

    MainWindow w;
    w.show();

    programState.lccManager = &lccManager;
    programState.mdnsManager = mdnsManager;
    programState.loconetManager = &loconetManager;

    w.setTrainUtilsState(&programState);

    int ret = a.exec();
    delete mdnsManager;
    return ret;
}
