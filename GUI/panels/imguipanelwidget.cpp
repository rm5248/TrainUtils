#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

#include <QGuiApplication>
#include <QHideEvent>
#include <QShowEvent>

#include <imgui.h>
#include <imgui-node-editor/imgui_node_editor.h>

#include <log4cxx/logger.h>
#include <fmt/format.h>

#include "imguipanelwidget.h"
#include "panelfields.h"
#include "../common/turnout.h"

namespace ed = ax::NodeEditor;

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("traingui.ImguiPanelWidget");

namespace {

// imgui-node-editor requires non-zero, globally-unique node/pin ids. Each
// panel item gets a block of ids: one for its node, one per pin.
constexpr uint64_t kNodeEditorIdStride = 8;

uint64_t nodeEditorNodeIdValue(PanelItemId id){
    return static_cast<uint64_t>(id) * kNodeEditorIdStride;
}

uint64_t nodeEditorPinIdValue(PanelItemId id, int pinIndex){
    return static_cast<uint64_t>(id) * kNodeEditorIdStride + 1 + static_cast<uint64_t>(pinIndex);
}

// Inverse of nodeEditorPinIdValue(): recovers which node/pin a hovered/hit
// ed::PinId refers to. Valid because 1+pinIndex (1..3) is always < the stride.
PanelItemId nodeIdFromPinIdValue(uint64_t pinIdValue){
    return static_cast<PanelItemId>(pinIdValue / kNodeEditorIdStride);
}

int pinIndexFromPinIdValue(uint64_t pinIdValue){
    return static_cast<int>(pinIdValue % kNodeEditorIdStride) - 1;
}

// Inverse of nodeEditorNodeIdValue().
PanelItemId panelItemIdFromNodeIdValue(uint64_t nodeIdValue){
    return static_cast<PanelItemId>(nodeIdValue / kNodeEditorIdStride);
}

// Point on a cubic bezier at parameter t, matching TrackSegment::bezierPoint().
ImVec2 cubicBezierPoint(ImVec2 a, ImVec2 ca, ImVec2 cb, ImVec2 b, float t){
    const float u = 1.0f - t;
    const float uu = u * u;
    const float tt = t * t;
    return ImVec2(
        uu * u * a.x + 3.0f * uu * t * ca.x + 3.0f * u * tt * cb.x + tt * t * b.x,
        uu * u * a.y + 3.0f * uu * t * ca.y + 3.0f * u * tt * cb.y + tt * t * b.y);
}

// Shortest distance from p to the line segment a-b, matching TrackSegment::distanceToSegment().
float distanceToLineSegment(ImVec2 p, ImVec2 a, ImVec2 b){
    const ImVec2 pa(p.x - a.x, p.y - a.y);
    const ImVec2 ba(b.x - a.x, b.y - a.y);
    const float denom = ba.x * ba.x + ba.y * ba.y;
    if(denom == 0.0f){
        return std::sqrt(pa.x * pa.x + pa.y * pa.y);
    }
    const float t = std::clamp((pa.x * ba.x + pa.y * ba.y) / denom, 0.0f, 1.0f);
    const float dx = p.x - (a.x + t * ba.x);
    const float dy = p.y - (a.y + t * ba.y);
    return std::sqrt(dx * dx + dy * dy);
}

} // namespace

// Redraw interval.  Immediate mode plus the node editor's own animations
// (selection, navigation easing) make update-on-input unreliable, so just run a
// steady frame loop while the panel is actually visible.
static constexpr int kRepaintIntervalMs = 16;

// The node editor fits the view to its content on the first frame, before node
// sizes have been measured, which leaves a brand new panel at a nonsense zoom.
// Re-fit once the content has settled.
static constexpr int kSettleFrames = 3;

ImguiPanelWidget::ImguiPanelWidget(TrainUtilsState* state, QWidget* parent)
    : QOpenGLWidget{parent}
    , m_state(state)
{
    m_name = "Panel";

    // Needed so key events reach qtimgui's event filter.
    setFocusPolicy(Qt::StrongFocus);

    m_repaintTimer.setInterval(kRepaintIntervalMs);
    connect(&m_repaintTimer, &QTimer::timeout,
            this, QOverload<>::of(&ImguiPanelWidget::update));
}

ImguiPanelWidget::~ImguiPanelWidget()
{
    makeCurrent();
    if(m_editor){
        ed::DestroyEditor(m_editor);
        m_editor = nullptr;
    }
    // NOTE: qtimgui offers no teardown for a RenderRef, so the ImGuiRenderer and
    // its ImGui context outlive this widget.  Panels are long-lived, so this is
    // tolerable for now; fixing it means adding a destroy() to GUI/qtimgui.
    doneCurrent();
}

void ImguiPanelWidget::initializeGL()
{
    // defaultRender must be false: true would share one process-wide renderer
    // (and one ImGui context) between every panel.
    m_imgui = QtImGui::initialize(this, false);

    LOG4CXX_DEBUG_FMT(logger, "Initialized imgui panel '{}'", m_name.toStdString());
}

