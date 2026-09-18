#ifndef IMGUIPANELWIDGET_H
#define IMGUIPANELWIDGET_H

#include <QOpenGLWidget>
#include <set>
#include <string>
#include <vector>

#include <imgui.h>
#include <QString>
#include <QTimer>

#include "QtImGui.h"
#include "panelmodel.h"

namespace ax {
namespace NodeEditor {
struct EditorContext;
}
}

struct TrainUtilsState;

/**
 * Operate: clicking a turnout throws it; nothing on the panel can be moved or
 * rotated (any drag is undone every frame -- see syncTurnoutTransforms()).
 * Edit: clicking a turnout selects/drags it instead; the rotate handle and
 * grid/angle snapping are available. Matches the QWidget implementation's
 * m_allowMoving flag, split into two explicit, mutually exclusive modes.
 */
enum class PanelMode {
    Operate,
    Edit
};

/**
 * Edit-mode tool. Select: click a node to drag/rotate it (Phase 2/3), click a
 * segment to select it and drag its bezier handles. DrawSegment: click a pin
 * to start a segment, click empty canvas to place up to two bezier control
 * points, click a second pin to finish, or right-click empty canvas for a
 * menu to insert a new node (today just Turnout) partway through.
 */
enum class Tool {
    Select,
    DrawSegment
};

/**
 * Hosts a track panel.  Everything inside the panel is drawn with Dear ImGui
 * (via the vendored qtimgui backend) on top of imgui-node-editor -- this widget
 * is the only Qt-aware part of the panel.
 *
 * Each widget owns its own ImGui context and its own node editor context, so
 * several panels can be open in different docks at the same time.
 */
class ImguiPanelWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit ImguiPanelWidget(TrainUtilsState* state, QWidget* parent = nullptr);
    ~ImguiPanelWidget() override;

    QString getName() const;
    void setName(QString name);

