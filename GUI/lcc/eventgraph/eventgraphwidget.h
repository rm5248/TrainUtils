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
    /**
     * A line in a node: either a group(which can be expanded) or a pin.
     */
    struct NodeRow {
        bool isGroup = false;
        int depth = 0;
        /** For a group row */
        const PinTreeGroup* group = nullptr;
        QString groupKey;
        bool expanded = false;
        /** For a pin row: index into EventGraphNode::pins */
        int pinIndex = -1;
        /** The pin matches the search, or the group has pins that do */
        bool matched = false;
    };

    /**
     * Summary of the pins in a group and all of its children
     */
    struct GroupInfo {
        int matches = 0;
        QVector<PinRef> linkedPins;
    };

    /**
     * Where the lines for a pin attach: the pin itself, or the header of the
     * collapsed group that it is in.
     */
    struct Anchor {
        uint64_t input = 0;
        uint64_t output = 0;
    };

    void updateCaches();
    void fillGroupInfo(uint64_t nodeId, const EventGraphNode& node, const PinTreeGroup& group);
    void collectRows(const EventGraphNode& node, const PinTreeGroup& group, int depth, QVector<NodeRow>& rows);
    QString groupKey(uint64_t nodeId, const QStringList& path) const;
    QString groupRowLabel(const NodeRow& row) const;
    void drawNode(const EventGraphNode& node);
    void drawPinMarker(bool filled, ImU32 color);
    void drawGroupToggle(const NodeRow& row);
    void drawLinks();
    void drawContextMenus();
    void drawToolbox(const struct ImGuiViewport* viewport);
    void drawHoveredLinkTooltip();
    void syncNodePositions();
    void saveLayout();

    uint64_t editorNodeId(uint64_t nodeId);
    uint64_t editorPinId(const PinRef& pin, bool output);
    uint64_t editorLinkId(const EventLink& link);
    uint64_t editorGroupPinId(const QString& key, bool output);
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
    bool m_onlyConnected = false;

    // Worked out again when the model or the search changes, rather than every frame
    int m_modelVersion = 0;
    int m_cacheVersion = -1;
    QString m_cachedSearch;
    QVector<EventLink> m_links;
    QSet<PinRef> m_linkedPins;
    QHash<uint64_t, QVector<PinTreeGroup>> m_pinTrees;
    QHash<QString, GroupInfo> m_groupInfo;
    QSet<PinRef> m_matches;
    /** Nodes that have pins matching the search, in the order they are drawn */
    QVector<uint64_t> m_matchingNodes;

    /** Groups that the user has expanded, by groupKey() */
    QSet<QString> m_expandedGroups;
    /** Where the lines for each pin attach this frame */
    QHash<PinRef, Anchor> m_anchors;
    QHash<QString, uint32_t> m_groupIndex;

    EventSearch m_search;
    char m_searchBuffer[128] = {0};
    /** Which of m_matchingNodes was navigated to last */
    int m_matchCursor = -1;
    /** Show this node on the next frame */
    uint64_t m_navigateToNode = 0;

    /** Stable small numbers for the node editor's IDs */
    QHash<uint64_t, uint32_t> m_nodeIndex;
    QMap<PinRef, uint32_t> m_pinIndex;
    /** The links drawn this frame, by node editor link ID */
    QHash<uint64_t, EventLink> m_drawnLinks;
    /** The node that was right-clicked */
    uint64_t m_contextNodeId = 0;
};

#endif // EVENTGRAPHWIDGET_H
