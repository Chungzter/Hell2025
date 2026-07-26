#include "EditorHierarchy.h"

#include "EditorCoordinates.h"
#include "EditorLayout.h"
#include "EditorScrollBar.h"
#include "EditorSelection.h"
#include "EditorStyle.h"
#include "EditorUI.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Common/Constants.h"
#include "Hell/Input.h"
#include "Hell/UI/UIBackEnd.h"

#include "Unloved/Common/Constants.h"
#include "Unloved/ObjectId.h"
#include "Unloved/Objects/House/Wall.h"
#include "Unloved/Objects/House/WorldPlane.h"
#include "Unloved/Objects/Props/Christmas/ChristmasLights.h"
#include "Unloved/World/World.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace Unloved::EditorSession::Hierarchy {
    namespace {
        constexpr int32_t ROW_HEIGHT = 24;
        constexpr int32_t ROWS_PER_SCROLL = 3;
        constexpr int32_t SCROLL_BAR_WIDTH = 10;
        constexpr int32_t SCROLL_BAR_RIGHT_PADDING = 7;
        constexpr int32_t LEFT_PADDING = 8;
        constexpr int32_t INDENT_WIDTH = 14;
        constexpr int32_t ARROW_SIZE = 8;
        constexpr int32_t TEXT_GAP = 6;

        const glm::vec4 TEXT_COLOR = glm::vec4(0.545098f, 0.541176f, 0.568627f, 1.0f);
        const glm::vec4 HOVER_COLOR = glm::vec4(0.141176f, 0.125490f, 0.168627f, 1.0f);
        const glm::vec4 SELECTED_COLOR = glm::vec4(0.231373f, 0.196078f, 0.286275f, 1.0f);

        struct HierarchyNode {
            std::string label;
            uint64_t itemId = 0;
            bool selectable = false;
            bool expanded = false;
            std::vector<HierarchyNode> children;
            int32_t christmasLightPointIndex = -1;
            int32_t wallSegmentIndex = -1;
        };

        struct VisibleRow {
            HierarchyNode* node = nullptr;
            uint32_t depth = 0;
        };

        HierarchyNode g_root;
        std::vector<VisibleRow> g_visibleRows;
        HierarchyNode* g_hoveredNode = nullptr;
        EditorScrollBar g_scrollBar;
        bool g_wantsMouseCapture = false;
        bool g_expandHierachyOnLoad = false;

        HierarchyNode& AddObjectNode(HierarchyNode& group, uint64_t objectId) {
            HierarchyNode& node = group.children.emplace_back();
            const std::string& editorName = World::GetEditorNameById(objectId);

            node.label = editorName.empty() || editorName == UNDEFINED_STRING ? std::to_string(objectId) : editorName;
            node.itemId = objectId;
            node.selectable = true;
            return node;
        }

        void AddWorldGroup(const char* label, const std::vector<uint64_t>& objectIds) {
            if (objectIds.empty()) return;

            HierarchyNode group;
            group.label = label;
            group.expanded = g_expandHierachyOnLoad;
            group.children.reserve(objectIds.size());

            for (uint64_t objectId : objectIds) {
                AddObjectNode(group, objectId);
            }

            g_root.children.push_back(std::move(group));
        }

        void AddWorldPlaneGroup(const char* label, WorldPlaneType type) {
            HierarchyNode group;
            group.label = label;
            group.expanded = g_expandHierachyOnLoad;
            group.children.reserve(World::GetWorldPlanes().size());

            for (uint64_t objectId : World::GetWorldPlanes().ids()) {
                WorldPlane* worldPlane = World::GetWorldPlaneByObjectId(objectId);
                if (worldPlane && worldPlane->GetType() == type) AddObjectNode(group, objectId);
            }

            if (!group.children.empty()) g_root.children.push_back(std::move(group));
        }

        void AddWallGroup() {
            const std::vector<uint64_t>& wallIds = World::GetWalls().ids();
            if (wallIds.empty()) return;

            HierarchyNode group;
            group.label = "Walls";
            group.expanded = g_expandHierachyOnLoad;
            group.children.reserve(wallIds.size());

            for (uint64_t wallId : wallIds) {
                Wall* wall = World::GetWallByObjectId(wallId);
                if (!wall) continue;

                HierarchyNode& wallNode = AddObjectNode(group, wallId);
                wallNode.expanded = g_expandHierachyOnLoad;
                wallNode.children.reserve(wall->GetWallSegments().size());

                for (size_t i = 0; i < wall->GetWallSegments().size(); i++) {
                    HierarchyNode& segmentNode = wallNode.children.emplace_back();
                    segmentNode.label = "Wall Segment " + std::to_string(i + 1);
                    segmentNode.itemId = wallId;
                    segmentNode.selectable = true;
                    segmentNode.wallSegmentIndex = static_cast<int32_t>(i);
                }
            }

            if (!group.children.empty()) g_root.children.push_back(std::move(group));
        }

        void AddChristmasLightPoints(HierarchyNode& christmasLightsNode, const ChristmasLightSet& christmasLights) {
            const std::vector<SequencePoint>& sequencePoints = christmasLights.GetCreateInfo().sequencePoints;
            christmasLightsNode.children.reserve(sequencePoints.size());

            for (size_t i = 0; i < sequencePoints.size(); i++) {
                HierarchyNode& pointNode = christmasLightsNode.children.emplace_back();
                pointNode.label = "Point " + std::to_string(i + 1);
                pointNode.itemId = christmasLights.GetObjectId();
                pointNode.selectable = true;
                pointNode.christmasLightPointIndex = static_cast<int32_t>(i);
            }
        }

        void AddChristmasLightGroup() {
            const std::vector<uint64_t>& christmasLightIds = World::GetChristmasLightSets().ids();
            if (christmasLightIds.empty()) return;

            HierarchyNode group;
            group.label = "Christmas Lights";
            group.expanded = g_expandHierachyOnLoad;
            group.children.reserve(christmasLightIds.size());

            for (uint64_t objectId : christmasLightIds) {
                ChristmasLightSet* christmasLights = World::GetChristmasLightsByObjectId(objectId);
                if (!christmasLights) continue;

                HierarchyNode& christmasLightsNode = AddObjectNode(group, objectId);
                christmasLightsNode.expanded = g_expandHierachyOnLoad;
                AddChristmasLightPoints(christmasLightsNode, *christmasLights);
            }

            if (!group.children.empty()) g_root.children.push_back(std::move(group));
        }

        int32_t GetVisibleRowCapacity() {
            return std::max(0, Layout::GetHierarchyContentRect().height / ROW_HEIGHT);
        }

        void GatherVisibleRows(HierarchyNode& node, uint32_t depth) {
            g_visibleRows.push_back({ &node, depth });
            if (!node.expanded) return;

            for (HierarchyNode& child : node.children) {
                GatherVisibleRows(child, depth + 1);
            }
        }

        void RefreshVisibleRows() {
            g_visibleRows.clear();
            GatherVisibleRows(g_root, 0);

            const int32_t maximumFirstRow = std::max(0, static_cast<int32_t>(g_visibleRows.size()) - GetVisibleRowCapacity());
            g_scrollBar.value = std::clamp(g_scrollBar.value, 0, maximumFirstRow);
        }

        EditorRect GetScrollBarRect() {
            const EditorRect contentRect = Layout::GetHierarchyContentRect();
            return { contentRect.Right() - SCROLL_BAR_RIGHT_PADDING - SCROLL_BAR_WIDTH, contentRect.y, SCROLL_BAR_WIDTH, contentRect.height };
        }

        EditorRect GetRowRect(size_t rowIndex) {
            const EditorRect contentRect = Layout::GetHierarchyContentRect();
            const int32_t visibleIndex = static_cast<int32_t>(rowIndex) - g_scrollBar.value;
            const int32_t scrollBarWidth = g_scrollBar.visible ? SCROLL_BAR_WIDTH + SCROLL_BAR_RIGHT_PADDING : 0;
            return { contentRect.x + 1, contentRect.y + visibleIndex * ROW_HEIGHT, std::max(0, contentRect.width - scrollBarWidth - 2), ROW_HEIGHT };
        }

        EditorRect GetArrowRect(const EditorRect& rowRect, uint32_t depth) {
            const int32_t x = rowRect.x + LEFT_PADDING + static_cast<int32_t>(depth) * INDENT_WIDTH;
            return { x, rowRect.y + (ROW_HEIGHT - ARROW_SIZE) / 2, ARROW_SIZE, ARROW_SIZE };
        }

        int32_t FindHoveredRow(const glm::ivec2& mousePosition) {
            const int32_t lastVisibleRow = std::min(static_cast<int32_t>(g_visibleRows.size()), g_scrollBar.value + GetVisibleRowCapacity());

            for (int32_t i = g_scrollBar.value; i < lastVisibleRow; i++) {
                if (GetRowRect(i).Contains(mousePosition)) return i;
            }

            return -1;
        }

        void RefreshHover(const glm::ivec2& mousePosition) {
            const int32_t hoveredRow = FindHoveredRow(mousePosition);
            g_hoveredNode = hoveredRow >= 0 ? g_visibleRows[hoveredRow].node : nullptr;
        }

        void HandleMousePress(const glm::ivec2& mousePosition) {
            const int32_t hoveredRow = FindHoveredRow(mousePosition);
            if (hoveredRow < 0) return;

            VisibleRow& row = g_visibleRows[hoveredRow];
            HierarchyNode& node = *row.node;
            const bool clickedArrow = !node.children.empty() && GetArrowRect(GetRowRect(hoveredRow), row.depth).Contains(mousePosition);

            // Groups toggle from the whole row but selectable bones keep the row for selection
            if (clickedArrow || (!node.selectable && !node.children.empty())) {
                node.expanded = !node.expanded;
                RefreshVisibleRows();
                RefreshHover(mousePosition);
                return;
            }

            if (node.christmasLightPointIndex >= 0) Selection::SelectChristmasLightPoint(node.itemId, node.christmasLightPointIndex);
            else if (node.wallSegmentIndex >= 0) Selection::SelectWallSegment(node.itemId, node.wallSegmentIndex);
            else if (node.selectable) Selection::SelectObject(node.itemId);
        }
    }

    void Init() {
        g_root = { "Scene", 0, false, true, {} };
        g_scrollBar = {};
        g_hoveredNode = nullptr;
        g_wantsMouseCapture = false;
        RefreshVisibleRows();
    }

    void Refresh() {
        g_root = { "Scene", 0, false, true, {} };
        g_root.children.reserve(23);

        AddWorldPlaneGroup("Ceilings", WorldPlaneType::CEILING);
        AddWorldGroup("Christmas Trees", World::GetChristmasTrees().ids());
        AddChristmasLightGroup();
        AddWorldGroup("DDGI Volumes", World::GetDDGIVolumes().ids());
        AddWorldGroup("Dobermann", World::GetDobermanns().ids());
        AddWorldGroup("Doors", World::GetDoors().ids());
        AddWorldGroup("Fences", World::GetFences().ids());
        AddWorldGroup("Fireplaces", World::GetFireplaces().ids());
        AddWorldPlaneGroup("Floors", WorldPlaneType::FLOOR);
        AddWorldGroup("Generic Objects", World::GetGenericObjects().ids());
        AddWorldGroup("Jetties", World::GetJetties().ids());
        AddWorldGroup("Kangaroos", World::GetKangaroos().ids());
        AddWorldGroup("Ladders", World::GetLadders().ids());
        AddWorldGroup("Lights", World::GetLightIds());
        AddWorldGroup("Mermaids", World::GetMermaids().ids());
        AddWorldGroup("Pick Ups", World::GetPickUps().ids());
        AddWorldGroup("Picture Frames", World::GetPictureFrames().ids());
        AddWorldGroup("Pianos", World::GetPianos().ids());
        AddWorldGroup("Power Pole Sets", World::GetPowerPoleSets().ids());
        AddWorldGroup("Staircases", World::GetStaircases().ids());
        AddWorldGroup("Sharks", World::GetSharks().ids());
        AddWallGroup();
        AddWorldGroup("Windows", World::GetWindows().ids());

        g_scrollBar.value = 0;
        g_hoveredNode = nullptr;
        RefreshVisibleRows();
    }

    void RefreshChristmasLightPoints(uint64_t objectId) {
        ChristmasLightSet* christmasLights = World::GetChristmasLightsByObjectId(objectId);
        if (!christmasLights) return;

        for (HierarchyNode& group : g_root.children) {
            for (HierarchyNode& node : group.children) {
                if (node.itemId != objectId || node.christmasLightPointIndex >= 0) continue;

                node.children.clear();
                AddChristmasLightPoints(node, *christmasLights);
                RefreshVisibleRows();
                return;
            }
        }
    }

    void RemoveObject(uint64_t objectId) {
        for (auto groupIt = g_root.children.begin(); groupIt != g_root.children.end(); groupIt++) {
            std::vector<HierarchyNode>& nodes = groupIt->children;
            const auto objectIt = std::find_if(nodes.begin(), nodes.end(), [objectId](const HierarchyNode& node) { return node.itemId == objectId && node.christmasLightPointIndex < 0 && node.wallSegmentIndex < 0; });
            if (objectIt == nodes.end()) continue;

            nodes.erase(objectIt);
            if (nodes.empty()) g_root.children.erase(groupIt);
            g_hoveredNode = nullptr;
            RefreshVisibleRows();
            return;
        }
    }

    void Update(bool allowInput) {
        const glm::ivec2 mousePosition = Coordinates::GetMousePositionUI();
        const EditorRect& panelRect = Layout::GetHierarchyPanel().rect;
        const EditorRect contentRect = Layout::GetHierarchyContentRect();

        g_wantsMouseCapture = panelRect.Contains(mousePosition);
        g_hoveredNode = nullptr;
        RefreshVisibleRows();

        if (allowInput && contentRect.Contains(mousePosition)) {
            if (Hell::Input::MouseWheelUp()) {
                g_scrollBar.value -= ROWS_PER_SCROLL;
            }
            else if (Hell::Input::MouseWheelDown()) {
                g_scrollBar.value += ROWS_PER_SCROLL;
            }
        }

        ScrollBar::Update(g_scrollBar, GetScrollBarRect(), static_cast<int32_t>(g_visibleRows.size()), GetVisibleRowCapacity(), allowInput);
        g_wantsMouseCapture = g_wantsMouseCapture || ScrollBar::WantsMouseCapture(g_scrollBar);

        if (!allowInput || ScrollBar::WantsMouseCapture(g_scrollBar) || !contentRect.Contains(mousePosition)) return;

        Hell::BackEnd::SetCursor(HELL_CURSOR_ARROW);
        RefreshHover(mousePosition);

        if (Hell::Input::LeftMousePressed()) HandleMousePress(mousePosition);
    }

    void Render() {
        RefreshVisibleRows();

        const int32_t lastVisibleRow = std::min(static_cast<int32_t>(g_visibleRows.size()), g_scrollBar.value + GetVisibleRowCapacity());
        for (int32_t i = g_scrollBar.value; i < lastVisibleRow; i++) {
            const VisibleRow& row = g_visibleRows[i];
            const HierarchyNode& node = *row.node;
            const EditorRect rowRect = GetRowRect(i);
            std::string label = node.label;
            // Sub items use their own labels
            if (node.selectable && node.christmasLightPointIndex < 0 && node.wallSegmentIndex < 0) {
                const std::string& editorName = World::GetEditorNameById(node.itemId);
                label = editorName.empty() || editorName == UNDEFINED_STRING ? std::to_string(node.itemId) : editorName;
            }

            const bool pointSelected = node.christmasLightPointIndex >= 0 && Selection::HasSelectedChristmasLightPoint() && node.itemId == Selection::GetSelectedObjectId() && node.christmasLightPointIndex == Selection::GetSelectedChristmasLightPointIndex();
            const bool wallSegmentSelected = node.wallSegmentIndex >= 0 && Selection::HasSelectedWallSegment() && node.itemId == Selection::GetSelectedObjectId() && node.wallSegmentIndex == Selection::GetSelectedWallSegmentIndex();
            const bool objectSelected = node.christmasLightPointIndex < 0 && node.wallSegmentIndex < 0 && !Selection::HasSelectedChristmasLightPoint() && !Selection::HasSelectedWallSegment() && node.itemId == Selection::GetSelectedObjectId();
            if (node.selectable && (pointSelected || wallSegmentSelected || objectSelected)) {
                UI::DrawSolidRect(rowRect, SELECTED_COLOR);
            }
            else if (row.node == g_hoveredNode) {
                UI::DrawSolidRect(rowRect, HOVER_COLOR);
            }

            const EditorRect arrowRect = GetArrowRect(rowRect, row.depth);
            if (!node.children.empty()) {
                const float rotation = node.expanded ? 0.0f : HELL_PI * -0.5f;
                UIBackEnd::BlitTexture(UICanvas::NATIVE, "DropDownArrow", glm::ivec2(arrowRect.x + arrowRect.width / 2, arrowRect.y + arrowRect.height / 2), Alignment::CENTERED, TEXT_COLOR, glm::ivec2(ARROW_SIZE), TextureFilter::NEAREST, rotation);
            }

            const int32_t textX = arrowRect.Right() + TEXT_GAP;
            UIBackEnd::BlitText(UICanvas::NATIVE, std::string(Style::TEXT_COLOR_TAG) + label, Style::FONT_NAME, glm::ivec2(textX, rowRect.y + rowRect.height / 2), Alignment::CENTERED_VERTICAL, Style::FONT_SCALE, TextureFilter::NEAREST);
        }

        ScrollBar::Render(g_scrollBar);
    }

    bool WantsMouseCapture() {
        return g_wantsMouseCapture;
    }
}