bool ImguiPanelWidget::ensureEditor()
{
    if(m_editor){
        return true;
    }

    // The node editor fits its view to its content on its very first frame and
    // keeps the result.  The dock has not been laid out on our first paint --
    // the widget is around 200x70 there -- so creating the editor that early
    // leaves a brand new panel sitting at roughly an 8x zoom.  Wait until the
    // widget size has settled before the editor ever sees a frame.
    const QSize current = size();
    if(current != m_lastSize){
        m_lastSize = current;
        m_stableSizeFrames = 0;
        return false;
    }
    if(++m_stableSizeFrames < kStableSizeFrames){
        return false;
    }

    ed::Config config;
    // Keep the editor's own settings (node sizes, pan and zoom) in memory rather
    // than letting it write NodeEditor.json next to the binary.  Node positions
    // are owned by the panel itself, so nothing here needs to outlive the panel.
    config.SettingsFile = nullptr;
    config.UserPointer = this;
    config.SaveSettings = [](const char* data, size_t size,
                             ed::SaveReasonFlags, void* userPointer) {
        auto* self = static_cast<ImguiPanelWidget*>(userPointer);
        self->m_editorSettings.assign(data, size);
        return true;
    };
    config.LoadSettings = [](char* data, void* userPointer) {
        auto* self = static_cast<ImguiPanelWidget*>(userPointer);
        if(data){
            memcpy(data, self->m_editorSettings.data(), self->m_editorSettings.size());
        }
        return self->m_editorSettings.size();
    };
    m_editor = ed::CreateEditor(&config);

    // WORKAROUND: imgui-node-editor 0.9.3's background/grid draw commands
    // (imgui_node_editor.cpp, "Draw grid") end up with an ImDrawCmd::ClipRect
    // that never receives the fixup ImGuiEx::Canvas::LeaveLocalSpace() applies
    // to everything else drawn inside ed::Begin/End -- confirmed by dumping the
    // raw ClipRect values: after panning by (dx, 0) the grid's clip rect sits at
    // (correct.x - dx, ...) instead of (correct.x, ...), i.e. it's simply
    // missing the pan offset. At zoom 1 a widget positioned at the window
    // origin masks this (the missing offset is ~0); at any other pan or zoom it
    // leaves a stale, wrongly-positioned/sized rectangle on screen -- reported
    // as "the viewport moves opposite to the mouse" and reproduced independent
    // of DPI. Nodes, links and our own draw-list content are unaffected (their
    // clip rects were confirmed correct in the same dump).
    //
    // Rather than patch the vendored library, make ed::'s own background/grid
    // fully transparent so its broken clip rect never has anything visible to
    // clip, and draw our own background + grid instead, directly on the
    // window's draw list, which -- like the placeholder bezier below -- is
    // confirmed to pan and zoom correctly.
    ed::SetCurrentEditor(m_editor);
    ed::Style& style = ed::GetStyle();
    style.Colors[ed::StyleColor_Bg]   = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ed::StyleColor_Grid] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ed::SetCurrentEditor(nullptr);

    return true;
}

void ImguiPanelWidget::showEvent(QShowEvent* event)
{
    QOpenGLWidget::showEvent(event);
    m_repaintTimer.start();
}

void ImguiPanelWidget::hideEvent(QHideEvent* event)
{
    m_repaintTimer.stop();
    QOpenGLWidget::hideEvent(event);
}

void ImguiPanelWidget::applyKeyboardModifiers()
{
    const Qt::KeyboardModifiers mods = QGuiApplication::keyboardModifiers();
    ImGuiIO& io = ImGui::GetIO();

    const bool ctrl  = mods.testFlag(Qt::ControlModifier);
    const bool shift = mods.testFlag(Qt::ShiftModifier);
    const bool alt   = mods.testFlag(Qt::AltModifier);
    const bool super = mods.testFlag(Qt::MetaModifier);

    // Direct assignment takes effect for the frame NewFrame() just started.
    io.KeyCtrl  = ctrl;
    io.KeyShift = shift;
    io.KeyAlt   = alt;
    io.KeySuper = super;

    // ...and queue real key events so ImGui's own key state stays consistent.
    io.AddKeyEvent(ImGuiMod_Ctrl, ctrl);
    io.AddKeyEvent(ImGuiMod_Shift, shift);
    io.AddKeyEvent(ImGuiMod_Alt, alt);
    io.AddKeyEvent(ImGuiMod_Super, super);
}

void ImguiPanelWidget::paintGL()
{
    QtImGui::newFrame(m_imgui);

    if(!m_imguiConfigured){
        // Our ImGui context is current from here on; stop it writing imgui.ini
        // into whatever the working directory happens to be.
        ImGui::GetIO().IniFilename = nullptr;
        m_imguiConfigured = true;
    }

    applyKeyboardModifiers();

    drawFrame();

    ImGui::Render();
    QtImGui::render(m_imgui);
}

