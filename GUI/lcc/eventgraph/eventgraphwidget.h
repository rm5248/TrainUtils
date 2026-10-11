/* SPDX-License-Identifier: GPL-2.0 */
#ifndef EVENTGRAPHWIDGET_H
#define EVENTGRAPHWIDGET_H

#include <QHash>
#include <QMap>
#include <QOpenGLWidget>
#include <QSet>
#include <QTimer>
#include <memory>
#include <string>

#include <imgui.h>

#include "QtImGui.h"
#include "eventgraphcontroller.h"
#include "eventgraphstorage.h"

namespace ax {
namespace NodeEditor {
struct EditorContext;
}
}

class LCCConnection;

/**
 * Shows the nodes on an LCC network as boxes, with a pin for every event ID in
 * their configuration, and lines between pins that have the same event ID.
 *
 * Drawn with Dear ImGui and imgui-node-editor, the same way as ImguiPanelWidget.
 */
class EventGraphWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit EventGraphWidget(std::shared_ptr<LCCConnection> conn, QWidget* parent = nullptr);
    ~EventGraphWidget() override;

protected:
    void initializeGL() override;
    void paintGL() override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    bool ensureEditor();
    void applyKeyboardModifiers();
    void drawFrame();
    void drawGrid(ImVec2 size);
    void drawNode(const EventGraphNode& node, const QSet<PinRef>& linkedPins);
    void drawPinMarker(bool filled, ImU32 color);
    void drawLinks(const QVector<EventLink>& links);
    void drawContextMenus();
    void drawToolbox(const struct ImGuiViewport* viewport);
    void drawHoveredLinkTooltip();
    void syncNodePositions();
    void saveLayout();

    uint64_t editorNodeId(uint64_t nodeId);
    uint64_t editorPinId(const PinRef& pin, bool output);
    uint64_t editorLinkId(const EventLink& link);
    QString nodeTitle(const EventGraphNode& node) const;

    std::unique_ptr<EventGraphController> m_controller;
    QString m_connectionName;

    QtImGui::RenderRef m_imgui = nullptr;
    ax::NodeEditor::EditorContext* m_editor = nullptr;
    QTimer m_repaintTimer;
    QTimer m_saveTimer;
    bool m_imguiConfigured = false;
    QSize m_lastSize;
    int m_stableSizeFrames = 0;
    int m_frame = 0;
    /** The node editor's own settings blob(pan and zoom) */
    std::string m_editorSettings;

    EventGraphLayout m_layout;
    /** Nodes that the node editor has been given a position for */
    QSet<uint64_t> m_seededNodes;
    /** Where new nodes without a saved position go next */
    int m_autoPlaceCount = 0;
    bool m_showUnconnected = false;

    /** Stable small numbers for the node editor's IDs */
    QHash<uint64_t, uint32_t> m_nodeIndex;
    QMap<PinRef, uint32_t> m_pinIndex;
    /** The links drawn this frame, by node editor link ID */
    QHash<uint64_t, EventLink> m_drawnLinks;
    /** The node that was right-clicked */
    uint64_t m_contextNodeId = 0;
};

#endif // EVENTGRAPHWIDGET_H
