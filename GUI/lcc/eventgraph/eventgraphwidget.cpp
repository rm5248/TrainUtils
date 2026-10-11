/* SPDX-License-Identifier: GPL-2.0 */
#include "eventgraphwidget.h"

#include <cmath>

#include <QGuiApplication>
#include <imgui-node-editor/imgui_node_editor.h>

#include "lcc/lccconnection.h"
#include "lcc-common.h"

#include <log4cxx/logger.h>
#include <fmt/format.h>

namespace ed = ax::NodeEditor;

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("traingui.EventGraphWidget");

// Same as ImguiPanelWidget: run a steady frame loop while visible
static constexpr int REPAINT_INTERVAL_MS = 32;
// Consecutive paints at the same size before the editor is created(see ImguiPanelWidget::ensureEditor())
static constexpr int STABLE_SIZE_FRAMES = 2;
// How long after something changes to save the layout
static constexpr int SAVE_DELAY_MS = 2000;

static constexpr float PIN_SIZE = 12.0f;
static constexpr float AUTO_PLACE_COLUMN_WIDTH = 420.0f;
static constexpr float AUTO_PLACE_ROW_HEIGHT = 360.0f;
static constexpr int AUTO_PLACE_COLUMNS = 4;

// How far each level of the tree is indented
static constexpr float TREE_INDENT = 14.0f;

// Node editor IDs must be unique and non-zero across nodes, pins and links
static constexpr uint64_t PIN_ID_BASE = 0x100000000ull;
static constexpr uint64_t LINK_ID_BASE = 0x200000000ull;
static constexpr uint64_t GROUP_PIN_ID_BASE = 0x300000000ull;

static const ImU32 COLOR_PRODUCER = IM_COL32(110, 200, 120, 255);
static const ImU32 COLOR_CONSUMER = IM_COL32(110, 160, 230, 255);
static const ImU32 COLOR_UNKNOWN = IM_COL32(170, 170, 170, 255);
static const ImVec4 COLOR_DIM(0.6f, 0.6f, 0.6f, 1.0f);
static const ImVec4 COLOR_ERROR(1.0f, 0.45f, 0.4f, 1.0f);
// Things that match the search
static const ImVec4 COLOR_MATCH(1.0f, 0.8f, 0.25f, 1.0f);
static const ImU32 COLOR_MATCH_BACKGROUND = IM_COL32(255, 200, 60, 70);

/**
 * A color for an event, so that the lines for different events can be told apart.
 */
static ImVec4 colorForEvent(uint64_t eventId, float alpha){
    // Mix the bits so that sequential event IDs get very different colors
    uint64_t h = eventId * 0x9E3779B97F4A7C15ull;
    float hue = static_cast<float>((h >> 40) % 360) / 360.0f;
    float r, g, b;
    ImGui::ColorConvertHSVtoRGB(hue, 0.55f, 0.95f, r, g, b);
    return ImVec4(r, g, b, alpha);
}

static QString formatNodeId(uint64_t nodeId){
    char buffer[20];
    lcc_node_id_to_dotted_format(nodeId, buffer, sizeof(buffer));
    return QString(buffer);
}

static QString formatEventId(uint64_t eventId){
    QStringList parts;
    for(int x = 7; x >= 0; x--){
        parts.push_back(QString("%1").arg((eventId >> (x * 8)) & 0xFF, 2, 16, QLatin1Char('0')).toUpper());
    }
    return parts.join('.');
}

EventGraphWidget::EventGraphWidget(std::shared_ptr<LCCConnection> conn, QWidget* parent)
    : QOpenGLWidget{parent},
    m_controller(std::make_unique<EventGraphController>(conn)),
    m_connectionName(conn->name())
{
    // Needed so key events reach qtimgui's event filter
    setFocusPolicy(Qt::StrongFocus);

    m_repaintTimer.setInterval(REPAINT_INTERVAL_MS);
    connect(&m_repaintTimer, &QTimer::timeout,
            this, QOverload<>::of(&EventGraphWidget::update));

    m_layout = EventGraphStorage::load(m_connectionName);
    m_controller->model().setLearnedDirections(m_layout.learnedDirections);

    // Save what we've learned once things settle down
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(SAVE_DELAY_MS);
    connect(&m_saveTimer, &QTimer::timeout, this, &EventGraphWidget::saveLayout);
    connect(m_controller.get(), &EventGraphController::modelChanged,
            &m_saveTimer, QOverload<>::of(&QTimer::start));
    connect(m_controller.get(), &EventGraphController::modelChanged,
            this, [this](){ m_modelVersion++; });

    m_controller->start();
}