protected:
    void initializeGL() override;
    void paintGL() override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    /**
     * Push Qt's current modifier state into ImGui.
     *
     * qtimgui only touches the modifiers from key events, and it assigns
     * io.KeyCtrl/KeyShift/KeyAlt directly.  Since ImGui 1.87 those fields are
     * recomputed by NewFrame() from io.KeyMods, which is only fed by
     * AddKeyEvent(ImGuiMod_*) -- and qtimgui's key map contains no modifier
     * keys at all.  Without this the modifiers would always read as released,
     * which would break snap-to-grid, snap-rotate and the node editor's own
     * ctrl-multiselect.  Polling Qt also means modifiers work even when this
     * widget does not hold keyboard focus.
     *
     * Must be called after QtImGui::newFrame(), which is what makes our ImGui
     * context current and runs NewFrame().
     */
    bool ensureEditor();
    void applyKeyboardModifiers();

    void drawFrame();
    void drawCanvas();
    void drawGrid();
    void drawToolbox(const struct ImGuiViewport* viewport);
    /** The selected turnout's editable fields (panelfields.h), shown in the
     *  toolbox in Edit mode. Needs the editor current (to ask ed:: which node
     *  is selected), so it brackets its own SetCurrentEditor() call rather
     *  than relying on drawFrame()'s -- drawToolbox() runs after that's
     *  already been cleared for the frame. */
    void drawProperties();
    void drawDebugWindow(const struct ImGuiViewport* viewport);

    void drawTurnoutNode(TurnoutNode& node);
    /** nodeTopLeft is in node-editor canvas space (as returned by GetItemRectMin() while inside ed::Begin/End). */
    void drawRotateHandle(TurnoutNode& node, ImVec2 nodeTopLeft);
    /** Pulls each node's position back from the node editor after ed::End(), applying Ctrl
     *  grid-snap in Edit mode, or forcing it back to the model's position every frame in
     *  Operate mode so an accidental drag never sticks. */
    void syncTurnoutTransforms();
    /** Click-to-throw disambiguation (Operate mode only): mirrors the press/release timing
     *  and movement thresholds TurnoutDisplay::mousePressEvent/mouseReleaseEvent used, since
     *  the node editor -- not a per-widget event handler -- now owns the click/drag itself. */
    void handleOperateModeClicks();

    // --- Track segments (Phase 4) ---

    /** A segment's endpoints/control points resolved to this frame's canvas positions. */
    struct ResolvedSegment {
        ImVec2 a, b, ca, cb;
    };
    /** Recomputes controlOffsetA/B from the endpoints when seg.straight, so a
     *  straight segment keeps tracking a moving/rotating endpoint. */
    ResolvedSegment resolveSegment(TrackSegmentEdge& seg) const;
    void drawSegments();
    /** Bezier control handles for the selected segment: drawn/dragged in a Suspend
     *  block, same reasoning as drawRotateHandle(). */
    void drawSegmentHandles(TrackSegmentEdge& seg, const ResolvedSegment& r);
    /** The dashed line while actively drawing a new segment, following the cursor. */
    void drawSegmentPreview();
    /** Select tool: hit-tests a left-click against every segment (only when it
     *  didn't land on a node/pin, which take priority). Must run after ed::End() --
     *  see the note on handleSegmentDrawingClicks() for why. */
    void handleSegmentSelection();
    /** DrawSegment tool: the click-driven parts of the state machine described on
     *  the Tool enum (starting/finishing on a pin, placing control points on empty
     *  canvas). Must run after ed::End(): ed::IsBackgroundClicked() and
     *  ed::GetBackgroundClickButtonIndex() are only populated by ed::End()'s own
     *  per-frame action processing, so querying them earlier (e.g. from inside
     *  drawCanvas(), between ed::Begin() and ed::End()) always reads stale data --
     *  confirmed by a precise click landing exactly on a rendered segment still
     *  failing to select it when this was called too early. */
    void handleSegmentDrawingClicks();
    /** DrawSegment tool: the right-click "insert a node here" popup. Must run
     *  before ed::End() (ed::ShowBackgroundContextMenu(), like ed::BeginDelete(),
     *  is part of the node editor's own per-frame processing inside Begin/End --
     *  confirmed against imgui-node-editor's own blueprints-example.cpp). */
    void handleSegmentInsertMenu();
    /** Ends the in-progress segment by connecting its start to `end`, converting
     *  any placed control points into TrackSegmentEdge's endpoint-relative offsets. */
    void finishSegmentDraw(SegmentEnd end);
    /** Inserts a Turnout node at canvasPos partway through drawing a segment: connects
     *  the in-progress segment to whichever of its 3 pins is nearest, then continues
     *  drawing from another pin on that same node. */
    void insertTurnoutIntoDraw(ImVec2 canvasPos);
    /** Segment deletion (ours to handle) and node deletion (via ed::BeginDelete(),
     *  which PanelModel::removeTurnout() cascades to the segments attached to it). */
    void handleDelete();

    // Turnout artwork is drawn inside a fixed square big enough that rotating it
    // never clips (the diagonal of the 50x50 artwork is ~71px).
    static constexpr float kNodeSize = 72.0f;
    /** Consecutive paints at the same size before the editor is created. */
    static constexpr int kStableSizeFrames = 2;
    /** Matches PanelDisplay::kGridSize in the QWidget implementation this replaces. */
    static constexpr float kGridSize = 10.0f;
    /** Matches PanelDisplay::kRotationSnapDegrees. */
    static constexpr double kRotationSnapDegrees = 5.0;
    /** Matches PanelDisplay::kRotateHandleDistance -- offset beyond the content's half-height. */
    static constexpr float kRotateHandleDistance = 20.0f;
    static constexpr float kRotateHandleRadius = 6.0f;
    /** Matches TurnoutDisplay::mouseReleaseEvent's click-vs-drag thresholds. */
    static constexpr float kClickMaxMovePixels = 10.0f;
    static constexpr double kClickMaxSeconds = 1.0;
    /** Matches TrackSegment::PADDING, the curve click/select tolerance. */
    static constexpr float kSegmentHitTolerance = 8.0f;

    /** Owned by MainWindow; lifetime covers this widget's -- may be null in
     *  tests, in which case connection binding fields are simply omitted. */
    TrainUtilsState* m_state = nullptr;
    QtImGui::RenderRef m_imgui = nullptr;
    ax::NodeEditor::EditorContext* m_editor = nullptr;
    QTimer m_repaintTimer;
    QString m_name;
    bool m_imguiConfigured = false;
    int m_frame = 0;
    QSize m_lastSize;
    int m_stableSizeFrames = 0;
    ImVec2 m_dbgAvail;
    PanelMode m_mode = PanelMode::Operate;
    Tool m_tool = Tool::Select;
    /** The node hovered when the left mouse button went down, awaiting release
     *  to decide whether it was a click (toggle) or the start of a drag. 0 = none. */
    PanelItemId m_clickCandidateId = 0;
    ImVec2 m_clickStartMouse;
    double m_clickStartTime = 0.0;
    /** The node editor's own settings blob (node sizes, pan, zoom). */
    std::string m_editorSettings;

    PanelModel m_model;
    /** Node ids the node editor has already been given an initial position for
     *  (see drawTurnoutNode() -- after the first frame, position is owned by
     *  the node editor/user dragging, not re-seeded from the model each frame). */
    std::set<PanelItemId> m_seededNodes;

    /** 0 = no segment selected. Segments aren't node-editor items, so their
     *  selection is tracked here rather than queried from ed::. */
    PanelItemId m_selectedSegmentId = 0;
    /** Set while the DrawSegment tool has an in-progress segment (i.e. after
     *  the user has clicked a starting pin but before they've finished it). */
    bool m_drawingSegment = false;
    SegmentEnd m_drawStart;
    /** Canvas-space bezier control points placed so far this draw (0-2). */
    std::vector<ImVec2> m_drawControls;
};

#endif // IMGUIPANELWIDGET_H
