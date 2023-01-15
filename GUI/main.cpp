/* SPDX-License-Identifier: GPL-2.0 */
#include <QApplication>
#include <QTimer>

#include "mainwindow.h"
#include "trainutils_state.h"
#include "lcc/lccmanager.h"

#include <dbus-cxx-qt.h>

#include <log4cxx-qt/messagehandler.h>
#include <log4cxx-qt/configuration.h>
#include <log4cxx/logger.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui" );

static void linux_dbus_setup(std::shared_ptr<DBus::Qt::QtDispatcher> disp){

}

static void dbus_log_function(const char* logger_name,
        const struct SL_LogLocation* location,
        const enum SL_LogLevel level,
        const char* log_string ){
    log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( logger_name );
    log4cxx::spi::LocationInfo inf( location->file,
                                    log4cxx::spi::LocationInfo::calcShortFileName(location->file),
                                    location->function,
                                    location->line_number);

    switch(level){
    case SL_TRACE:
        logger->trace(log_string, inf);
        break;
    case SL_DEBUG:
        logger->debug(log_string, inf);
        break;
    case SL_INFO:
        logger->info(log_string, inf);
        break;
    case SL_WARN:
        logger->warn(log_string, inf);
        break;
    case SL_ERROR:
        logger->error(log_string, inf);
        break;
    case SL_FATAL:
        logger->fatal(log_string, inf);
        break;
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    log4cxx::qt::Configuration::configureFromFileAndWatch({"/home/robert/TrainUtils/GUI"}, {"log4cxx.xml"});

    qInstallMessageHandler(log4cxx::qt::messageHandler);
    DBus::set_logging_function(dbus_log_function);

    LOG4CXX_INFO(logger, "Train GUI starting up" );

    std::shared_ptr<DBus::Qt::QtDispatcher> dispatcher = DBus::Qt::QtDispatcher::create();
    LCCManager lccManager;

    MainWindow w;
    w.show();

    TrainUtilsState programState;
    programState.dispatcher = dispatcher;
    programState.lccManager = &lccManager;

    w.setTrainUtilsState(&programState);

    QTimer::singleShot(0, [dispatcher](){
       linux_dbus_setup(dispatcher);
    });

    return a.exec();
}