EventGraphWidget::~EventGraphWidget()
{
    saveLayout();

    makeCurrent();
    if(m_editor){
        ed::DestroyEditor(m_editor);
        m_editor = nullptr;
    }
    // NOTE: same as ImguiPanelWidget, qtimgui has no way to free a RenderRef
    doneCurrent();
}

void EventGraphWidget::initializeGL()
{
    // defaultRender must be false: true would share one ImGui context between widgets
    m_imgui = QtImGui::initialize(this, false);
}

bool EventGraphWidget::ensureEditor()
{
    if(m_editor){
        return true;
    }

    // Don't create the editor until our size has settled - see ImguiPanelWidget::ensureEditor()
    const QSize current = size();
    if(current != m_lastSize){
        m_lastSize = current;
        m_stableSizeFrames = 0;
        return false;
    }
    if(++m_stableSizeFrames < STABLE_SIZE_FRAMES){
        return false;
    }

    ed::Config config;
    // Node positions are saved by us, so keep the editor's own settings(pan/zoom) in memory
    config.SettingsFile = nullptr;
    config.UserPointer = this;
    config.SaveSettings = [](const char* data, size_t size, ed::SaveReasonFlags, void* userPointer) {
        auto* self = static_cast<EventGraphWidget*>(userPointer);
        self->m_editorSettings.assign(data, size);
        return true;
    };
    config.LoadSettings = [](char* data, void* userPointer) {
        auto* self = static_cast<EventGraphWidget*>(userPointer);
        if(data){
            memcpy(data, self->m_editorSettings.data(), self->m_editorSettings.size());
        }
        return self->m_editorSettings.size();
    };
    m_editor = ed::CreateEditor(&config);

    // WORKAROUND: imgui-node-editor 0.9.3 draws its background/grid with a bad clip
    // rect, so make it transparent and draw our own - see ImguiPanelWidget::ensureEditor()
    ed::SetCurrentEditor(m_editor);
    ed::Style& style = ed::GetStyle();
    style.Colors[ed::StyleColor_Bg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ed::StyleColor_Grid] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    ed::SetCurrentEditor(nullptr);

    return true;
}

void EventGraphWidget::showEvent(QShowEvent* event)
{
    QOpenGLWidget::showEvent(event);
    m_repaintTimer.start();
}

void EventGraphWidget::hideEvent(QHideEvent* event)
{
    m_repaintTimer.stop();
    saveLayout();
    QOpenGLWidget::hideEvent(event);
}

void EventGraphWidget::applyKeyboardModifiers()
{
    // qtimgui doesn't pass modifiers through - see ImguiPanelWidget::applyKeyboardModifiers()
    const Qt::KeyboardModifiers mods = QGuiApplication::keyboardModifiers();
    ImGuiIO& io = ImGui::GetIO();

    const bool ctrl = mods.testFlag(Qt::ControlModifier);
    const bool shift = mods.testFlag(Qt::ShiftModifier);
    const bool alt = mods.testFlag(Qt::AltModifier);
    const bool super = mods.testFlag(Qt::MetaModifier);

    io.KeyCtrl = ctrl;
    io.KeyShift = shift;
    io.KeyAlt = alt;
    io.KeySuper = super;
    io.AddKeyEvent(ImGuiMod_Ctrl, ctrl);
    io.AddKeyEvent(ImGuiMod_Shift, shift);
    io.AddKeyEvent(ImGuiMod_Alt, alt);
    io.AddKeyEvent(ImGuiMod_Super, super);
}

void EventGraphWidget::paintGL()
{
    QtImGui::newFrame(m_imgui);

    if(!m_imguiConfigured){
        // Don't write imgui.ini into the working directory
        ImGui::GetIO().IniFilename = nullptr;
        m_imguiConfigured = true;
    }

    applyKeyboardModifiers();
    drawFrame();

    ImGui::Render();
    QtImGui::render(m_imgui);
}

