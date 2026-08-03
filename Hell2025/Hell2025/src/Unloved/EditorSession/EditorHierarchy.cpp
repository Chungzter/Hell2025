#include "EditorHierarchy.h"

#include "Unloved/EditorSession/UI/EditorLayout.h"
#include "Unloved/EditorSession/Interaction/EditorSelection.h"
#include "EditorSession.h"
#include "Unloved/EditorSession/UI/EditorStyle.h"
#include "Unloved/EditorSession/UI/EditorUI.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Common/Constants.h"
#include "Hell/Input.h"
#include "Hell/UI/UIBackEnd.h"

#include "Unloved/Common/Constants.h"
#include "Unloved/ObjectId.h"
#include "Unloved/Objects/House/Wall.h"
#include "Unloved/Objects/House/WorldPlane.h"
#include "Unloved/Objects/House/PlanarQuadObject.h"
#include "Unloved/Objects/House/PointPairObject.h"
#include "Unloved/Objects/Props/Christmas/ChristmasLights.h"
#include "Unloved/World/World.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace Unloved::EditorSession::Hierarchy {
    namespace {
        constexpr int32_t ROWS_PER_SCROLL = 3;

        struct HierarchyNode {
            std::string label;
            uint64_t itemId = 0;
            bool selectable = false;
            bool expanded = false;
            bool collapsible = true;
            bool workspace = false;
            std::vector<HierarchyNode> children;
            int32_t pointIndex = -1;
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

        const char* GetWorkspaceLabel() {
            return Unloved::EditorSession::GetMode() == EditorSessionMode::MAP ? "Map Settings" : "House Settings";
        }

        void ResetRoot() {
            g_root = {};
            g_root.expanded = true;
            g_root.collapsible = false;
            if (!Unloved::EditorSession::HasMode()) return;

            g_root.children.reserve(2);

            HierarchyNode& workspaceNode = g_root.children.emplace_back();
            workspaceNode.label = GetWorkspaceLabel();
            workspaceNode.selectable = true;
            workspaceNode.collapsible = false;
            workspaceNode.workspace = true;

            HierarchyNode& sceneNode = g_root.children.emplace_back();
            sceneNode.label = "Scene";
            sceneNode.expanded = true;
            sceneNode.children.reserve(25);
        }

        HierarchyNode& GetSceneNode() {
            return g_root.children.back();
        }

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

            GetSceneNode().children.push_back(std::move(group));
        }

        void AddSpawnPointGroup() {
            const std::vector<uint64_t>& campaignIds = World::GetSpawnPointsCampaign().ids();
            const std::vector<uint64_t>& deathmatchIds = World::GetSpawnPointsDeathMatch().ids();
            if (campaignIds.empty() && deathmatchIds.empty()) return;

            HierarchyNode group;
            group.label = "Spawn Points";
            group.expanded = g_expandHierachyOnLoad;
            group.children.reserve(2);

            if (!campaignIds.empty()) {
                HierarchyNode& campaign = group.children.emplace_back();
                campaign.label = "Campaign";
                campaign.expanded = g_expandHierachyOnLoad;
                campaign.children.reserve(campaignIds.size());
                for (uint64_t objectId : campaignIds) {
                    AddObjectNode(campaign, objectId);
                }
            }

            if (!deathmatchIds.empty()) {
                HierarchyNode& deathmatch = group.children.emplace_back();
                deathmatch.label = "Deathmatch";
                deathmatch.expanded = g_expandHierachyOnLoad;
                deathmatch.children.reserve(deathmatchIds.size());
                for (uint64_t objectId : deathmatchIds) {
                    AddObjectNode(deathmatch, objectId);
                }
            }

            GetSceneNode().children.push_back(std::move(group));
        }

        bool RemoveObjectNode(std::vector<HierarchyNode>& nodes, uint64_t objectId) {
            for (auto nodeIt = nodes.begin(); nodeIt != nodes.end(); nodeIt++) {
                HierarchyNode& node = *nodeIt;
                if (node.itemId == objectId && node.pointIndex < 0 && node.wallSegmentIndex < 0) {
                    nodes.erase(nodeIt);
                    return true;
                }

                if (!RemoveObjectNode(node.children, objectId)) continue;

                if (node.children.empty() && !node.selectable) {
                    nodes.erase(nodeIt);
                }

                return true;
            }

            return false;
        }

        void AddWorldPlaneGroup(const char* label, WorldPlaneType type) {
            HierarchyNode group;
            group.label = label;
            group.expanded = g_expandHierachyOnLoad;
            group.children.reserve(World::GetWorldPlanes().size());

            for (uint64_t objectId : World::GetWorldPlanes().ids()) {
                WorldPlane* worldPlane = World::GetWorldPlaneByObjectId(objectId);
                if (worldPlane && worldPlane->GetParentDoorId() == 0 && worldPlane->GetType() == type) {
                    AddObjectNode(group, objectId);
                }
            }

            if (!group.children.empty()) {
                GetSceneNode().children.push_back(std::move(group));
            }
        }

        void AddPlanarQuadObjectGroup(const char* label, PlanarQuadObjectType type) {
            HierarchyNode group;
            group.label = label;
            group.expanded = g_expandHierachyOnLoad;
            group.children.reserve(World::GetPlanarQuadObjects().size());

            for (uint64_t objectId : World::GetPlanarQuadObjects().ids()) {
                PlanarQuadObject* object = World::GetPlanarQuadObjectByObjectId(objectId);
                if (object && object->GetType() == type) {
                    AddObjectNode(group, objectId);
                }
            }

            if (!group.children.empty()) {
                GetSceneNode().children.push_back(std::move(group));
            }
        }

        void AddPointPairObjectGroup(const char* label, PointPairObjectType type) {
            HierarchyNode group;
            group.label = label;
            group.expanded = g_expandHierachyOnLoad;
            group.children.reserve(World::GetPointPairObjects().size());

            for (uint64_t objectId : World::GetPointPairObjects().ids()) {
                PointPairObject* object = World::GetPointPairObjectByObjectId(objectId);
                if (object && object->GetType() == type) {
                    AddObjectNode(group, objectId);
                }
            }

            if (!group.children.empty()) {
                GetSceneNode().children.push_back(std::move(group));
            }
        }

        void AddWallSegments(HierarchyNode& wallNode, Wall& wall) {
            wallNode.children.reserve(wall.GetWallSegments().size());
            for (size_t i = 0; i < wall.GetWallSegments().size(); i++) {
                HierarchyNode& segmentNode = wallNode.children.emplace_back();
                segmentNode.label = "Wall Segment " + std::to_string(i + 1);
                segmentNode.itemId = wall.GetObjectId();
                segmentNode.selectable = true;
                segmentNode.wallSegmentIndex = static_cast<int32_t>(i);
            }
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
                AddWallSegments(wallNode, *wall);
            }

            if (!group.children.empty()) {
                GetSceneNode().children.push_back(std::move(group));
            }
        }

        void AddChristmasLightPoints(HierarchyNode& christmasLightsNode, const ChristmasLightSet& christmasLights) {
            const std::vector<SequencePoint>& sequencePoints = christmasLights.GetCreateInfo().sequencePoints;
            christmasLightsNode.children.reserve(sequencePoints.size());

            for (size_t i = 0; i < sequencePoints.size(); i++) {
                HierarchyNode& pointNode = christmasLightsNode.children.emplace_back();
                pointNode.label = "Point " + std::to_string(i + 1);
                pointNode.itemId = christmasLights.GetObjectId();
                pointNode.selectable = true;
                pointNode.pointIndex = static_cast<int32_t>(i);
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

            if (!group.children.empty()) {
                GetSceneNode().children.push_back(std::move(group));
            }
        }

        int32_t GetVisibleRowCapacity() {
            return std::max(0, Layout::GetHierarchyContentRect().height / GetStyle().hierarchy.rowHeight);
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
            for (HierarchyNode& node : g_root.children) {
                GatherVisibleRows(node, 0);
            }

            const int32_t maximumFirstRow = std::max(0, static_cast<int32_t>(g_visibleRows.size()) - GetVisibleRowCapacity());
            g_scrollBar.value = std::clamp(g_scrollBar.value, 0, maximumFirstRow);
        }

        EditorRect GetScrollBarRect() {
            const EditorHierarchyStyle& style = GetStyle().hierarchy;
            const EditorRect contentRect = Layout::GetHierarchyContentRect();
            return { contentRect.Right() - style.scrollBarRightPadding - style.scrollBarWidth, contentRect.y, style.scrollBarWidth, contentRect.height };
        }

        EditorRect GetRowRect(size_t rowIndex) {
            const EditorHierarchyStyle& style = GetStyle().hierarchy;
            const EditorRect contentRect = Layout::GetHierarchyContentRect();
            const int32_t visibleIndex = static_cast<int32_t>(rowIndex) - g_scrollBar.value;
            const int32_t scrollBarWidth = g_scrollBar.visible ? style.scrollBarWidth + style.scrollBarRightPadding : 0;
            return { contentRect.x + 1, contentRect.y + visibleIndex * style.rowHeight, std::max(0, contentRect.width - scrollBarWidth - 2), style.rowHeight };
        }

        EditorRect GetArrowRect(const EditorRect& rowRect, uint32_t depth) {
            const EditorHierarchyStyle& style = GetStyle().hierarchy;
            const int32_t x = rowRect.x + style.leftPadding + static_cast<int32_t>(depth) * style.indentWidth;
            return { x, rowRect.y + (style.rowHeight - style.arrowSize) / 2, style.arrowSize, style.arrowSize };
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
            const bool clickedArrow = node.collapsible && !node.children.empty() && GetArrowRect(GetRowRect(hoveredRow), row.depth).Contains(mousePosition);

            // Groups toggle from the whole row; selectable nodes keep the row for selection
            if (clickedArrow || (!node.selectable && node.collapsible && !node.children.empty())) {
                node.expanded = !node.expanded;
                RefreshVisibleRows();
                RefreshHover(mousePosition);
                return;
            }

            if (node.pointIndex >= 0) {
                Selection::SelectPoint(node.itemId, node.pointIndex);
            }
            else if (node.wallSegmentIndex >= 0) {
                Selection::SelectWallSegment(node.itemId, node.wallSegmentIndex);
            }
            else if (node.workspace) {
                Selection::SelectWorkspace();
            }
            else if (node.selectable) {
                Selection::SelectObject(node.itemId);
            }
        }
    }

    void Init() {
        ResetRoot();
        g_scrollBar = {};
        g_hoveredNode = nullptr;
        g_wantsMouseCapture = false;
        RefreshVisibleRows();
    }

    void Refresh() {
        ResetRoot();
        g_scrollBar.value = 0;
        g_hoveredNode = nullptr;

        if (!Unloved::EditorSession::HasMode()) {
            RefreshVisibleRows();
            return;
        }

        AddWorldPlaneGroup("Ceilings", WorldPlaneType::CEILING);
        AddWorldGroup("Christmas Trees", World::GetChristmasTrees().ids());
        AddChristmasLightGroup();
        AddPointPairObjectGroup("Decking Bearer", PointPairObjectType::DECKING_BEARER);
        AddPlanarQuadObjectGroup("Decking Boards", PlanarQuadObjectType::DECKING_BOARDS);
        AddPointPairObjectGroup("Decking Posts", PointPairObjectType::DECKING_POST);
        AddWorldGroup("DDGI Volumes", World::GetDDGIVolumes().ids());
        AddWorldGroup("Dobermann", World::GetDobermanns().ids());
        AddPointPairObjectGroup("Down Pipes", PointPairObjectType::DOWN_PIPE);
        AddWorldGroup("Doors", World::GetDoors().ids());
        AddWorldGroup("Fences", World::GetFences().ids());
        AddWorldGroup("Fireplaces", World::GetFireplaces().ids());
        AddWorldPlaneGroup("Floors", WorldPlaneType::FLOOR);
        AddWorldGroup("Generic Animated Objects", World::GetGenericAnimatedObjects().ids());
        AddWorldGroup("Generic Objects", World::GetGenericObjects().ids());
        AddPointPairObjectGroup("Gutters", PointPairObjectType::GUTTER);
        AddWorldGroup("House Locations", World::GetHouseLocations().ids());
        AddWorldGroup("Jetties", World::GetJetties().ids());
        AddWorldGroup("Kangaroos", World::GetKangaroos().ids());
        AddWorldGroup("Ladders", World::GetLadders().ids());
        AddWorldGroup("Lights", World::GetLightIds());
        AddWorldGroup("Mermaids", World::GetMermaids().ids());
        AddWorldGroup("Pick Ups", World::GetPickUps().ids());
        AddWorldGroup("Picture Frames", World::GetPictureFrames().ids());
        AddWorldGroup("Pianos", World::GetPianos().ids());
        AddWorldGroup("Power Pole Sets", World::GetPowerPoleSets().ids());
        AddPointPairObjectGroup("Ridge Capping", PointPairObjectType::RIDGE_CAPPING);
        AddPlanarQuadObjectGroup("Roofing Iron", PlanarQuadObjectType::ROOFING_IRON);
        AddSpawnPointGroup();
        AddWorldGroup("Staircases", World::GetStaircases().ids());
        AddWorldGroup("Sharks", World::GetSharks().ids());
        AddWallGroup();
        AddWorldGroup("Windows", World::GetWindows().ids());

        RefreshVisibleRows();
    }

    void RefreshObjectChildren(uint64_t objectId) {
        for (HierarchyNode& group : GetSceneNode().children) {
            for (HierarchyNode& node : group.children) {
                if (node.itemId != objectId || node.pointIndex >= 0) continue;

                node.children.clear();
                switch (GetObjectIdType(objectId)) {
                    case ObjectType::WALL: {
                        if (Wall* wall = World::GetWallByObjectId(objectId)) {
                            AddWallSegments(node, *wall);
                        }
                        break;
                    }
                    case ObjectType::CHRISTMAS_LIGHTS: {
                        if (ChristmasLightSet* christmasLights = World::GetChristmasLightsByObjectId(objectId)) {
                            AddChristmasLightPoints(node, *christmasLights);
                        }
                        break;
                    }
                    default: {
                        break;
                    }
                }

                RefreshVisibleRows();
                return;
            }
        }
    }

    void RemoveObject(uint64_t objectId) {
        std::vector<HierarchyNode>& groups = GetSceneNode().children;
        if (!RemoveObjectNode(groups, objectId)) return;
        g_hoveredNode = nullptr;
        RefreshVisibleRows();
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

        if (Hell::Input::LeftMousePressed()) {
            HandleMousePress(mousePosition);
        }
    }

    void Render() {
        const EditorStyle& style = GetStyle();
        RefreshVisibleRows();

        const int32_t lastVisibleRow = std::min(static_cast<int32_t>(g_visibleRows.size()), g_scrollBar.value + GetVisibleRowCapacity());
        for (int32_t i = g_scrollBar.value; i < lastVisibleRow; i++) {
            const VisibleRow& row = g_visibleRows[i];
            const HierarchyNode& node = *row.node;
            const EditorRect rowRect = GetRowRect(i);
            std::string label = node.label;

            // Sub-items use their own labels
            if (node.selectable && node.itemId != 0 && node.pointIndex < 0 && node.wallSegmentIndex < 0) {
                const std::string& editorName = World::GetEditorNameById(node.itemId);
                label = editorName.empty() || editorName == UNDEFINED_STRING ? std::to_string(node.itemId) : editorName;
            }

            // Row background
            const bool pointSelected = node.pointIndex >= 0 && Selection::HasSelectedPoint() && node.itemId == Selection::GetSelectedObjectId() && node.pointIndex == Selection::GetSelectedPointIndex();
            const bool wallSegmentSelected = node.wallSegmentIndex >= 0 && Selection::HasSelectedWallSegment() && node.itemId == Selection::GetSelectedObjectId() && node.wallSegmentIndex == Selection::GetSelectedWallSegmentIndex();
            const bool workspaceSelected = node.workspace && Selection::HasWorkspaceSelection();
            const bool objectSelected = node.itemId != 0 && node.pointIndex < 0 && node.wallSegmentIndex < 0 && !Selection::HasSelectedPoint() && !Selection::HasSelectedWallSegment() && node.itemId == Selection::GetSelectedObjectId();
            if (node.selectable && (workspaceSelected || pointSelected || wallSegmentSelected || objectSelected)) {
                UI::DrawSolidRect(rowRect, style.colors.selected);
            }
            else if (row.node == g_hoveredNode) {
                UI::DrawSolidRect(rowRect, style.colors.hover);
            }

            // Expand arrow
            const EditorRect arrowRect = GetArrowRect(rowRect, row.depth);
            if (node.collapsible && !node.children.empty()) {
                const float rotation = node.expanded ? 0.0f : HELL_PI * -0.5f;
                UIBackEnd::BlitTexture(UICanvas::NATIVE, "DropDownArrow", glm::ivec2(arrowRect.x + arrowRect.width / 2, arrowRect.y + arrowRect.height / 2), Alignment::CENTERED, style.colors.text, glm::ivec2(style.hierarchy.arrowSize), TextureFilter::NEAREST, rotation);
            }

            // Row label
            const int32_t textX = arrowRect.Right() + style.hierarchy.textGap;
            UIBackEnd::BlitText(UICanvas::NATIVE, std::string(style.font.textColorTag) + label, style.font.name, glm::ivec2(textX, rowRect.y + rowRect.height / 2), Alignment::CENTERED_VERTICAL, style.font.scale, TextureFilter::NEAREST);
        }

        ScrollBar::Render(g_scrollBar);
    }

    bool WantsMouseCapture() {
        return g_wantsMouseCapture;
    }
}
