#ifndef PANELSTORAGE_H
#define PANELSTORAGE_H

#include <QString>
#include <QStringList>

#include "panelmodel.h"

struct TrainUtilsState;

/**
 * JSON save/load of a PanelModel, one file per panel name under
 * QStandardPaths::AppConfigLocation + "/panels/" -- kept separate from
 * SystemConnection::save()'s per-connection .ini files in the same base
 * directory, since a panel's node/segment arrays don't map cleanly onto
 * QSettings' flat key-value groups.
 *
 * Node positions/rotations/etc. always come from here, never from the node
 * editor's own settings blob (which imguipanelwidget.cpp keeps in memory
 * only, for exactly this reason -- see ensureEditor()'s Config::SaveSettings/
 * LoadSettings). A freshly loaded panel is left to the node editor's own
 * fit-to-content behavior for its initial pan/zoom rather than trying to
 * restore the exact camera position: every node position it depends on is
 * seeded from this file already, so the two are never in conflict, and
 * attempting to also serialize the camera would mean re-deriving it from that
 * same settings blob at exactly the right moment relative to CreateEditor() --
 * fragile, per the note already on ensureEditor(), for a feature panel-updates.txt
 * never actually asked for.
 */
namespace PanelStorage {

/** Every panel name with a saved file, for populating an "Open panel..." picker. */
QStringList savedPanelNames();

bool save(const PanelModel& model, const QString& panelName);

/**
 * Populates model (first clearing it) from the file for panelName. Rebinds
 * each turnout's Turnout by looking up its saved connection name in state's
 * connections and calling SystemConnection::getDCCTurnout() with its saved
 * address; a node whose connection is no longer open loads unbound
 * (decorative) rather than failing the whole load, exactly as an unbound
 * turnout dropped fresh onto a panel already behaves.
 */
bool load(PanelModel& model, const QString& panelName, TrainUtilsState* state);

} // namespace PanelStorage

#endif // PANELSTORAGE_H