void EventGraphWidget::drawFrame()
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("##eventgraphroot", nullptr,
                 ImGuiWindowFlags_NoDecoration
                     | ImGuiWindowFlags_NoMove
                     | ImGuiWindowFlags_NoBringToFrontOnFocus
                     | ImGuiWindowFlags_NoNavFocus
                     | ImGuiWindowFlags_NoSavedSettings);

    const ImVec2 available = ImGui::GetContentRegionAvail();

    if(!ensureEditor()){
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }

    EventGraphModel& model = m_controller->model();
    updateCaches();
    m_anchors.clear();

    ed::SetCurrentEditor(m_editor);

    // Must be before ed::Begin() - see ImguiPanelWidget::drawGrid()
    drawGrid(available);

    ed::Begin("EventGraph", ImVec2(0.0f, 0.0f));

    for(uint64_t nodeId : model.nodeIds()){
        drawNode(*model.node(nodeId));
    }
    drawLinks();
    drawContextMenus();

    if(m_navigateToNode != 0){
        ed::ClearSelection();
        ed::SelectNode(ed::NodeId(editorNodeId(m_navigateToNode)));
        ed::NavigateToSelection(false, 0.3f);
        m_navigateToNode = 0;
    }

    // The view is fit to the content on the first frame, before anything has a size
    if(m_frame == 3){
        ed::NavigateToContent(0.0f);
    }
    m_frame++;

    ed::End();

    syncNodePositions();
    drawHoveredLinkTooltip();

    ed::SetCurrentEditor(nullptr);

    ImGui::End();
    ImGui::PopStyleVar();

    drawToolbox(viewport);
}

void EventGraphWidget::drawGrid(ImVec2 size)
{
    // Our own background and grid - see the WORKAROUND in ensureEditor()
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(ImVec2(0.0f, 0.0f), size, IM_COL32(38, 38, 45, 255));

    const ImVec2 topLeft = ed::ScreenToCanvas(ImVec2(0.0f, 0.0f));
    const ImVec2 bottomRight = ed::ScreenToCanvas(size);

    constexpr float GRID_STEP = 32.0f;
    const ImU32 gridColor = IM_COL32(120, 120, 120, 40);
    for(float x = std::floor(topLeft.x / GRID_STEP) * GRID_STEP; x < bottomRight.x; x += GRID_STEP){
        drawList->AddLine(ed::CanvasToScreen(ImVec2(x, topLeft.y)),
                          ed::CanvasToScreen(ImVec2(x, bottomRight.y)), gridColor);
    }
    for(float y = std::floor(topLeft.y / GRID_STEP) * GRID_STEP; y < bottomRight.y; y += GRID_STEP){
        drawList->AddLine(ed::CanvasToScreen(ImVec2(topLeft.x, y)),
                          ed::CanvasToScreen(ImVec2(bottomRight.x, y)), gridColor);
    }
}

QString EventGraphWidget::nodeTitle(const EventGraphNode& node) const
{
    if(!node.name.isEmpty()){
        return node.name;
    }
    if(!node.model.isEmpty()){
        return node.model;
    }
    return formatNodeId(node.nodeId);
}

void EventGraphWidget::drawPinMarker(bool filled, ImU32 color)
{
    ImGui::Dummy(ImVec2(PIN_SIZE, PIN_SIZE));
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 center(min.x + PIN_SIZE / 2, min.y + PIN_SIZE / 2);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    if(filled){
        drawList->AddCircleFilled(center, PIN_SIZE / 2 - 1, color);
    }else{
        drawList->AddCircle(center, PIN_SIZE / 2 - 1.5f, color, 0, 1.5f);
    }
}

void EventGraphWidget::updateCaches()
{
    if(m_cacheVersion == m_modelVersion && m_cachedSearch == m_search.text()){
        return;
    }
    m_cacheVersion = m_modelVersion;
    m_cachedSearch = m_search.text();

    const EventGraphModel& model = m_controller->model();

    m_links = model.links();
    m_linkedPins.clear();
    for(const EventLink& link : m_links){
        m_linkedPins.insert(link.from);
        m_linkedPins.insert(link.to);
    }

    m_pinTrees.clear();
    m_groupInfo.clear();
    m_matches.clear();
    m_matchingNodes.clear();

    for(uint64_t nodeId : model.nodeIds()){
        const EventGraphNode& node = *model.node(nodeId);

        if(!m_search.isEmpty()){
            for(const EventPin& pin : node.pins){
                if(m_search.matches(pin)){
                    m_matches.insert(PinRef{nodeId, pin.space, pin.address});
                }
            }
        }

        QVector<PinTreeGroup>& tree = m_pinTrees[nodeId];
        tree = buildPinTree(node.pins);
        int nodeMatches = 0;
        for(const PinTreeGroup& group : tree){
            fillGroupInfo(nodeId, node, group);
            nodeMatches += m_groupInfo.value(groupKey(nodeId, group.path)).matches;
        }
        if(nodeMatches > 0){
            m_matchingNodes.push_back(nodeId);
        }
    }
}

