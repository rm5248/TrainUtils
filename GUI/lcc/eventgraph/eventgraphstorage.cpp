/* SPDX-License-Identifier: GPL-2.0 */
#include "eventgraphstorage.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace {

QString graphsDir(){
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/eventgraphs";
}

QString filePathForName(const QString& connectionName){
    // Connection names can be things like host:port or /dev/ttyUSB0
    QString safeName = connectionName;
    for(QChar& c : safeName){
        if(!c.isLetterOrNumber() && c != '-' && c != '_' && c != '.'){
            c = '_';
        }
    }
    return graphsDir() + "/" + safeName + ".json";
}

QString directionToString(PinDirection direction){
    switch(direction){
    case PinDirection::Producer: return "producer";
    case PinDirection::Consumer: return "consumer";
    default: return "unknown";
    }
}

PinDirection directionFromString(const QString& str){
    if(str == "producer"){
        return PinDirection::Producer;
    }else if(str == "consumer"){
        return PinDirection::Consumer;
    }
    return PinDirection::Unknown;
}

}

QByteArray EventGraphStorage::toJson(const EventGraphLayout& layout){
    QJsonObject root;

    QJsonArray nodes;
    for(auto it = layout.positions.constBegin(); it != layout.positions.constEnd(); ++it){
        QJsonObject node;
        // Node IDs are 48 bits, which a double can hold exactly, but hex is easier to read
        node["id"] = QString::number(it.key(), 16);
        node["x"] = it.value().x();
        node["y"] = it.value().y();
        nodes.append(node);
    }
    root["nodes"] = nodes;

    QJsonArray models;
    for(auto modelIt = layout.learnedDirections.constBegin(); modelIt != layout.learnedDirections.constEnd(); ++modelIt){
        QJsonObject model;
        model["model"] = modelIt.key();
        QJsonObject fields;
        for(auto fieldIt = modelIt->constBegin(); fieldIt != modelIt->constEnd(); ++fieldIt){
            fields[fieldIt.key()] = directionToString(fieldIt.value());
        }
        model["directions"] = fields;
        models.append(model);
    }
    root["learned"] = models;

    return QJsonDocument(root).toJson();
}

EventGraphLayout EventGraphStorage::fromJson(const QByteArray& json){
    EventGraphLayout layout;
    QJsonObject root = QJsonDocument::fromJson(json).object();

    for(const QJsonValue& value : root["nodes"].toArray()){
        QJsonObject node = value.toObject();
        bool ok = false;
        uint64_t id = node["id"].toString().toULongLong(&ok, 16);
        if(ok){
            layout.positions[id] = QPointF(node["x"].toDouble(), node["y"].toDouble());
        }
    }

    for(const QJsonValue& value : root["learned"].toArray()){
        QJsonObject model = value.toObject();
        QJsonObject fields = model["directions"].toObject();
        QMap<QString, PinDirection>& directions = layout.learnedDirections[model["model"].toString()];
        for(auto it = fields.constBegin(); it != fields.constEnd(); ++it){
            PinDirection direction = directionFromString(it.value().toString());
            if(direction != PinDirection::Unknown){
                directions[it.key()] = direction;
            }
        }
    }

    return layout;
}

bool EventGraphStorage::save(const EventGraphLayout& layout, const QString& connectionName){
    QDir().mkpath(graphsDir());
    QFile file(filePathForName(connectionName));
    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate)){
        return false;
    }
    return file.write(toJson(layout)) >= 0;
}

EventGraphLayout EventGraphStorage::load(const QString& connectionName){
    QFile file(filePathForName(connectionName));
    if(!file.open(QIODevice::ReadOnly)){
        return EventGraphLayout();
    }
    return fromJson(file.readAll());
}
