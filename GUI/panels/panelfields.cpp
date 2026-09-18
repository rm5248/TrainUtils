#include <imgui.h>

#include "panelfields.h"
#include "../systemconnection.h"
#include "../trainutils_state.h"

namespace {

// Re-resolves node.turnout from whichever connection/address are currently
// stored on it. Called after either field changes so the two always agree --
// mirrors what SystemConnection::getDCCTurnout() itself does (idempotent,
// cached by switch number), so flipping back to a previous address/connection
// pair returns the same Turnout rather than creating a duplicate.
void rebindTurnout(TurnoutNode& node, TrainUtilsState* state){
    if(!state || node.connectionName.isEmpty() || node.dccAddress <= 0){
        node.turnout.reset();
        return;
    }
    std::shared_ptr<SystemConnection> conn = TrainUtils::connectionByName(state, node.connectionName);
    node.turnout = conn ? conn->getDCCTurnout(node.dccAddress) : nullptr;
}

} // namespace

std::vector<PanelField> fieldsFor(TurnoutNode& node, TrainUtilsState* state){
    std::vector<PanelField> fields;

    fields.push_back(PanelField{
        "Name", PanelField::Kind::Text,
        [&node](){ return QVariant(node.name); },
        [&node](const QVariant& v){ node.name = v.toString(); },
        {}, 0.0, 0.0,
    });

    fields.push_back(PanelField{
        "Hand", PanelField::Kind::Enum,
        [&node](){ return QVariant(node.hand == TurnoutHand::Left ? 0 : 1); },
        [&node](const QVariant& v){ node.hand = (v.toInt() == 0) ? TurnoutHand::Left : TurnoutHand::Right; },
        QStringList{"Left", "Right"}, 0.0, 0.0,
    });

    fields.push_back(PanelField{
        "Rotation", PanelField::Kind::Double,
        [&node](){ return QVariant(node.rotationDegrees); },
        [&node](const QVariant& v){ node.rotationDegrees = v.toDouble(); },
        {}, -360.0, 360.0,
    });

    // "(none)" plus every currently open connection's name -- rebuilt fresh
    // each frame (fieldsFor() is only ever called for the current frame's
    // properties panel), so a newly opened/closed connection shows up
    // immediately without this panel needing to know when that happens.
    QStringList connectionNames{"(none)"};
    if(state){
        for(const std::shared_ptr<SystemConnection>& conn : state->m_connections){
            connectionNames << conn->name();
        }
    }
    fields.push_back(PanelField{
        "Connection", PanelField::Kind::Enum,
        [&node, connectionNames](){
            const int idx = connectionNames.indexOf(node.connectionName);
            return QVariant(idx < 0 ? 0 : idx);
        },
        [&node, state, connectionNames](const QVariant& v){
            const int idx = v.toInt();
            node.connectionName = (idx <= 0) ? QString() : connectionNames[idx];
            rebindTurnout(node, state);
        },
        connectionNames, 0.0, 0.0,
    });

    fields.push_back(PanelField{
        "DCC Address", PanelField::Kind::Int,
        [&node](){ return QVariant(node.dccAddress); },
        [&node, state](const QVariant& v){
            node.dccAddress = v.toInt();
            rebindTurnout(node, state);
        },
        {}, 0.0, 2048.0,
    });

    return fields;
}

void renderPanelFields(std::vector<PanelField>& fields){
    constexpr float kLabelColumnWidth = 110.0f;

    for(PanelField& field : fields){
        ImGui::PushID(field.label.toUtf8().constData());
        ImGui::TextUnformatted(field.label.toUtf8().constData());
        ImGui::SameLine(kLabelColumnWidth);
        ImGui::SetNextItemWidth(140.0f);

        switch(field.kind){
        case PanelField::Kind::Text: {
            const std::string current = field.get().toString().toStdString();
            char buf[256];
            std::snprintf(buf, sizeof(buf), "%s", current.c_str());
            if(ImGui::InputText("##value", buf, sizeof(buf))){
                field.set(QVariant(QString::fromUtf8(buf)));
            }
            break;
        }
        case PanelField::Kind::Double: {
            float value = static_cast<float>(field.get().toDouble());
            if(ImGui::DragFloat("##value", &value, 1.0f,
                                 static_cast<float>(field.min), static_cast<float>(field.max), "%.1f")){
                field.set(QVariant(static_cast<double>(value)));
            }
            break;
        }
        case PanelField::Kind::Int: {
            int value = field.get().toInt();
            if(ImGui::DragInt("##value", &value, 1.0f,
                               static_cast<int>(field.min), static_cast<int>(field.max))){
                field.set(QVariant(value));
            }
            break;
        }
        case PanelField::Kind::Enum: {
            // ImGui::Combo needs the label strings to outlive the call, so copy
            // them into local storage rather than pointing at temporaries.
            std::vector<std::string> storage;
            storage.reserve(field.enumLabels.size());
            for(const QString& label : field.enumLabels){
                storage.push_back(label.toStdString());
            }
            std::vector<const char*> items;
            items.reserve(storage.size());
            for(const std::string& s : storage){
                items.push_back(s.c_str());
            }
            int current = field.get().toInt();
            if(ImGui::Combo("##value", &current, items.data(), static_cast<int>(items.size()))){
                field.set(QVariant(current));
            }
            break;
        }
        }

        ImGui::PopID();
    }
}