void EventGraphWidget::fillGroupInfo(uint64_t nodeId, const EventGraphNode& node, const PinTreeGroup& group)
{
    GroupInfo info;
    for(int pinIndex : group.pins){
        const EventPin& pin = node.pins[pinIndex];
        const PinRef ref{nodeId, pin.space, pin.address};
        if(m_linkedPins.contains(ref)){
            info.linkedPins.push_back(ref);
        }
        if(m_matches.contains(ref)){
            info.matches++;
        }
    }

    for(const PinTreeGroup& child : group.children){
        fillGroupInfo(nodeId, node, child);
        const GroupInfo& childInfo = m_groupInfo[groupKey(nodeId, child.path)];
        info.linkedPins += childInfo.linkedPins;
        info.matches += childInfo.matches;
    }

    m_groupInfo.insert(groupKey(nodeId, group.path), info);
}

QString EventGraphWidget::groupKey(uint64_t nodeId, const QStringList& path) const
{
    // A character that won't be in a CDI name separates the parts
    return QString::number(nodeId, 16) + QChar(0x1f) + path.join(QChar(0x1f));
}

void EventGraphWidget::collectRows(const EventGraphNode& node, const PinTreeGroup& group, int depth, QVector<NodeRow>& rows)
{
    const QString key = groupKey(node.nodeId, group.path);
    const GroupInfo info = m_groupInfo.value(key);
    const bool searching = !m_search.isEmpty();

    if(m_onlyConnected && info.linkedPins.isEmpty() && info.matches == 0){
        // Nothing in here to show
        return;
    }

    NodeRow header;
    header.isGroup = true;
    header.depth = depth;
    header.group = &group;
    header.groupKey = key;
    // Groups with search matches are opened so that the matches can be seen
    header.expanded = m_expandedGroups.contains(key) || (searching && info.matches > 0);
    header.matched = info.matches > 0;
    rows.push_back(header);

    if(!header.expanded){
        // Lines to anything in here go to the group instead
        const Anchor anchor{editorGroupPinId(key, false), editorGroupPinId(key, true)};
        for(const PinRef& ref : info.linkedPins){
            m_anchors.insert(ref, anchor);
        }
        return;
    }

    for(int pinIndex : group.pins){
        const EventPin& pin = node.pins[pinIndex];
        const PinRef ref{node.nodeId, pin.space, pin.address};
        const bool matched = m_matches.contains(ref);
        if(m_onlyConnected && !m_linkedPins.contains(ref) && !matched){
            continue;
        }

        NodeRow row;
        row.depth = depth + 1;
        row.pinIndex = pinIndex;
        row.matched = matched;
        rows.push_back(row);
        m_anchors.insert(ref, Anchor{editorPinId(ref, false), editorPinId(ref, true)});
    }

    for(const PinTreeGroup& child : group.children){
        collectRows(node, child, depth + 1, rows);
    }
}

QString EventGraphWidget::groupRowLabel(const NodeRow& row) const
{
    const GroupInfo info = m_groupInfo.value(row.groupKey);
    if(info.matches > 0){
        return QString("%1 (%2, %3 matching)").arg(row.group->label).arg(row.group->totalPins).arg(info.matches);
    }
    return QString("%1 (%2)").arg(row.group->label).arg(row.group->totalPins);
}