void ImguiPanelWidget::drawFrame()
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("##panelroot", nullptr,
                 ImGuiWindowFlags_NoDecoration
                     | ImGuiWindowFlags_NoMove
                     | ImGuiWindowFlags_NoBringToFrontOnFocus
                     | ImGuiWindowFlags_NoNavFocus
                     | ImGuiWindowFlags_NoSavedSettings);

    m_dbgAvail = ImGui::GetContentRegionAvail();

    if(!ensureEditor()){
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }

    ed::SetCurrentEditor(m_editor);

    // Must run before ed::Begin(): see the comment on drawGrid() for why.
    drawGrid();

    ed::Begin("TrackPanel", ImVec2(0.0f, 0.0f));

    drawCanvas();

    m_frame++;

    ed::End();

    // Needs the editor current (GetNodePosition/SetNodePosition), but must run
    // after ed::End() -- not before -- so it sees where the user actually
    // dropped a node this frame, not last frame's position.
    syncTurnoutTransforms();
    handleOperateModeClicks();
    if(m_mode == PanelMode::Edit){
        handleSegmentSelection();
        handleSegmentDrawingClicks();
    }

    ed::SetCurrentEditor(nullptr);

    ImGui::End();
    ImGui::PopStyleVar();

    drawToolbox(viewport);
    drawDebugWindow(viewport);
}

void ImguiPanelWidget::drawGrid()
{
    // Our own replacement for ed::'s built-in background/grid -- see the
    // WORKAROUND comment in ensureEditor() for why.
    //
    // Must be called BEFORE ed::Begin(), not just outside ed::'s node/pin
    // drawing: ed::Begin() itself is what leaves the window's draw list
    // mid-channel-split (it draws its own, now-invisible, grid on a channel it
    // never switches back off of before returning). Any of our own drawing
    // done after ed::Begin() returns, even before the first node, inherits
    // that same channel and the same broken clip rect -- confirmed by this
    // exact bug still reproducing when this function was called first thing
    // inside drawCanvas() instead. Calling it here, before the window's draw
    // list has been split into channels at all, avoids the whole mechanism:
    // this is plain, unsplit ImGui drawing in screen space, using
    // ed::ScreenToCanvas()/CanvasToScreen() (confirmed correct -- they're what
    // the debug window's own readouts use) to place a grid that still tracks
    // panning and zoom. Being the first thing drawn on the window's draw list
    // this frame, it renders behind everything ed::Begin()/drawCanvas() add
    // afterward.
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(ImVec2(0.0f, 0.0f), m_dbgAvail, IM_COL32(38, 38, 45, 255));

    const ImVec2 topLeft = ed::ScreenToCanvas(ImVec2(0.0f, 0.0f));
    const ImVec2 bottomRight = ed::ScreenToCanvas(m_dbgAvail);

    constexpr float kGridStep = 32.0f;
    const ImU32 gridColor = IM_COL32(120, 120, 120, 40);
    for(float x = fmodf(topLeft.x, kGridStep); x < bottomRight.x; x += kGridStep){
        const ImVec2 top = ed::CanvasToScreen(ImVec2(x, topLeft.y));
        const ImVec2 bottom = ed::CanvasToScreen(ImVec2(x, bottomRight.y));
        drawList->AddLine(top, bottom, gridColor);
    }
    for(float y = fmodf(topLeft.y, kGridStep); y < bottomRight.y; y += kGridStep){
        const ImVec2 left = ed::CanvasToScreen(ImVec2(topLeft.x, y));
        const ImVec2 right = ed::CanvasToScreen(ImVec2(bottomRight.x, y));
        drawList->AddLine(left, right, gridColor);
    }
}

void ImguiPanelWidget::drawCanvas()
{
    // ed::GetNodeBackgroundDrawList() must be called AFTER ed::EndNode() --
    // calling it between BeginNode/EndNode trips an assertion inside
    // ImDrawListSplitter::SetCurrentChannel (see drawTurnoutNode()).
    for(TurnoutNode& node : m_model.turnouts()){
        drawTurnoutNode(node);
    }

    drawSegments();
    drawSegmentPreview();

    // ed::BeginDelete()/EndDelete() and the right-click "insert node" popup
    // (ed::ShowBackgroundContextMenu()) must run before ed::End(), per the
    // node-editor's own examples -- unlike GetHoveredNode()/GetHoveredPin(),
    // which tolerate either side of End() (handleOperateModeClicks() and
    // handleSegmentDrawingClicks() call them after; see the note there for
    // why "after" is in fact required for anything driven by
    // ed::IsBackgroundClicked()/ed::GetBackgroundClickButtonIndex()).
    if(m_mode == PanelMode::Edit){
        handleSegmentInsertMenu();
        handleDelete();
    }
}

