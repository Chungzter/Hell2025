#pragma once
#include "Hell/Common.h"
#include "Hell/Physics/PhysicsTypes.h"
#include "Unloved/ObjectId/ObjectId_types.h"
#include <cstdint>

enum struct RendererMode {
    OLD_DEFERRED,
    RE_STYLE,
    RENDERER_MODE_COUNT
};

enum InputType {
    KEYBOARD_AND_MOUSE,
    CONTROLLER
};

enum struct IESProfileType {
    NONE = 0,
    LAMP_0,
    LAMP_1,
    LAMP_2,
    LAMP_3,
    LAMP_4,
    LAMP_5,
    LAMP_6,
    LAMP_7,
    LAMP_8,
    LAMP_9,
    LAMP_10,
    LAMP_11,
};

enum struct CollisionShapeType {
    BOX,
    CAPSULE,
    CONVEX_MESH,
    UNDEFINED
};

enum struct PhysicsShapeType {
    BOX,
    CONVEX_MESH
};

enum struct Shortcut {
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    CTRL_A, CTRL_B, CTRL_C, CTRL_D, CTRL_E, CTRL_F, CTRL_G, CTRL_H, CTRL_I, CTRL_J,
    CTRL_K, CTRL_L, CTRL_M, CTRL_N, CTRL_O, CTRL_P, CTRL_Q, CTRL_R, CTRL_S, CTRL_T,
    CTRL_U, CTRL_V, CTRL_W, CTRL_X, CTRL_Y, CTRL_Z,
    ESC, NONE
};

enum class BlendingMode {
    ALPHA_DISCARD,
    BLENDED,
    DEFAULT,
    HAIR_UNDER_LAYER,
    HAIR,
    TOILET_WATER,
    MIRROR,
    GLASS,
    PLASTIC,
    DO_NOT_RENDER,
    STAINED_GLASS,
    UNDEFINED
};

enum struct EditorSelectionMode {
    OBJECT,
    VERTEX
};

enum struct EditorState {
    IDLE,
    RESIZING_HORIZONTAL,
    RESIZING_VERTICAL,
    RESIZING_HORIZONTAL_VERTICAL,
    GIZMO_TRANSLATING,
    GIZMO_SCALING,
    GIZMO_ROTATING,
    DRAGGING_SELECT_RECT,

    PLACE_CHRISTMAS_LIGHTS,
    PLACE_DDGI_VOLUME,
    PLACE_FENCE,
    PLACE_OBJECT,
    PLACE_POWER_POLES,
    PLACE_WALL,

    // Object placement REMOVEEEEEEE MEEEEEEEE
    PLACE_DOOR,
    PLACE_DRAWERS,
    PLACE_FLOOR,
    PLACE_HOUSE,
    PLACE_PICTURE_FRAME,
    PLACE_TREE,
    PLACE_WINDOW,
    PLACE_PLAYER_CAMPAIGN_SPAWN,
    PLACE_PLAYER_DEATHMATCH_SPAWN
};

enum WeaponAction {
    IDLE = 0,
    FIRE,
    DRY_FIRE,
    RELOAD,
    RELOAD_FROM_EMPTY,
    DRAW_BEGIN,
    DRAWING,
    DRAWING_FIRST,
    DRAWING_WITH_SHOTGUN_PUMP,
    SPAWNING,
    SHOTGUN_UNLOAD_BEGIN,
    SHOTGUN_UNLOAD_SINGLE_SHELL,
    SHOTGUN_UNLOAD_DOUBLE_SHELL,
    SHOTGUN_UNLOAD_END,
    SHOTGUN_RELOAD_BEGIN,
    SHOTGUN_RELOAD_SINGLE_SHELL,
    SHOTGUN_RELOAD_DOUBLE_SHELL,
    SHOTGUN_RELOAD_END,
    SHOTGUN_RELOAD_END_WITH_PUMP,
    SHOTGUN_MELEE,
    ADS_IN,
    ADS_OUT,
    ADS_IDLE,
    ADS_FIRE,
    MELEE,
    TOGGLING_AUTO,
    UNDEFINED
};

enum class ShellEjectionState {
    IDLE, AWAITING_SHELL_EJECTION
};

enum DebugRenderMode {
    NONE = 0,
    LIGHTS,
    DECALS,
    RAGDOLLS,
    PATHFINDING_RECAST,
    PHYSX_ALL,
    PHYSX_RAYCAST,
    PHYSX_COLLISION,
    RAYTRACE_LAND,
    PHYSX_EDITOR,
    BOUNDING_BOXES,
    RTX_LAND_AABBS,
    RTX_LAND_TRIS,
    RTX_LAND_TOP_LEVEL_ACCELERATION_STRUCTURE,
    RTX_LAND_BOTTOM_LEVEL_ACCELERATION_STRUCTURES,
    RTX_LAND_TOP_AND_BOTTOM_LEVEL_ACCELERATION_STRUCTURES,
    CLIPPING_CUBES,
    HOUSE_GEOMETRY,
    BONES,
    BONE_TANGENTS,
    BVH_CPU_PLAYER_RAYS,
    DEBUG_LINE_MODE_COUNT,
};

enum struct LightType {
    LAMP_POST = 0,
    HANGING_LIGHT,
    FIREPLACE_FIRE,
    WALL_LAMP,
    UNDEFINED
};

enum struct EditorViewportSplitMode {
    SINGLE,
    FOUR_WAY_SPLIT,
    UNDEFINED
};