void EventGraphWidget::drawGroupToggle(const NodeRow& row)
{
    ImGui::PushID(row.groupKey.toUtf8().constData());
    if(ImGui::InvisibleButton("##toggle", ImVec2(PIN_SIZE, PIN_SIZE))){
        if(m_expandedGroups.contains(row.groupKey)){
            m_expandedGroups.remove(row.groupKey);
        }else{
            m_expandedGroups.insert(row.groupKey);
        }
    }
    ImGui::PopID();

    // A triangle: pointing right when collapsed, down when expanded
    const ImVec2 min = ImGui::GetItemRectMin();
    const float pad = 2.0f;
    const ImU32 color = ImGui::IsItemHovered() ? IM_COL32(255, 255, 255, 255) : IM_COL32(190, 190, 190, 255);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    if(row.expanded){
        drawList->AddTriangleFilled(ImVec2(min.x + pad, min.y + pad + 1),
                                    ImVec2(min.x + PIN_SIZE - pad, min.y + pad + 1),
                                    ImVec2(min.x + PIN_SIZE / 2, min.y + PIN_SIZE - pad), color);
    }else{
        drawList->AddTriangleFilled(ImVec2(min.x + pad + 1, min.y + pad),
                                    ImVec2(min.x + PIN_SIZE - pad, min.y + PIN_SIZE / 2),
                                    ImVec2(min.x + pad + 1, min.y + PIN_SIZE - pad), color);
    }
}