void ImguiPanelWidget::drawTurnoutNode(TurnoutNode& node)
{
    const ed::NodeId nodeId(nodeEditorNodeIdValue(node.id));

    // Only seed the node editor's own position the first time it sees this
    // node; afterward, position is owned by the user dragging it (read back
    // in syncTurnoutTransforms()), not re-pushed from the model every frame.
    if(m_seededNodes.insert(node.id).second){
        ed::SetNodePosition(nodeId, node.position);
    }

    ed::BeginNode(nodeId);
    ImGui::Dummy(ImVec2(kNodeSize, kNodeSize));
    // GetItemRectMin()/Max() return canvas-space coordinates while inside
    // ed::Begin/End (confirmed in Phase 1), not screen coordinates.
    const ImVec2 origin = ImGui::GetItemRectMin();

    const TurnoutGeometry geo = turnoutGeometry(node.hand, node.rotationDegrees);

    for(int i = 0; i < 3; i++){
        const ImVec2 pinCenter(origin.x + geo.pins[i].x, origin.y + geo.pins[i].y);
        ImGui::SetCursorScreenPos(ImVec2(pinCenter.x - 4.0f, pinCenter.y - 4.0f));
        ed::BeginPin(ed::PinId(nodeEditorPinIdValue(node.id, i)),
                     i == 0 ? ed::PinKind::Input : ed::PinKind::Output);
        ed::PinPivotAlignment(ImVec2(0.5f, 0.5f));
        ImGui::Dummy(ImVec2(8.0f, 8.0f));
        ed::EndPin();
    }

    ed::EndNode();

    if(ImGui::IsItemVisible()){
        ImDrawList* bg = ed::GetNodeBackgroundDrawList(nodeId);
        for(const auto& leg : geo.legs){
            bg->AddLine(ImVec2(origin.x + leg[0].x, origin.y + leg[0].y),
                        ImVec2(origin.x + leg[1].x, origin.y + leg[1].y),
                        IM_COL32(230, 230, 230, 255), 3.0f);
        }

        const char* stateText = "N/A";
        if(node.turnout){
            switch(node.turnout->getState()){
            case Turnout::TurnoutState::Unknown: stateText = "unknown"; break;
            case Turnout::TurnoutState::Closed:  stateText = "closed";  break;
            case Turnout::TurnoutState::Thrown:  stateText = "thrown";  break;
            }
        }
        bg->AddText(ImVec2(origin.x, origin.y + kTurnoutContentSize / 4.0f),
                    IM_COL32(230, 230, 230, 255), stateText);
    }

    // Rotation is an editing action -- keep it unavailable in Operate mode,
    // matching the QWidget implementation (there, nothing could ever become
    // m_selectedWidget, and so the rotate handle could never appear, unless
    // m_allowMoving was on).
    if(m_mode == PanelMode::Edit && ed::IsNodeSelected(nodeId)){
        drawRotateHandle(node, origin);
    }
}

void ImguiPanelWidget::drawRotateHandle(TurnoutNode& node, ImVec2 nodeTopLeft)
{
    const ImVec2 centerCanvas(nodeTopLeft.x + kNodeSize / 2.0f, nodeTopLeft.y + kNodeSize / 2.0f);
    const double rad = node.rotationDegrees * std::numbers::pi / 180.0;
    const float handleDistance = kTurnoutContentSize / 2.0f + kRotateHandleDistance;
    const ImVec2 handleCanvas(centerCanvas.x + std::sin(rad) * handleDistance,
                              centerCanvas.y - std::cos(rad) * handleDistance);

    // Draw and hit-test the handle in screen space so its size stays constant
    // regardless of zoom, and so a click on it is claimed here rather than
    // being interpreted as a click on the node/canvas underneath -- the same
    // reason PanelDisplay::eventFilter existed in the QWidget implementation.
    ed::Suspend();

    const ImVec2 centerScreen = ed::CanvasToScreen(centerCanvas);
    const ImVec2 handleScreen = ed::CanvasToScreen(handleCanvas);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddLine(centerScreen, handleScreen, IM_COL32(90, 90, 90, 200));

    ImGui::SetCursorScreenPos(ImVec2(handleScreen.x - kRotateHandleRadius,
                                     handleScreen.y - kRotateHandleRadius));
    ImGui::PushID(static_cast<int>(node.id));
    ImGui::InvisibleButton("##rotate_handle", ImVec2(kRotateHandleRadius * 2.0f, kRotateHandleRadius * 2.0f));
    const bool dragging = ImGui::IsItemActive();
    ImGui::PopID();

    drawList->AddCircleFilled(handleScreen, kRotateHandleRadius, IM_COL32(255, 255, 255, 255));
    drawList->AddCircle(handleScreen, kRotateHandleRadius, IM_COL32(30, 140, 30, 255), 0, 2.0f);

    if(dragging){
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        const ImVec2 delta(mouse.x - centerScreen.x, mouse.y - centerScreen.y);
        // 0 degrees = straight up, positive = clockwise, matching QPainter::rotate()
        // (see turnoutGeometry()) and the QWidget implementation this replaces.
        double angle = std::atan2(delta.x, -delta.y) * 180.0 / std::numbers::pi;
        if(ImGui::GetIO().KeyCtrl){
            angle = std::lround(angle / kRotationSnapDegrees) * kRotationSnapDegrees;
        }
        node.rotationDegrees = angle;
    }

    ed::Resume();
}

void ImguiPanelWidget::syncTurnoutTransforms()
{
    if(m_mode == PanelMode::Operate){
        // Nothing moves while operating: force every node back to its last
        // known-good position every frame, so a click that the node editor
        // interpreted as the start of a drag never actually goes anywhere
        // (matches the QWidget implementation, where dragging only ever
        // engaged via a right-click that Operate mode never generated).
        for(TurnoutNode& node : m_model.turnouts()){
            ed::SetNodePosition(ed::NodeId(nodeEditorNodeIdValue(node.id)), node.position);
        }
        return;
    }

    const bool snapToGrid = ImGui::GetIO().KeyCtrl;

    for(TurnoutNode& node : m_model.turnouts()){
        const ed::NodeId nodeId(nodeEditorNodeIdValue(node.id));
        ImVec2 pos = ed::GetNodePosition(nodeId);

        const bool moved = (pos.x != node.position.x) || (pos.y != node.position.y);
        if(snapToGrid && moved){
            pos.x = std::lround(pos.x / kGridSize) * kGridSize;
            pos.y = std::lround(pos.y / kGridSize) * kGridSize;
            ed::SetNodePosition(nodeId, pos);
        }

        node.position = pos;
    }
}

