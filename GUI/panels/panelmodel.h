#ifndef PANELMODEL_H
#define PANELMODEL_H

#include <cstdint>
#include <memory>
#include <vector>

#include <imgui.h>
#include <QString>

class Turnout;
class PanelModel;

/**
 * Identifies an item on a panel (a turnout node, a track segment, ...),
 * independent of how imgui-node-editor numbers its own nodes/pins. Never 0.
 */
using PanelItemId = uint32_t;

enum class TurnoutHand {
    Left,
    Right
};

/**
 * Everything a turnout node needs to be drawn, saved and reloaded.
 * Framework-agnostic: no Qt widgets, no ImGui/node-editor calls.
 */
struct TurnoutNode {
    PanelItemId id = 0;
    QString name;
    TurnoutHand hand = TurnoutHand::Right;
    double rotationDegrees = 0.0;
    /** Canvas coordinates (node-editor space), node top-left. */
    ImVec2 position{0.0f, 0.0f};

    /** Which SystemConnection this is bound to, for rebinding on load. */
    QString connectionName;
    int dccAddress = 0;
    /** May be null: a turnout dropped on the panel with no connection/address
     *  chosen yet is decorative only, same as the old addBlankTurnout(). */
    std::shared_ptr<Turnout> turnout;
};

/**
 * Canonical turnout artwork and anchor-point geometry, shared by rendering
 * and hit-testing so they can never drift out of sync -- the QWidget
 * implementation this replaces computed the same numbers twice, by hand, in
 * TurnoutDisplay::paintEvent() and TurnoutDisplay::updateConnectionPoints().
 *
 * All coordinates are in the node's own local space: (0,0) is the top-left of
 * the node's fixed-size bounding box, matching where a turnout node's
 * ImGui::Dummy(kNodeSize, kNodeSize) item starts.
 */
struct TurnoutGeometry {
    // Three straight legs: the through route, the diverging diagonal, and the
    // diverging outgoing leg.
    ImVec2 legs[3][2];
    // Anchor/pin points: 0 = incoming, 1 = normal outgoing, 2 = diverged outgoing.
    ImVec2 pins[3];
};

/**
 * Size of the turnout artwork's content box (before the padding that lets it
 * rotate without clipping its bounding box -- see kNodeSize in
 * imguipanelwidget.h, which is the padded, actual node size).
 */
constexpr float kTurnoutContentSize = 50.0f;

TurnoutGeometry turnoutGeometry(TurnoutHand hand, double rotationDegrees);

/**
 * imgui-node-editor's default Style::NodePadding is (8,8,8,8) -- ed::BeginNode()
 * indents its content by (left,top) before anything is drawn, so a node's content
 * origin (what drawTurnoutNode() anchors pins to, via ImGui::GetItemRectMin()) sits
 * this far past node.position, not exactly on it. Duplicated here (rather than read
 * from ed::GetStyle(), which needs a current editor context this framework-agnostic
 * file has no access to) so resolveSegmentEnd() anchors segments to the same point
 * pins are actually drawn/hit-tested at -- matches kNodeSize's existing duplication
 * pattern in panelmodel.cpp.
 */
constexpr float kNodeContentPadding = 8.0f;

/**
 * One endpoint of a track segment: either a turnout's pin (nodeId != 0) or a
 * free-floating point with no connection (nodeId == 0). Mirrors
 * PanelDisplay::ConnectionEndpoint, but by id/index rather than by a raw
 * Connectable* pointer, since panel items are no longer widgets.
 */
struct SegmentEnd {
    PanelItemId nodeId = 0;
    /** 0 = incoming, 1 = normal outgoing, 2 = diverged outgoing. Ignored when nodeId == 0. */
    int pinIndex = -1;
    /** Only meaningful when nodeId == 0. Canvas coordinates. */
    ImVec2 freePos{0.0f, 0.0f};
};

/**
 * A cubic-bezier track segment between two endpoints. Framework-agnostic --
 * resolving an endpoint to its current canvas position (a turnout's pin
 * moves as the turnout is dragged/rotated) is done each frame by whoever
 * draws it; this struct only stores what was actually chosen/dragged.
 *
 * controlOffsetA/B and straight carry over verbatim from TrackSegment
 * (tracksegment.cpp) -- storing control points as offsets from their
 * governing endpoint is what makes a curve move rigidly with a rotating
 * turnout, and "straight" is what lets it re-track a moving endpoint as a
 * straight line until the user actually drags a control handle.
 */
struct TrackSegmentEdge {
    PanelItemId id = 0;
    SegmentEnd a, b;
    ImVec2 controlOffsetA{0.0f, 0.0f};
    ImVec2 controlOffsetB{0.0f, 0.0f};
    bool straight = true;
};

/**
 * Resolves a SegmentEnd to its current canvas-space position: the free point
 * if unconnected, or the referenced turnout's current pin position (which
 * moves as the turnout is dragged/rotated) if connected. Returns freePos
 * (0,0 by default) if the referenced node no longer exists.
 */
ImVec2 resolveSegmentEnd(const PanelModel& model, const SegmentEnd& end);

/**
 * Owns every item placed on a track panel. Framework-agnostic: no Qt widgets,
 * no ImGui/node-editor calls -- ImguiPanelWidget reads and writes this once
 * per frame.
 */
class PanelModel
{
public:
    TurnoutNode& addTurnout(ImVec2 position);
    TurnoutNode* findTurnout(PanelItemId id);
    /** Also removes any segment with an endpoint on this node. */
    void removeTurnout(PanelItemId id);

    const std::vector<TurnoutNode>& turnouts() const { return m_turnouts; }
    std::vector<TurnoutNode>& turnouts() { return m_turnouts; }

    TrackSegmentEdge& addSegment(SegmentEnd a, SegmentEnd b);
    TrackSegmentEdge* findSegment(PanelItemId id);
    void removeSegment(PanelItemId id);

    const std::vector<TrackSegmentEdge>& segments() const { return m_segments; }
    std::vector<TrackSegmentEdge>& segments() { return m_segments; }

private:
    PanelItemId m_nextId = 1;
    std::vector<TurnoutNode> m_turnouts;
    std::vector<TrackSegmentEdge> m_segments;
};

#endif // PANELMODEL_H