void EventGraphWidget::drawNode(const EventGraphNode& node)
{
    const ed::NodeId nodeId(editorNodeId(node.nodeId));

    if(!m_seededNodes.contains(node.nodeId)){
        m_seededNodes.insert(node.nodeId);
        QPointF position;
        if(m_layout.positions.contains(node.nodeId)){
            position = m_layout.positions.value(node.nodeId);
        }else{
            // Put new nodes in a grid, out of the way of the ones that are already placed
            position = QPointF((m_autoPlaceCount % AUTO_PLACE_COLUMNS) * AUTO_PLACE_COLUMN_WIDTH,
                               (m_autoPlaceCount / AUTO_PLACE_COLUMNS) * AUTO_PLACE_ROW_HEIGHT);
            m_autoPlaceCount++;
            m_layout.positions[node.nodeId] = position;
        }
        ed::SetNodePosition(nodeId, ImVec2(position.x(), position.y()));
    }

    const bool nodeMatches = m_matchingNodes.contains(node.nodeId);
    if(nodeMatches){
        ed::PushStyleColor(ed::StyleColor_NodeBorder, COLOR_MATCH);
        ed::PushStyleVar(ed::StyleVar_NodeBorderWidth, 3.0f);
    }

    ed::BeginNode(nodeId);
    ImGui::PushID(static_cast<int>(editorNodeId(node.nodeId)));

    // Title
    ImGui::TextUnformatted(nodeTitle(node).toUtf8().constData());
    QString subtitle = formatNodeId(node.nodeId);
    if(!node.manufacturer.isEmpty() || !node.model.isEmpty()){
        subtitle = QString("%1 %2  %3").arg(node.manufacturer, node.model, subtitle);
    }
    ImGui::TextColored(COLOR_DIM, "%s", subtitle.toUtf8().constData());

    // How far along reading the node is
    const NodeLoadStatus status = m_controller->loadStatus(node.nodeId);
    switch(status.state){
    case NodeLoadState::WaitingForInfo:
        ImGui::TextColored(COLOR_DIM, "Waiting for node information...");
        break;
    case NodeLoadState::ReadingCdi:
        ImGui::TextColored(COLOR_DIM, "Reading configuration description... %d%%", status.progress);
        break;
    case NodeLoadState::ReadingMemory:
        ImGui::TextColored(COLOR_DIM, "Reading event IDs... %d%%", status.progress);
        break;
    case NodeLoadState::NoCdi:
        ImGui::TextColored(COLOR_DIM, "No configuration description");
        break;
    case NodeLoadState::Failed:
        ImGui::TextColored(COLOR_ERROR, "%s", status.error.toUtf8().constData());
        break;
    case NodeLoadState::Ready:
        break;
    }

    // The tree of groups and pins that are visible
    QVector<NodeRow> rows;
    for(const PinTreeGroup& group : m_pinTrees.value(node.nodeId)){
        collectRows(node, group, 0, rows);
    }

    // Columns are placed relative to the left edge of the node, in the same coordinates
    // that the node is drawn in.  Don't use SameLine(offset)/GetCursorPosX(): those
    // are relative to the ImGui window, which inside the node editor is not the same
    // space as the node, so the columns drift away from the node as it moves.
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float left = ImGui::GetCursorScreenPos().x;
    const float labelStart = left + PIN_SIZE + spacing;

    float labelExtent = 0.0f;
    for(const NodeRow& row : rows){
        float width = row.depth * TREE_INDENT;
        if(row.isGroup){
            width += PIN_SIZE + 4.0f + ImGui::CalcTextSize(groupRowLabel(row).toUtf8().constData()).x;
        }else{
            width += ImGui::CalcTextSize(node.pins[row.pinIndex].label.toUtf8().constData()).x;
        }
        labelExtent = std::max(labelExtent, width);
    }
    const float eventIdColumn = labelStart + labelExtent + spacing * 2;
    const float eventIdWidth = ImGui::CalcTextSize("00.00.00.00.00.00.00.00").x;
    const float outputColumn = eventIdColumn + eventIdWidth + spacing;
    const float rowHeight = std::max(PIN_SIZE, ImGui::GetTextLineHeight());

    for(const NodeRow& row : rows){
        const float rowY = ImGui::GetCursorScreenPos().y;
        const float x = labelStart + row.depth * TREE_INDENT;

        if(!row.isGroup && row.matched){
            // Drawn first so that it is behind the row
            ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(left - 2.0f, rowY - 1.0f),
                                                      ImVec2(outputColumn + PIN_SIZE + 2.0f, rowY + rowHeight + 1.0f),
                                                      COLOR_MATCH_BACKGROUND, 3.0f);
        }

        if(row.isGroup){
            // A collapsed group with connected pins in it gets the lines for them
            const bool anchor = !row.expanded && !m_groupInfo.value(row.groupKey).linkedPins.isEmpty();

            ImGui::SetCursorScreenPos(ImVec2(left, rowY));
            if(anchor){
                ed::BeginPin(ed::PinId(editorGroupPinId(row.groupKey, false)), ed::PinKind::Input);
                ed::PinPivotAlignment(ImVec2(0.0f, 0.5f));
                drawPinMarker(true, COLOR_UNKNOWN);
                ed::EndPin();
            }else{
                ImGui::Dummy(ImVec2(PIN_SIZE, PIN_SIZE));
            }

            ImGui::SameLine();
            ImGui::SetCursorScreenPos(ImVec2(x, ImGui::GetCursorScreenPos().y));
            drawGroupToggle(row);

            ImGui::SameLine();
            ImGui::SetCursorScreenPos(ImVec2(x + PIN_SIZE + 4.0f, ImGui::GetCursorScreenPos().y));
            const QByteArray label = groupRowLabel(row).toUtf8();
            if(row.matched){
                ImGui::TextColored(COLOR_MATCH, "%s", label.constData());
            }else{
                ImGui::TextUnformatted(label.constData());
            }

            if(anchor){
                ImGui::SameLine();
                ImGui::SetCursorScreenPos(ImVec2(outputColumn, ImGui::GetCursorScreenPos().y));
                ed::BeginPin(ed::PinId(editorGroupPinId(row.groupKey, true)), ed::PinKind::Output);
                ed::PinPivotAlignment(ImVec2(1.0f, 0.5f));
                drawPinMarker(true, COLOR_UNKNOWN);
                ed::EndPin();
            }
            continue;
        }

        const EventPin& pin = node.pins[row.pinIndex];
        const PinRef ref{node.nodeId, pin.space, pin.address};

        // Left side: where the event comes in, if this is a consumer
        ImGui::SetCursorScreenPos(ImVec2(left, rowY));
        ed::BeginPin(ed::PinId(editorPinId(ref, false)), ed::PinKind::Input);
        ed::PinPivotAlignment(ImVec2(0.0f, 0.5f));
        drawPinMarker(pin.direction == PinDirection::Consumer,
                      pin.direction == PinDirection::Unknown ? COLOR_UNKNOWN : COLOR_CONSUMER);
        ed::EndPin();

        ImGui::SameLine();
        ImGui::SetCursorScreenPos(ImVec2(x, ImGui::GetCursorScreenPos().y));
        ImGui::TextUnformatted(pin.label.toUtf8().constData());

        ImGui::SameLine();
        ImGui::SetCursorScreenPos(ImVec2(eventIdColumn, ImGui::GetCursorScreenPos().y));
        if(pin.eventId.has_value()){
            ImGui::TextColored(COLOR_DIM, "%s", formatEventId(pin.eventId.value()).toUtf8().constData());
        }else{
            ImGui::TextColored(COLOR_DIM, "...");
        }

        // Right side: where the event goes out, if this is a producer
        ImGui::SameLine();
        ImGui::SetCursorScreenPos(ImVec2(outputColumn, ImGui::GetCursorScreenPos().y));
        ed::BeginPin(ed::PinId(editorPinId(ref, true)), ed::PinKind::Output);
        ed::PinPivotAlignment(ImVec2(1.0f, 0.5f));
        drawPinMarker(pin.direction == PinDirection::Producer,
                      pin.direction == PinDirection::Unknown ? COLOR_UNKNOWN : COLOR_PRODUCER);
        ed::EndPin();
    }

    ImGui::PopID();
    ed::EndNode();

    if(nodeMatches){
        ed::PopStyleVar();
        ed::PopStyleColor();
    }
}

