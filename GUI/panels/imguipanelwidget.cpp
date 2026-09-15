#include <QGuiApplication>
#include <QHideEvent>
#include <QShowEvent>

#include <imgui.h>
#include <imgui-node-editor/imgui_node_editor.h>

#include <log4cxx/logger.h>
#include <fmt/format.h>

#include "imguipanelwidget.h"

namespace ed = ax::NodeEditor;

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("traingui.ImguiPanelWidget");

// Redraw interval.  Immediate mode plus the node editor's own animations
// (selection, navigation easing) make update-on-input unreliable, so just run a
// steady frame loop while the panel is actually visible.
static constexpr int kRepaintIntervalMs = 16;

// The node editor fits the view to its content on the first frame, before node
// sizes have been measured, which leaves a brand new panel at a nonsense zoom.
// Re-fit once the content has settled.
static constexpr int kSettleFrames = 3;

ImguiPanelWidget::ImguiPanelWidget(QWidget* parent)
    : QOpenGLWidget{parent}
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
    ed::SetCurrentEditor(nullptr);

    ImGui::End();
    ImGui::PopStyleVar();

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
    // Placeholder content until the panel model lands: two nodes joined by a
    // hand-drawn bezier, which is the shape the real turnouts and track
    // segments will take.
    //
    // Two things here are load-bearing and easy to get wrong:
    //  * ed::GetNodeBackgroundDrawList() must be called AFTER ed::EndNode().
    //    Calling it between BeginNode/EndNode trips an assertion inside
    //    ImDrawListSplitter::SetCurrentChannel.
    //  * Drawing on ImGui::GetWindowDrawList() between ed::Begin/ed::End puts
    //    the geometry in canvas space, so it pans and zooms with the nodes.
    //    (ed::Suspend() + GetForegroundDrawList() would be screen space, and
    //    would need ed::CanvasToScreen() on every point.)
    ImVec2 pinPos[2];

    for(int i = 0; i < 2; i++){
        const ed::NodeId nodeId(i + 1);

        if(m_frame == 0){
            ed::SetNodePosition(nodeId, ImVec2(i * 200.0f, 0.0f));
        }

        ed::BeginNode(nodeId);
        ImGui::Dummy(ImVec2(kNodeSize, kNodeSize));
        const ImVec2 origin = ImGui::GetItemRectMin();

        ImGui::SetCursorScreenPos(ImVec2(origin.x + (i == 0 ? kNodeSize - 8.0f : 0.0f),
                                         origin.y + kNodeSize / 2.0f - 4.0f));
        ed::BeginPin(ed::PinId((i + 1) * 8 + 1),
                     i == 0 ? ed::PinKind::Output : ed::PinKind::Input);
        ed::PinPivotAlignment(ImVec2(0.5f, 0.5f));
        ImGui::Dummy(ImVec2(8.0f, 8.0f));
        const ImVec2 pinMin = ImGui::GetItemRectMin();
        pinPos[i] = ImVec2(pinMin.x + 4.0f, pinMin.y + 4.0f);
        ed::EndPin();

        ed::EndNode();

        if(ImGui::IsItemVisible()){
            ed::GetNodeBackgroundDrawList(nodeId)
                ->AddLine(ImVec2(origin.x, origin.y + kNodeSize / 2.0f),
                          ImVec2(origin.x + kNodeSize, origin.y + kNodeSize / 2.0f),
                          IM_COL32(230, 230, 230, 255), 3.0f);
        }
    }

    const float dx = (pinPos[1].x - pinPos[0].x) / 3.0f;
    ImGui::GetWindowDrawList()->AddBezierCubic(
        pinPos[0],
        ImVec2(pinPos[0].x + dx, pinPos[0].y),
        ImVec2(pinPos[1].x - dx, pinPos[1].y),
        pinPos[1],
        IM_COL32(230, 230, 230, 255), 3.0f);
}

void ImguiPanelWidget::drawDebugWindow(const ImGuiViewport* viewport)
{
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + viewport->Size.x - 260.0f,
                                   viewport->Pos.y + 12.0f),
                            ImGuiCond_FirstUseEver);
    if(ImGui::Begin("Panel debug")){
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