void ImguiPanelWidget::handleOperateModeClicks()
{
    if(m_mode != PanelMode::Operate){
        m_clickCandidateId = 0;
        return;
    }

    const ImGuiIO& io = ImGui::GetIO();

    if(ImGui::IsMouseClicked(ImGuiMouseButton_Left)){
        const ed::NodeId hovered = ed::GetHoveredNode();
        m_clickCandidateId = hovered.Get()
            ? static_cast<PanelItemId>(hovered.Get() / kNodeEditorIdStride)
            : 0;
        m_clickStartMouse = io.MousePos;
        m_clickStartTime = ImGui::GetTime();
    }

    if(ImGui::IsMouseReleased(ImGuiMouseButton_Left) && m_clickCandidateId != 0){
        const ed::NodeId hoveredNow = ed::GetHoveredNode();
        const PanelItemId releasedOverId = hoveredNow.Get()
            ? static_cast<PanelItemId>(hoveredNow.Get() / kNodeEditorIdStride)
            : 0;

        const float dx = std::abs(io.MousePos.x - m_clickStartMouse.x);
        const float dy = std::abs(io.MousePos.y - m_clickStartMouse.y);
        const double elapsed = ImGui::GetTime() - m_clickStartTime;

        if(releasedOverId == m_clickCandidateId
           && dx < kClickMaxMovePixels && dy < kClickMaxMovePixels
           && elapsed > 0.0 && elapsed < kClickMaxSeconds){
            if(TurnoutNode* node = m_model.findTurnout(m_clickCandidateId)){
                LOG4CXX_DEBUG_FMT(logger, "Toggle turnout T{}", node->id);
                if(node->turnout){
                    node->turnout->toggleTurnout();
                }
            }
        }

        m_clickCandidateId = 0;
    }
}

ImguiPanelWidget::ResolvedSegment ImguiPanelWidget::resolveSegment(TrackSegmentEdge& seg) const
{
    ResolvedSegment r;
    r.a = resolveSegmentEnd(m_model, seg.a);
    r.b = resolveSegmentEnd(m_model, seg.b);

    if(seg.straight){
        // Recompute as the endpoints move/rotate, matching
        // TrackSegment::straightenControlOffsets() -- keeps a straight
        // segment tracking a moving/rotating turnout as a straight line
        // until the user actually drags a control handle.
        const ImVec2 diff(r.b.x - r.a.x, r.b.y - r.a.y);
        seg.controlOffsetA = ImVec2(diff.x / 3.0f, diff.y / 3.0f);
        seg.controlOffsetB = ImVec2(-diff.x / 3.0f, -diff.y / 3.0f);
    }

    r.ca = ImVec2(r.a.x + seg.controlOffsetA.x, r.a.y + seg.controlOffsetA.y);
    r.cb = ImVec2(r.b.x + seg.controlOffsetB.x, r.b.y + seg.controlOffsetB.y);
    return r;
}

void ImguiPanelWidget::drawSegments()
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    for(TrackSegmentEdge& seg : m_model.segments()){
        const ResolvedSegment r = resolveSegment(seg);
        drawList->AddBezierCubic(r.a, r.ca, r.cb, r.b, IM_COL32(230, 230, 230, 255), 3.0f);

        if(m_mode == PanelMode::Edit && m_tool == Tool::Select && seg.id == m_selectedSegmentId){
            drawSegmentHandles(seg, r);
        }
    }
}

void ImguiPanelWidget::drawSegmentHandles(TrackSegmentEdge& seg, const ResolvedSegment& r)
{
    // Dashed tethers from each endpoint to its control point, matching
    // PanelDisplay::paintEvent()'s presentation for a selected segment. Drawn
    // in canvas space -- same reasoning as the bezier curve itself -- so no
    // Suspend() is needed just for these lines.
    auto addDashedLine = [](ImDrawList* dl, ImVec2 from, ImVec2 to, ImU32 color){
        constexpr float kDashLength = 6.0f;
        const float dx = to.x - from.x;
        const float dy = to.y - from.y;
        const float length = std::sqrt(dx * dx + dy * dy);
        if(length < 1.0f){
            return;
        }
        const int steps = std::max(1, static_cast<int>(length / kDashLength));
        for(int i = 0; i < steps; i += 2){
            const int endStep = std::min(i + 1, steps);
            dl->AddLine(ImVec2(from.x + dx * (float(i) / steps), from.y + dy * (float(i) / steps)),
                        ImVec2(from.x + dx * (float(endStep) / steps), from.y + dy * (float(endStep) / steps)),
                        color, 1.0f);
        }
    };

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    addDashedLine(drawList, r.a, r.ca, IM_COL32(150, 150, 150, 220));
    addDashedLine(drawList, r.b, r.cb, IM_COL32(150, 150, 150, 220));

    // The handles themselves are drawn/hit-tested in screen space, same
    // reasoning as drawRotateHandle(): fixed size regardless of zoom, and
    // claimed here before the node editor's own background click handling
    // sees them.
    ed::Suspend();

    ImDrawList* fg = ImGui::GetWindowDrawList();
    ImGui::PushID(static_cast<int>(seg.id));

    const ImVec2 canvasHandles[2] = { r.ca, r.cb };
    ImVec2* offsets[2] = { &seg.controlOffsetA, &seg.controlOffsetB };
    const ImVec2 endpoints[2] = { r.a, r.b };
    const char* ids[2] = { "##ctrlA", "##ctrlB" };

    for(int i = 0; i < 2; i++){
        const ImVec2 handleScreen = ed::CanvasToScreen(canvasHandles[i]);
        ImGui::SetCursorScreenPos(ImVec2(handleScreen.x - kRotateHandleRadius,
                                         handleScreen.y - kRotateHandleRadius));
        ImGui::InvisibleButton(ids[i], ImVec2(kRotateHandleRadius * 2.0f, kRotateHandleRadius * 2.0f));
        if(ImGui::IsItemActive()){
            const ImVec2 mouseCanvas = ed::ScreenToCanvas(ImGui::GetIO().MousePos);
            *offsets[i] = ImVec2(mouseCanvas.x - endpoints[i].x, mouseCanvas.y - endpoints[i].y);
            seg.straight = false;
        }
        fg->AddCircleFilled(handleScreen, kRotateHandleRadius, IM_COL32(255, 255, 255, 255));
        fg->AddCircle(handleScreen, kRotateHandleRadius, IM_COL32(30, 140, 30, 255), 0, 2.0f);
    }

    ImGui::PopID();
    ed::Resume();
}