void EventGraphWidget::drawLinks()
{
    m_drawnLinks.clear();

    for(const EventLink& link : m_links){
        auto from = m_anchors.constFind(link.from);
        auto to = m_anchors.constFind(link.to);
        if(from == m_anchors.constEnd() || to == m_anchors.constEnd()){
            continue;
        }
        if(from->output == to->output){
            // Both ends are in the same collapsed group
            continue;
        }

        const uint64_t linkId = editorLinkId(link);
        m_drawnLinks.insert(linkId, link);

        // Links where we don't know who sends the event are drawn fainter
        ed::Link(ed::LinkId(linkId),
                 ed::PinId(from->output),
                 ed::PinId(to->input),
                 colorForEvent(link.eventId, link.directed ? 1.0f : 0.55f),
                 link.directed ? 2.5f : 1.5f);
    }
}

void EventGraphWidget::drawContextMenus()
{
    // Popups must be opened/drawn with the editor suspended(see the node editor's blueprints example)
    ed::Suspend();
    ed::NodeId contextNode;
    if(ed::ShowNodeContextMenu(&contextNode)){
        m_contextNodeId = 0;
        for(auto it = m_nodeIndex.constBegin(); it != m_nodeIndex.constEnd(); ++it){
            if(it.value() + 1 == contextNode.Get()){
                m_contextNodeId = it.key();
            }
        }
        ImGui::OpenPopup("Node context menu");
    }

    if(ImGui::BeginPopup("Node context menu")){
        const EventGraphNode* node = m_controller->model().node(m_contextNodeId);
        if(node){
            ImGui::TextUnformatted(nodeTitle(*node).toUtf8().constData());
            ImGui::Separator();
            if(ImGui::MenuItem("Re-read node")){
                m_controller->reloadNode(m_contextNodeId);
            }
        }
        ImGui::EndPopup();
    }
    ed::Resume();
}

void EventGraphWidget::drawHoveredLinkTooltip()
{
    ed::LinkId hovered = ed::GetHoveredLink();
    if(!hovered){
        return;
    }

    auto it = m_drawnLinks.constFind(hovered.Get());
    if(it == m_drawnLinks.constEnd()){
        return;
    }

    const EventGraphModel& model = m_controller->model();
    const EventGraphNode* fromNode = model.node(it->from.nodeId);
    const EventGraphNode* toNode = model.node(it->to.nodeId);
    const EventPin* fromPin = model.pin(it->from);
    const EventPin* toPin = model.pin(it->to);
    if(!fromNode || !toNode || !fromPin || !toPin){
        return;
    }

    ed::Suspend();
    ImGui::BeginTooltip();
    ImGui::Text("Event %s", formatEventId(it->eventId).toUtf8().constData());
    ImGui::Text("%s: %s", nodeTitle(*fromNode).toUtf8().constData(), fromPin->path().toUtf8().constData());
    ImGui::Text("%s %s: %s", it->directed ? "->" : "--",
                nodeTitle(*toNode).toUtf8().constData(), toPin->path().toUtf8().constData());
    ImGui::EndTooltip();
    ed::Resume();
}

