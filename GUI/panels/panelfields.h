#ifndef PANELFIELDS_H
#define PANELFIELDS_H

#include <functional>
#include <vector>

#include <QStringList>
#include <QVariant>

#include "panelmodel.h"

struct TrainUtilsState;

/**
 * One editable field in the properties panel: a label plus get/set closures
 * over whichever item is currently selected. Replaces QMetaProperty
 * reflection (the old QWidget panel's approach, which cannot survive the
 * move off QWidgets) with an explicit, per-type list built fresh each frame
 * by fieldsFor() below and rendered generically by renderPanelFields().
 */
struct PanelField {
    enum class Kind { Text, Double, Int, Enum };

    QString label;
    Kind kind = Kind::Text;
    std::function<QVariant()> get;
    std::function<void(const QVariant&)> set;
    QStringList enumLabels;      // Kind::Enum only
    double min = 0.0, max = 0.0; // Kind::Double/Int only
};

/**
 * Name/hand/rotation plus connection + DCC address binding. Setting either
 * of the last two re-resolves node.turnout via SystemConnection::getDCCTurnout()
 * (or clears it if no connection/address is chosen), which is what makes a
 * decorative, freshly-added turnout into one that actually throws over the bus.
 */
std::vector<PanelField> fieldsFor(TurnoutNode& node, TrainUtilsState* state);

/** Renders each field with the ImGui widget matching its Kind, calling set() on edit. */
void renderPanelFields(std::vector<PanelField>& fields);

#endif // PANELFIELDS_H