void ImguiPanelWidget::drawSegmentPreview()
{
    if(!m_drawingSegment){
        return;
    }

    const ImVec2 start = resolveSegmentEnd(m_model, m_drawStart);
    // io.MousePos is already canvas space here: drawCanvas() (which calls
    // this) runs inside ed::Begin/End, not suspended, and imgui-node-editor
    // rewrites io.MousePos into canvas coordinates for that whole span
    // (ImGuiEx::Canvas::EnterLocalSpace()) -- restoring it to real screen
    // coordinates only inside a Suspend()/Resume() block or after ed::End().
    // (drawRotateHandle()/drawSegmentHandles() convert explicitly because
    // they run *inside* such a Suspend block; handleOperateModeClicks() needs
    // no conversion either, because it runs *after* ed::End().)
    const ImVec2 endCanvas = ImGui::GetIO().MousePos;

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 prev = start;
    for(const ImVec2& control : m_drawControls){
        drawList->AddLine(prev, control, IM_COL32(90, 200, 255, 200), 2.0f);
        drawList->AddCircleFilled(control, 4.0f, IM_COL32(90, 200, 255, 255));
        prev = control;
    }
    drawList->AddLine(prev, endCanvas, IM_COL32(90, 200, 255, 200), 2.0f);
}

void ImguiPanelWidget::handleSegmentSelection()
{
    if(m_tool != Tool::Select){
        return;
    }

    if(ed::GetHoveredNode().Get() != 0 || ed::GetHoveredPin().Get() != 0){
        if(ImGui::IsMouseClicked(ImGuiMouseButton_Left)){
            // Let the node editor's own node/pin selection win.
            m_selectedSegmentId = 0;
        }
        return;
    }

    // 0 = left (matches Config::SelectButtonIndex's default); -1 = no
    // background click completed this frame. This is deliberately not
    // ImGui::IsMouseClicked(): that fires on the press frame, while
    // GetBackgroundClickButtonIndex() is only populated by ed::End()'s own
    // per-frame action processing (confirmed by reading
    // SelectAction::Accept() in imgui_node_editor.cpp) -- the two essentially
    // never coincide, which is exactly why this function must be called
    // after ed::End() (see the header) rather than from inside drawCanvas().
    // It also naturally excludes clicks on the toolbox/debug windows: ed::
    // only ever reports a background click for its own canvas.
    if(ed::GetBackgroundClickButtonIndex() != 0){
        return;
    }

    // io.MousePos is real screen space here (this runs after ed::End()).
    const ImVec2 mouseCanvas = ed::ScreenToCanvas(ImGui::GetIO().MousePos);
    // A constant screen-pixel hit tolerance, expressed in canvas units at the
    // current zoom (GetCurrentZoom() is the inverse scale, so multiplying
    // converts "screen pixels" to "canvas units" -- confirmed in Phase 1).
    const float toleranceCanvas = kSegmentHitTolerance * ed::GetCurrentZoom();

    PanelItemId hitId = 0;
    float bestDist = toleranceCanvas;
    for(TrackSegmentEdge& seg : m_model.segments()){
        const ResolvedSegment r = resolveSegment(seg);
        constexpr int kSamples = 24;
        ImVec2 prev = r.a;
        for(int i = 1; i <= kSamples; i++){
            const ImVec2 next = cubicBezierPoint(r.a, r.ca, r.cb, r.b, float(i) / kSamples);
            const float dist = distanceToLineSegment(mouseCanvas, prev, next);
            if(dist <= bestDist){
                bestDist = dist;
                hitId = seg.id;
            }
            prev = next;
        }
    }

    m_selectedSegmentId = hitId;
    if(hitId != 0){
        ed::ClearSelection();
    }
}