void EventGraphWidget::drawToolbox(const ImGuiViewport* viewport)
{
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + 12.0f, viewport->Pos.y + 12.0f), ImGuiCond_FirstUseEver);
    if(ImGui::Begin("Event graph", nullptr, ImGuiWindowFlags_AlwaysAutoResize)){
        const EventGraphModel& model = m_controller->model();
        int loading = 0;
        for(uint64_t nodeId : model.nodeIds()){
            NodeLoadState state = m_controller->loadStatus(nodeId).state;
            if(state == NodeLoadState::WaitingForInfo ||
                    state == NodeLoadState::ReadingCdi ||
                    state == NodeLoadState::ReadingMemory){
                loading++;
            }
        }

        if(loading > 0){
            ImGui::Text("%d nodes, reading %d", static_cast<int>(model.nodeIds().size()), loading);
        }else{
            ImGui::Text("%d nodes", static_cast<int>(model.nodeIds().size()));
        }

        if(ImGui::Button("Find nodes")){
            m_controller->start();
        }
        ImGui::SameLine();
        if(ImGui::Button("Fit to view") && m_editor){
            ed::SetCurrentEditor(m_editor);
            ed::NavigateToContent();
            ed::SetCurrentEditor(nullptr);
        }
        ImGui::SameLine();
        if(ImGui::Button("Collapse all")){
            m_expandedGroups.clear();
        }
        ImGui::Checkbox("Only show connected events", &m_onlyConnected);

        ImGui::Separator();
        ImGui::SetNextItemWidth(260.0f);
        if(ImGui::InputTextWithHint("##search", "Search: event name or ID", m_searchBuffer, sizeof(m_searchBuffer))){
            m_search = EventSearch(QString::fromUtf8(m_searchBuffer));
            m_matchCursor = -1;
        }

        if(!m_search.isEmpty()){
            if(m_matches.isEmpty()){
                ImGui::TextColored(COLOR_DIM, "No matching events");
            }else{
                ImGui::Text("%d matching events in %d nodes",
                            static_cast<int>(m_matches.size()), static_cast<int>(m_matchingNodes.size()));

                const int count = m_matchingNodes.size();
                if(ImGui::Button("Previous")){
                    m_matchCursor = m_matchCursor <= 0 ? count - 1 : m_matchCursor - 1;
                    m_navigateToNode = m_matchingNodes[m_matchCursor];
                }
                ImGui::SameLine();
                if(ImGui::Button("Next")){
                    m_matchCursor = (m_matchCursor + 1) % count;
                    m_navigateToNode = m_matchingNodes[m_matchCursor];
                }
                if(m_matchCursor >= 0 && m_matchCursor < count){
                    ImGui::SameLine();
                    ImGui::Text("%d of %d", m_matchCursor + 1, count);
                }
            }
        }
    }
    ImGui::End();
}

void EventGraphWidget::syncNodePositions()
{
    // The user moves the nodes in the node editor - keep our copy up to date so it gets saved
    for(auto it = m_nodeIndex.constBegin(); it != m_nodeIndex.constEnd(); ++it){
        if(!m_seededNodes.contains(it.key())){
            continue;
        }

        const ImVec2 pos = ed::GetNodePosition(ed::NodeId(it.value() + 1));
        const QPointF position(pos.x, pos.y);
        if(m_layout.positions.value(it.key()) != position){
            m_layout.positions[it.key()] = position;
            m_saveTimer.start();
        }
    }
}

void EventGraphWidget::saveLayout()
{
    m_layout.learnedDirections = m_controller->model().learnedDirections();
    if(!EventGraphStorage::save(m_layout, m_connectionName)){
        LOG4CXX_ERROR_FMT(logger, "Unable to save event graph for {}", m_connectionName.toStdString());
    }
}

uint64_t EventGraphWidget::editorNodeId(uint64_t nodeId)
{
    auto it = m_nodeIndex.constFind(nodeId);
    if(it == m_nodeIndex.constEnd()){
        it = m_nodeIndex.insert(nodeId, m_nodeIndex.size());
    }
    return it.value() + 1;
}

uint64_t EventGraphWidget::editorPinId(const PinRef& pin, bool output)
{
    auto it = m_pinIndex.constFind(pin);
    if(it == m_pinIndex.constEnd()){
        it = m_pinIndex.insert(pin, m_pinIndex.size());
    }
    return PIN_ID_BASE + static_cast<uint64_t>(it.value()) * 2 + (output ? 1 : 0);
}

uint64_t EventGraphWidget::editorGroupPinId(const QString& key, bool output)
{
    auto it = m_groupIndex.constFind(key);
    if(it == m_groupIndex.constEnd()){
        it = m_groupIndex.insert(key, m_groupIndex.size());
    }
    return GROUP_PIN_ID_BASE + static_cast<uint64_t>(it.value()) * 2 + (output ? 1 : 0);
}

uint64_t EventGraphWidget::editorLinkId(const EventLink& link)
{
    // Only valid until there are 2^20 pins
    const uint64_t from = (editorPinId(link.from, true) - PIN_ID_BASE) / 2;
    const uint64_t to = (editorPinId(link.to, true) - PIN_ID_BASE) / 2;
    return LINK_ID_BASE + (from << 20) + to;
}
