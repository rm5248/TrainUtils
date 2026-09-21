#include <cmath>
#include <numbers>

#include "panelmodel.h"

namespace {

// Rotate p about (cw/2, ch/2) by rotationDegrees, matching QTransform::rotate()'s
// convention (screen/Y-down coordinates, positive = clockwise) -- the same
// convention TurnoutDisplay::paintEvent()/updateConnectionPoints() used.
ImVec2 rotateAboutContentCenter(ImVec2 p, double rotationDegrees, float cw, float ch){
    if(rotationDegrees == 0.0){
        return p;
    }
    const float cx = cw / 2.0f;
    const float cy = ch / 2.0f;
    const double rad = rotationDegrees * std::numbers::pi / 180.0;
    const float cosA = static_cast<float>(std::cos(rad));
    const float sinA = static_cast<float>(std::sin(rad));
    const float dx = p.x - cx;
    const float dy = p.y - cy;
    return ImVec2(cx + dx * cosA - dy * sinA,
                  cy + dx * sinA + dy * cosA);
}

// Applies, in order, the same transform stack the QWidget implementation
// applied (turnoutdisplay.cpp): rotate about the content centre, then mirror
// vertically for a Left-handed turnout, then place within the node's padded
// bounding box.
ImVec2 turnoutLocalToNode(ImVec2 p, TurnoutHand hand, double rotationDegrees){
    constexpr float cw = kTurnoutContentSize;
    constexpr float ch = kTurnoutContentSize;

    p = rotateAboutContentCenter(p, rotationDegrees, cw, ch);

    if(hand == TurnoutHand::Left){
        p.y = ch - p.y;
    }

    // Centre the cw x ch content within the padded, fixed-size node box (see
    // kNodeSize in imguipanelwidget.h -- currently 72, giving 11px of padding
    // on each side so a full rotation never clips the bounding box).
    constexpr float kNodeSize = 72.0f;
    constexpr float contentOffset = (kNodeSize - cw) / 2.0f;
    p.x += contentOffset;
    p.y += contentOffset;
    return p;
}

} // namespace

TurnoutGeometry turnoutGeometry(TurnoutHand hand, double rotationDegrees){
    constexpr float cw = kTurnoutContentSize;
    constexpr float ch = kTurnoutContentSize;

    TurnoutGeometry geo;

    // Through leg.
    geo.legs[0][0] = turnoutLocalToNode(ImVec2(0.0f, ch / 4.0f), hand, rotationDegrees);
    geo.legs[0][1] = turnoutLocalToNode(ImVec2(cw, ch / 4.0f), hand, rotationDegrees);

    // Diverging diagonal.
    ImVec2 divergeStart(cw / 3.0f, ch / 4.0f);
    ImVec2 divergeEnd(cw / 2.0f + cw / 4.0f, ch / 2.0f + ch / 4.0f);
    geo.legs[1][0] = turnoutLocalToNode(divergeStart, hand, rotationDegrees);
    geo.legs[1][1] = turnoutLocalToNode(divergeEnd, hand, rotationDegrees);

    // Diverging outgoing leg.
    geo.legs[2][0] = turnoutLocalToNode(divergeEnd, hand, rotationDegrees);
    geo.legs[2][1] = turnoutLocalToNode(ImVec2(cw, ch / 2.0f + ch / 4.0f), hand, rotationDegrees);

    // Anchor points: 0 incoming, 1 normal outgoing, 2 diverged outgoing.
    geo.pins[0] = turnoutLocalToNode(ImVec2(0.0f, ch / 4.0f), hand, rotationDegrees);
    geo.pins[1] = turnoutLocalToNode(ImVec2(cw, ch / 4.0f), hand, rotationDegrees);
    geo.pins[2] = turnoutLocalToNode(ImVec2(cw, ch / 2.0f + ch / 4.0f), hand, rotationDegrees);

    return geo;
}

TurnoutNode& PanelModel::addTurnout(ImVec2 position){
    TurnoutNode node;
    node.id = m_nextId++;
    node.name = QString("Turnout %1").arg(node.id);
    node.position = position;
    m_turnouts.push_back(node);
    return m_turnouts.back();
}

TurnoutNode& PanelModel::addTurnoutWithId(PanelItemId id, ImVec2 position){
    TurnoutNode node;
    node.id = id;
    node.name = QString("Turnout %1").arg(id);
    node.position = position;
    m_turnouts.push_back(node);
    if(id >= m_nextId){
        m_nextId = id + 1;
    }
    return m_turnouts.back();
}

TurnoutNode* PanelModel::findTurnout(PanelItemId id){
    for(TurnoutNode& node : m_turnouts){
        if(node.id == id){
            return &node;
        }
    }
    return nullptr;
}

void PanelModel::removeTurnout(PanelItemId id){
    for(auto it = m_turnouts.begin(); it != m_turnouts.end(); ++it){
        if(it->id == id){
            m_turnouts.erase(it);
            break;
        }
    }

    // A segment attached to a node that no longer exists has nowhere sensible
    // to anchor itself, so it goes too, rather than silently collapsing to (0,0).
    for(auto it = m_segments.begin(); it != m_segments.end();){
        if(it->a.nodeId == id || it->b.nodeId == id){
            it = m_segments.erase(it);
        }else{
            ++it;
        }
    }
}

TrackSegmentEdge& PanelModel::addSegment(SegmentEnd a, SegmentEnd b){
    TrackSegmentEdge seg;
    seg.id = m_nextId++;
    seg.a = a;
    seg.b = b;
    m_segments.push_back(seg);
    return m_segments.back();
}

TrackSegmentEdge& PanelModel::addSegmentWithId(PanelItemId id, SegmentEnd a, SegmentEnd b){
    TrackSegmentEdge seg;
    seg.id = id;
    seg.a = a;
    seg.b = b;
    m_segments.push_back(seg);
    if(id >= m_nextId){
        m_nextId = id + 1;
    }
    return m_segments.back();
}

TrackSegmentEdge* PanelModel::findSegment(PanelItemId id){
    for(TrackSegmentEdge& seg : m_segments){
        if(seg.id == id){
            return &seg;
        }
    }
    return nullptr;
}

void PanelModel::removeSegment(PanelItemId id){
    for(auto it = m_segments.begin(); it != m_segments.end(); ++it){
        if(it->id == id){
            m_segments.erase(it);
            return;
        }
    }
}

ImVec2 resolveSegmentEnd(const PanelModel& model, const SegmentEnd& end){
    if(end.nodeId == 0){
        return end.freePos;
    }
    for(const TurnoutNode& node : model.turnouts()){
        if(node.id == end.nodeId){
            const TurnoutGeometry geo = turnoutGeometry(node.hand, node.rotationDegrees);
            const ImVec2& pin = geo.pins[end.pinIndex];
            return ImVec2(node.position.x + kNodeContentPadding + pin.x,
                           node.position.y + kNodeContentPadding + pin.y);
        }
    }
    // Referenced node is gone (shouldn't normally happen -- removeTurnout()
    // cascades -- but fall back to something rather than an uninitialized point).
    return end.freePos;
}

void PanelModel::clear(){
    m_turnouts.clear();
    m_segments.clear();
    m_nextId = 1;
}