void ImguiPanelWidget::handleSegmentDrawingClicks()
{
    if(m_tool != Tool::DrawSegment){
        m_drawingSegment = false;
        m_drawControls.clear();
        return;
    }

    if(m_drawingSegment && ImGui::IsKeyPressed(ImGuiKey_Escape)){
        m_drawingSegment = false;
        m_drawControls.clear();
        return;
    }

    const ed::PinId hoveredPin = ed::GetHoveredPin();
    if(ImGui::IsMouseClicked(ImGuiMouseButton_Left) && hoveredPin.Get() != 0){
        SegmentEnd end;
        end.nodeId = nodeIdFromPinIdValue(hoveredPin.Get());
        end.pinIndex = pinIndexFromPinIdValue(hoveredPin.Get());

        if(!m_drawingSegment){
            m_drawingSegment = true;
            m_drawStart = end;
            m_drawControls.clear();
        }else{
            finishSegmentDraw(end);
        }
        return;
    }

    // See the note on handleSegmentSelection() for why
    // GetBackgroundClickButtonIndex() rather than IsMouseClicked() here.
    if(m_drawingSegment && m_drawControls.size() < 2
       && ed::GetBackgroundClickButtonIndex() == 0){
        // io.MousePos is real screen space here (this runs after ed::End()).
        m_drawControls.push_back(ed::ScreenToCanvas(ImGui::GetIO().MousePos));
    }
}

void ImguiPanelWidget::handleSegmentInsertMenu()
{
    if(m_tool != Tool::DrawSegment || !m_drawingSegment){
        return;
    }

    // Right-click on empty canvas: offer to insert a node partway through.
    // openPopupPos is captured here, still unsuspended (this runs inside
    // ed::Begin/End, from drawCanvas()), so it's already in canvas space --
    // exactly what insertTurnoutIntoDraw() needs, with no further conversion,
    // even though the click that ultimately consumes it (the "Turnout" menu
    // item) may land on a later, suspended frame.
    const ImVec2 openPopupPos = ImGui::GetIO().MousePos;

    ed::Suspend();
    if(ed::ShowBackgroundContextMenu()){
        ImGui::OpenPopup("Insert Node");
    }
    if(ImGui::BeginPopup("Insert Node")){
        if(ImGui::MenuItem("Turnout")){
            insertTurnoutIntoDraw(openPopupPos);
        }
        ImGui::Separator();
        if(ImGui::MenuItem("Cancel")){
            m_drawingSegment = false;
            m_drawControls.clear();
        }
        ImGui::EndPopup();
    }
    ed::Resume();
}

void ImguiPanelWidget::finishSegmentDraw(SegmentEnd end)
{
    TrackSegmentEdge& seg = m_model.addSegment(m_drawStart, end);

    if(!m_drawControls.empty()){
        const ImVec2 a = resolveSegmentEnd(m_model, seg.a);
        const ImVec2 b = resolveSegmentEnd(m_model, seg.b);
        seg.controlOffsetA = ImVec2(m_drawControls[0].x - a.x, m_drawControls[0].y - a.y);
        if(m_drawControls.size() >= 2){
            seg.controlOffsetB = ImVec2(m_drawControls[1].x - b.x, m_drawControls[1].y - b.y);
        }else{
            // Only one control point was placed: mirror it across the
            // segment so the curve bends smoothly at both ends rather than
            // snapping straight at the end the user didn't touch.
            seg.controlOffsetB = ImVec2(-seg.controlOffsetA.x, -seg.controlOffsetA.y);
        }
        seg.straight = false;
    }
    // else: leave seg.straight = true; resolveSegment() keeps it tracking as
    // a straight line between its (possibly moving) endpoints.

    m_drawingSegment = false;
    m_drawControls.clear();
}

void ImguiPanelWidget::insertTurnoutIntoDraw(ImVec2 canvasPos)
{
    // Centre the new node on the click.
    TurnoutNode& node = m_model.addTurnout(
        ImVec2(canvasPos.x - kNodeSize / 2.0f, canvasPos.y - kNodeSize / 2.0f));

    // Connect the in-progress segment to whichever of the new node's three
    // pins is nearest the click.
    const TurnoutGeometry geo = turnoutGeometry(node.hand, node.rotationDegrees);
    int nearest = 0;
    float bestDistSq = std::numeric_limits<float>::max();
    for(int i = 0; i < 3; i++){
        const ImVec2 pinPos(node.position.x + geo.pins[i].x, node.position.y + geo.pins[i].y);
        const float dx = pinPos.x - canvasPos.x;
        const float dy = pinPos.y - canvasPos.y;
        const float distSq = dx * dx + dy * dy;
        if(distSq < bestDistSq){
            bestDistSq = distSq;
            nearest = i;
        }
    }

    finishSegmentDraw(SegmentEnd{node.id, nearest, ImVec2()});

    // Keep going: start a new segment from a different pin on the same node.
    // There's no real notion of "the next free pin" without a richer
    // connectivity model, so this just keeps the flow going rather than
    // silently stopping partway through what the user was drawing.
    m_drawingSegment = true;
    m_drawStart = SegmentEnd{node.id, (nearest + 1) % 3, ImVec2()};
    m_drawControls.clear();
}

