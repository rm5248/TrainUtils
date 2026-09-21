#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

#include <log4cxx/logger.h>
#include <fmt/format.h>

#include "panelstorage.h"
#include "../systemconnection.h"
#include "../trainutils_state.h"

namespace {

log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("traingui.PanelStorage");

QString panelsDir(){
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/panels";
}

QString filePathForName(const QString& panelName){
    return panelsDir() + "/" + panelName + ".json";
}

QJsonObject pointToJson(ImVec2 p){
    QJsonObject obj;
    obj["x"] = static_cast<double>(p.x);
    obj["y"] = static_cast<double>(p.y);
    return obj;
}

ImVec2 pointFromJson(const QJsonObject& obj){
    return ImVec2(static_cast<float>(obj["x"].toDouble()),
                  static_cast<float>(obj["y"].toDouble()));
}

// A connected end saves as {"node": id, "pin": index}; a free end (not used
// by anything the panel can create yet, but part of the model) as {"x","y"}.
QJsonObject segmentEndToJson(const SegmentEnd& end){
    if(end.nodeId != 0){
        QJsonObject obj;
        obj["node"] = static_cast<qint64>(end.nodeId);
        obj["pin"] = end.pinIndex;
        return obj;
    }
    return pointToJson(end.freePos);
}

SegmentEnd segmentEndFromJson(const QJsonObject& obj){
    SegmentEnd end;
    if(obj.contains("node")){
        end.nodeId = static_cast<PanelItemId>(obj["node"].toInteger());
        end.pinIndex = obj["pin"].toInt();
    }else{
        end.freePos = pointFromJson(obj);
    }
    return end;
}

} // namespace

QStringList PanelStorage::savedPanelNames(){
    QStringList names;
    for(const QFileInfo& info : QDir(panelsDir()).entryInfoList({"*.json"}, QDir::Files, QDir::Name)){
        names << info.completeBaseName();
    }
    return names;
}

bool PanelStorage::save(const PanelModel& model, const QString& panelName){
    if(!QDir().mkpath(panelsDir())){
        LOG4CXX_ERROR_FMT(logger, "Could not create panels directory");
        return false;
    }

    QJsonObject root;
    root["version"] = 1;
    root["name"] = panelName;

    QJsonArray nodes;
    for(const TurnoutNode& node : model.turnouts()){
        QJsonObject obj;
        obj["id"] = static_cast<qint64>(node.id);
        obj["type"] = "turnout";
        obj["name"] = node.name;
        obj["hand"] = (node.hand == TurnoutHand::Left) ? "left" : "right";
        obj["rotation"] = node.rotationDegrees;
        obj["x"] = static_cast<double>(node.position.x);
        obj["y"] = static_cast<double>(node.position.y);
        obj["connection"] = node.connectionName;
        obj["address"] = node.dccAddress;
        nodes.append(obj);
    }
    root["nodes"] = nodes;

    QJsonArray segments;
    for(const TrackSegmentEdge& seg : model.segments()){
        QJsonObject obj;
        obj["id"] = static_cast<qint64>(seg.id);
        obj["a"] = segmentEndToJson(seg.a);
        obj["b"] = segmentEndToJson(seg.b);
        obj["ca"] = pointToJson(seg.controlOffsetA);
        obj["cb"] = pointToJson(seg.controlOffsetB);
        obj["straight"] = seg.straight;
        segments.append(obj);
    }
    root["segments"] = segments;

    QFile file(filePathForName(panelName));
    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate)){
        LOG4CXX_ERROR_FMT(logger, "Could not open {} for writing", filePathForName(panelName).toStdString());
        return false;
    }
    file.write(QJsonDocument(root).toJson());
    LOG4CXX_DEBUG_FMT(logger, "Saved panel '{}' ({} nodes, {} segments)",
                       panelName.toStdString(), nodes.size(), segments.size());
    return true;
}

bool PanelStorage::load(PanelModel& model, const QString& panelName, TrainUtilsState* state){
    QFile file(filePathForName(panelName));
    if(!file.open(QIODevice::ReadOnly)){
        LOG4CXX_ERROR_FMT(logger, "Could not open {} for reading", filePathForName(panelName).toStdString());
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if(parseError.error != QJsonParseError::NoError || !doc.isObject()){
        LOG4CXX_ERROR_FMT(logger, "Could not parse {}: {}",
                           filePathForName(panelName).toStdString(), parseError.errorString().toStdString());
        return false;
    }
    const QJsonObject root = doc.object();

    model.clear();

    for(const QJsonValue& v : root["nodes"].toArray()){
        const QJsonObject obj = v.toObject();
        const PanelItemId id = static_cast<PanelItemId>(obj["id"].toInteger());
        const ImVec2 position(static_cast<float>(obj["x"].toDouble()),
                               static_cast<float>(obj["y"].toDouble()));

        TurnoutNode& node = model.addTurnoutWithId(id, position);
        node.name = obj["name"].toString();
        node.hand = (obj["hand"].toString() == "left") ? TurnoutHand::Left : TurnoutHand::Right;
        node.rotationDegrees = obj["rotation"].toDouble();
        node.connectionName = obj["connection"].toString();
        node.dccAddress = obj["address"].toInt();

        // Leave unbound (decorative) rather than fail the load if the saved
        // connection isn't open -- same fallback resolveSegmentEnd() and
        // rebindTurnout() already use for a missing node/connection.
        if(state && !node.connectionName.isEmpty() && node.dccAddress > 0){
            std::shared_ptr<SystemConnection> conn = TrainUtils::connectionByName(state, node.connectionName);
            if(conn){
                node.turnout = conn->getDCCTurnout(node.dccAddress);
            }
        }
    }

    for(const QJsonValue& v : root["segments"].toArray()){
        const QJsonObject obj = v.toObject();
        const PanelItemId id = static_cast<PanelItemId>(obj["id"].toInteger());

        TrackSegmentEdge& seg = model.addSegmentWithId(id,
            segmentEndFromJson(obj["a"].toObject()),
            segmentEndFromJson(obj["b"].toObject()));
        seg.controlOffsetA = pointFromJson(obj["ca"].toObject());
        seg.controlOffsetB = pointFromJson(obj["cb"].toObject());
        seg.straight = obj["straight"].toBool();
    }

    LOG4CXX_DEBUG_FMT(logger, "Loaded panel '{}' ({} nodes, {} segments)",
                       panelName.toStdString(), model.turnouts().size(), model.segments().size());
    return true;
}
