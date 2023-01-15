#!/bin/bash

dbus-cxx-xml2cpp --xml xml-files/org.freedesktop.Avahi.Server.xml -f --proxy
dbus-cxx-xml2cpp --xml xml-files/org.freedesktop.Avahi.ServiceBrowser.xml -f --proxy
dbus-cxx-xml2cpp --xml xml-files/org.freedesktop.Avahi.ServiceResolver.xml -f --proxy