void ImguiPanelWidget::handleDelete()
{
    // Segments aren't node-editor items, so their deletion is ours to handle.
    if(m_selectedSegmentId != 0 && ImGui::IsKeyPressed(ImGuiKey_Delete)){
        m_model.removeSegment(m_selectedSegmentId);
        m_selectedSegmentId = 0;
    }

    // Node deletion goes through ed::'s own delete flow (Del key while a node
    // is selected). PanelModel::removeTurnout() cascades to drop any segment
    // that was attached to the deleted node.
    if(ed::BeginDelete()){
        ed::NodeId nodeId;
        while(ed::QueryDeletedNode(&nodeId)){
            if(ed::AcceptDeletedItem()){
                const PanelItemId id = static_cast<PanelItemId>(nodeId.Get() / kNodeEditorIdStride);
                m_model.removeTurnout(id);
                m_seededNodes.erase(id);
            }
        }
    }
    ed::EndDelete();
}

void ImguiPanelWidget::drawToolbox(const ImGuiViewport* viewport)
{
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + 12.0f, viewport->Pos.y + 12.0f),
                            ImGuiCond_FirstUseEver);
    if(ImGui::Begin("Panel toolbox", nullptr, ImGuiWindowFlags_AlwaysAutoResize)){
        bool operate = (m_mode == PanelMode::Operate);
        if(ImGui::RadioButton("Operate", operate)){
            m_mode = PanelMode::Operate;
        }
        ImGui::SameLine();
        if(ImGui::RadioButton("Edit", !operate)){
            m_mode = PanelMode::Edit;
        }

        // Everything below is an editing action -- matches the "Tool (Edit
        // only)" split in the design.
        ImGui::BeginDisabled(m_mode != PanelMode::Edit);

        if(ImGui::Button("Add Turnout")){
            // Stagger repeated adds so they don't land exactly on top of one
            // another; the user can drag it wherever it belongs.
            const float offset = 24.0f * static_cast<float>(m_model.turnouts().size() % 8);
            m_model.addTurnout(ImVec2(offset, offset));
            m_tool = Tool::Select;
        }

        bool selectTool = (m_tool == Tool::Select);
        if(ImGui::RadioButton("Select", selectTool)){
            m_tool = Tool::Select;
        }
        ImGui::SameLine();
        if(ImGui::RadioButton("Draw Segment", !selectTool)){
            m_tool = Tool::DrawSegment;
        }

        if(m_selectedSegmentId != 0 && ImGui::Button("Make Straight")){
            if(TrackSegmentEdge* seg = m_model.findSegment(m_selectedSegmentId)){
                seg->straight = true;
            }
        }

        ImGui::EndDisabled();

        if(m_mode == PanelMode::Edit){
            drawProperties();
        }
    }
    ImGui::End();
}

void ImguiPanelWidget::drawProperties()
{
    ed::SetCurrentEditor(m_editor);
    ed::NodeId selectedNodeId;
    const int selectedCount = ed::GetSelectedNodes(&selectedNodeId, 1);
    ed::SetCurrentEditor(nullptr);

    if(selectedCount != 1){
        return;
    }
    TurnoutNode* node = m_model.findTurnout(panelItemIdFromNodeIdValue(selectedNodeId.Get()));
    if(!node){
        return;
    }

    ImGui::Separator();
    ImGui::Text("Turnout properties");
    std::vector<PanelField> fields = fieldsFor(*node, m_state);
    renderPanelFields(fields);
}

void ImguiPanelWidget::drawDebugWindow(const ImGuiViewport* viewport)
{
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + viewport->Size.x - 260.0f,
                                   viewport->Pos.y + 12.0f),
                            ImGuiCond_FirstUseEver);
    if(ImGui::Begin("Panel debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize)){
        const ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("%s", m_name.toStdString().c_str());
        ImGui::Text("%.1f FPS  dpr %.2f", io.Framerate, io.DisplayFramebufferScale.x);
        ImGui::Text("Ctrl %d  Shift %d  Alt %d", io.KeyCtrl, io.KeyShift, io.KeyAlt);

        ed::SetCurrentEditor(m_editor);
        // GetCurrentZoom() returns the inverse scale: 0.5 means zoomed in 2x.
        ImGui::Text("1/zoom %.3f", ed::GetCurrentZoom());
        ImGui::Text("canvas %.0fx%.0f", m_dbgAvail.x, m_dbgAvail.y);
        {
            const ImVec2 origin = ed::CanvasToScreen(ImVec2(0.0f, 0.0f));
            ImGui::Text("origin: %.1f,%.1f", origin.x, origin.y);
        }
        ImGui::Text("mouse: %.1f,%.1f", io.MousePos.x, io.MousePos.y);
        ImGui::Text("hover node %llu pin %llu", (unsigned long long)ed::GetHoveredNode().Get(),
                    (unsigned long long)ed::GetHoveredPin().Get());
        ImGui::Text("draw %d ctl %zu segs %zu sel %u",
                    m_drawingSegment, m_drawControls.size(), m_model.segments().size(),
                    m_selectedSegmentId);
        for(const TurnoutNode& node : m_model.turnouts()){
            ImGui::Text("T%u pos %.1f,%.1f rot %.2f", node.id,
                       node.position.x, node.position.y, node.rotationDegrees);
        }
        ed::SetCurrentEditor(nullptr);
    }
    ImGui::End();
}

QString ImguiPanelWidget::getName() const
{
    return m_name;
}

void ImguiPanelWidget::setName(QString name)
{
    m_name = name;
}