enum struct ItemType {
    HEAL,
    WEAPON,
    KEY,
    AMMO,
    USELESS,
    UNDEFINED
};

//enum struct PickUpTypeOld {
//    SHOTGUN_AMMO_BUCKSHOT,
//    SHOTGUN_AMMO_SLUG,
//    GLOCK,
//    GOLDEN_GLOCK,
//    AKS74U,
//    SPAS,
//    REMINGTON_870,
//    TOKAREV,
//    UNDEFINED
//};

enum struct EditorMode {
    HOUSE_EDITOR,
    MAP_HEIGHT_EDITOR,
    MAP_OBJECT_EDITOR,
    UNDEFINED,
};

enum struct OpeningState {
    CLOSED,
    CLOSING,
    OPEN,
    OPENING,
    UNDEFINED
};

enum struct DecalType {
    GLASS,
    PLASTER,
    UNDEFINED
};

enum struct TrimType {
    NONE,
    TIMBER,
    PLASTER,
    UNDEFINED
};

enum struct TrimSetType {
    FLOOR,
	MIDDLE,
	CEILING,
	CEILING_FANCY,
    UNDEFINED
};

enum struct HousePlaneType {
    FLOOR,
    CEILING,
    UNDEFINED
};

enum struct WallType {
    INTERIOR,
    WEATHER_BOARDS,
    UNDEFINED
};

enum struct TreeType {
    TREE_LARGE_0 = 0,
    TREE_LARGE_1,
    TREE_LARGE_2,
    BLACK_BERRIES,
    UNDEFINED
};

enum class RendererOverrideState {
    NONE = 0,
    BASE_COLOR,
    NORMALS,
    RMA,
    ROUGHNESS,
    METALIC,
    AO,
    CAMERA_NDOTL,
    TILE_HEATMAP_LIGHTS,
    TILE_HEATMAP_BLOOD_DECALS,
    TILE_HEATMAP_CHRISTMAS_LIGHTS,
    INDIRECT_DIFFUSE,
    VELOCITY,
    VIS_BUFFER,
    DEPTH,
    WORLD_POSITION,
    EMISSIVE,
    STATE_COUNT,
};

enum struct ProbeDebugState {
    HIDDEN,
    COLOR,
    DISTANCE,
    DISTANCE_COOL_DOWN,
    IRRADIENCE_COOL_DOWN,
    REVLANCE,
    ACTIVE,
    STATE_COUNT,
};

enum class PictureFrameType {
    BIG_LANDSCAPE,
    TALL_THIN,
    REGULAR_PORTRAIT,
    REGULAR_LANDSCAPE,
    UNDEFINED
};

enum class SharkMovementState {
    STOPPED,
    FOLLOWING_PATH,
    FOLLOWING_PATH_ANGRY,
    ARROW_KEYS,
    HUNT_PLAYER,
    UNDEFINED
};

enum class SharkHuntingState {
    CHARGE_PLAYER,
    BITING_PLAYER,
    UNDEFINED
};

enum class SharkMovementDirection {
    STRAIGHT,
    LEFT,
    RIGHT,
    UNDEFINED
};

enum class ChristmasPresentType : uint32_t {
    SMALL = 0,
    MEDIUM,
    LARGE,
    UNDEFINED
};


enum class GenericStaticType : uint32_t {
    COUCH = 0
};

enum class GenericBouncableType : uint32_t {
    COUCH_CUSHION_0 = 0,
    COUCH_CUSHION_1,
    COUCH_CUSHION_2,
    COUCH_CUSHION_3,
    COUCH_CUSHION_4
};

enum struct InventoryState {
    CLOSED,
    MAIN_SCREEN,
    EXAMINE_ITEM,
    MOVING_ITEM,
    ROTATING_ITEM,
    SHOP,
    UNDEFINED
};

enum class DebugTextMode{
    NONE,
    PER_PLAYER,
    GLOBAL,
    PROFILING,
    MEMORY_TRACKER,
    DEBUG_TEXT_MODE_COUNT
};

enum struct HouseType {
    SMALL_HOUSE,
    MEDIUM_HOUSE,
    LARGE_HOUSE,
    NAMED,
    UNDEFINED
};

enum struct FireplaceType {
	WOOD_STOVE,
	DEFAULT,
    UNDEFINED
};

enum struct GenericObjectType {
    CHRISTMAS_TREE,
    CHRISTMAS_PRESENT_SMALL,
    CHRISTMAS_PRESENT_LARGE,

    DRAWERS_SMALL,
    DRAWERS_LARGE,
    TOILET,
    COUCH,
    BATHROOM_BASIN,
    BATHROOM_CABINET,
    BATHROOM_TOWEL_RACK,

    CHAIR_RE,
    CHAIR_SPINDLE_BACK,

    MERMAID_ROCK,

    PLANT_BLACKBERRIES,
    PLANT_TREE,

    TEST_MODEL,
    TEST_MODEL2,
    TEST_MODEL3,
    TEST_MODEL4,
    UNDEFINED
};

enum struct DoorType {
    STANDARD_A,
    STANDARD_B,
    STAINED_GLASS,
    STAINED_GLASS2,
    UNDEFINED
};

enum struct DoorMaterialType {
    WHITE_PAINT,
    BACK_PAINT,
    RESIDENT_EVIL,
    UNDEFINED
};

enum struct ChainLinkType {
    DOOR_CHAIN,
    UNDEFINED
};

//enum struct MeshNodeType {
//    DEFAULT,
//    OPENABLE,
//    RIGID_STATIC,
//    RIGID_DYNAMIC
//};
