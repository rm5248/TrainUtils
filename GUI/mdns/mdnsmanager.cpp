/* SPDX-License-Identifier: GPL-2.0 */
#include <log4cxx/logger.h>
#include <fmt/format.h>
#include <QTimer>

#include "mdnsmanager.h"

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui.MDNSManager" );


MDNSManager::MDNSManager(QObject *parent) : QObject(parent)
{
}

MDNSManager::~MDNSManager(){}
