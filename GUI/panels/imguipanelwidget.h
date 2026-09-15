#ifndef IMGUIPANELWIDGET_H
#define IMGUIPANELWIDGET_H

#include <QOpenGLWidget>
#include <string>

#include <imgui.h>
#include <QString>
#include <QTimer>

#include "QtImGui.h"

namespace ax {
namespace NodeEditor {
struct EditorContext;
}
}

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
    explicit ImguiPanelWidget(QWidget* parent = nullptr);
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
    void drawDebugWindow(const struct ImGuiViewport* viewport);

    // Turnout artwork is drawn inside a fixed square big enough that rotating it
    // never clips (the diagonal of the 50x50 artwork is ~71px).
    static constexpr float kNodeSize = 72.0f;
    /** Consecutive paints at the same size before the editor is created. */
    static constexpr int kStableSizeFrames = 2;

    QtImGui::RenderRef m_imgui = nullptr;
    ax::NodeEditor::EditorContext* m_editor = nullptr;
    QTimer m_repaintTimer;
    QString m_name;
    bool m_imguiConfigured = false;
    int m_frame = 0;
    QSize m_lastSize;
    int m_stableSizeFrames = 0;
    ImVec2 m_dbgAvail;
    /** The node editor's own settings blob (node sizes, pan, zoom). */
    std::string m_editorSettings;
};

#endif // IMGUIPANELWIDGET_H
