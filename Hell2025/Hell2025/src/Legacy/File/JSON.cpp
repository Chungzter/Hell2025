#pragma once
#include "JSON.h"

#include "Hell/Common/Constants.h"
#include "Hell/Common/Enum.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include <fstream>

#include <iostream> // TODO: cleanup logging

namespace nlohmann {
    void to_json(nlohmann::json& j, const glm::vec3& v) {
        j = json::array({ v.x, v.y, v.z });
    }

    void to_json(nlohmann::json& j, const glm::vec2& v) {
        j = json::array({ v.x, v.y });
    }

    void to_json(nlohmann::json& j, const Unloved::SequencePoint& sequencePoint) {
        j = nlohmann::json{
            {"Position", sequencePoint.position},
            {"Normal", sequencePoint.normal},
            {"Value", sequencePoint.value},
        };
    }

    void to_json(nlohmann::json& j, const ChristmasLightsCreateInfo& info) {
        j = nlohmann::json{
            {"SequencePoints", info.sequencePoints},
            {"Spiral", info.spiral},
            {"SpiralRadius", info.spiralRadius},
            {"SpiarlHeight", info.spiarlHeight},
            {"SprialTopCenter", info.sprialTopCenter},

            {"EditorName", info.editorName},
        };
    }

    void to_json(nlohmann::json& j, const DDGIVolumeCreateInfo& createInfo) {
        j = nlohmann::json{
            {"Origin", createInfo.origin},
            {"Rotation", createInfo.rotation},
            {"Extents", createInfo.extents},
            {"ProbeSpacing", createInfo.probeSpacing},
            {"EditorName", createInfo.editorName},
            {"SaveToFile", createInfo.saveToFile},
        };
    }

    void to_json(nlohmann::json& j, const DobermannCreateInfo& createInfo) {
        j = nlohmann::json{
            {"Position", createInfo.position},
            {"Rotation", createInfo.rotation},
            {"EditorName", createInfo.editorName}
        };
    }

    void to_json(nlohmann::json& j, const DoorCreateInfo& createInfo) {
        j = nlohmann::json{
            {"Position", createInfo.position},
            {"Rotation", createInfo.rotation},
            {"EditorName", createInfo.editorName},
            {"Type", Hell::Enum::ToString(createInfo.type) },
            {"MaterialTypeFront", Hell::Enum::ToString(createInfo.materialTypeFront) },
            {"MaterialTypeBack", Hell::Enum::ToString(createInfo.materialTypeBack) },
            {"MaterialTypeFrameFront", Hell::Enum::ToString(createInfo.materialTypeFrameFront) },
            {"MaterialTypeFrameBack", Hell::Enum::ToString(createInfo.materialTypeFrameBack) },
            {"MaxOpenValue", createInfo.maxOpenValue},
            {"HasDeadLock", createInfo.hasDeadLock},
            {"DeadLockedAtStart", createInfo.deadLockedAtInit}
        };
    }

    void to_json(nlohmann::json& j, const FenceCreateInfo& createInfo) {
        j = nlohmann::json{
            {"ControlPoints2D", createInfo.controlPoints2D },
            {"EditorName", createInfo.editorName}
        };
    }

    void to_json(nlohmann::json& j, const FireplaceCreateInfo& createInfo) {
        j = nlohmann::json{
            {"Position", createInfo.position},
            {"Rotation", createInfo.rotation},
            {"Type", Hell::Enum::ToString(createInfo.type)},
            {"EditorName", createInfo.editorName}
        };
    }

    void to_json(nlohmann::json& j, const GenericObjectCreateInfo& createInfo) {
        j = nlohmann::json{
            {"Position", createInfo.position},
            {"Rotation", createInfo.rotation},
            {"Scale", createInfo.scale},
            {"EditorName", createInfo.editorName},
            {"Type", Hell::Enum::ToString(createInfo.type)}
        };
    }

    void to_json(nlohmann::json& j, const HouseLocation& houseLocation) {
        j = nlohmann::json{
            {"Position", houseLocation.position},
            {"Rotation", houseLocation.rotation},
            {"Type", Hell::Enum::ToString(houseLocation.type)}
        };
    }

    void to_json(nlohmann::json& j, const WorldPlaneCreateInfo& createInfo) {
        j = nlohmann::json{
            {"P0", createInfo.p0},
            {"P1", createInfo.p1},
            {"P2", createInfo.p2},
            {"P3", createInfo.p3},
            {"TextureScale", createInfo.textureScale},
            {"TextureOffsetU", createInfo.textureOffsetU},
            {"TextureOffsetV", createInfo.textureOffsetV},
            {"TextureRotation", createInfo.textureRotation},
            {"Material", createInfo.materialName},
            {"Type", Hell::Enum::ToString(createInfo.type)},
            {"EditorName", createInfo.editorName}
        };
    }

    void to_json(nlohmann::json& j, const LadderCreateInfo& createInfo) {
        j = nlohmann::json{
            {"Position", createInfo.position},
            {"Rotation", createInfo.rotation},
            {"StepCount", createInfo.stepCount},
            {"EditorName", createInfo.editorName}
        };
    }

    void to_json(nlohmann::json& j, const LightCreateInfo& createInfo) {
        j = nlohmann::json{
            {"Color", createInfo.color},
            {"Position", createInfo.position},
            {"Rotation", createInfo.rotation},
            {"Forward", createInfo.forward},
            {"Radius", createInfo.radius},
            {"Twist", createInfo.twist},
            {"SaveToFile", createInfo.saveToFile},
            {"IESProfileType", Hell::Enum::ToString(createInfo.iesProfileType)},
            {"IESExposure", createInfo.iesExposure},
            {"Strength", createInfo.strength},
            {"Type", Hell::Enum::ToString(createInfo.type)},
        };
    }

    void to_json(nlohmann::json& j, const MermaidCreateInfo& createInfo) {
        j = nlohmann::json{
            {"Position", createInfo.position},
            {"Rotation", createInfo.rotation},
            {"EditorName", createInfo.editorName}
        };
    }

    void to_json(nlohmann::json& j, const PianoCreateInfo& createInfo) {
        j = nlohmann::json{
            {"Position", createInfo.position},
            {"Rotation", createInfo.rotation},
            {"SoundFontName", createInfo.soundFontName}
        };
    }

    void to_json(nlohmann::json& j, const PickUpCreateInfo& createInfo) {
        j = nlohmann::json{
            {"Position", createInfo.position},
            {"Rotation", createInfo.rotation},
            {"SaveToFile", createInfo.saveToFile},
            {"Respawn", createInfo.respawn},
            {"DisablePhysicsAtSpawn", createInfo.disablePhysicsAtSpawn},
            {"Name", createInfo.name},
            {"Type", Hell::Enum::ToString(createInfo.type)},
            {"EditorName", createInfo.editorName}
        };
    }

    void to_json(nlohmann::json& j, const PictureFrameCreateInfo& createInfo) {
        j = nlohmann::json{
            {"Position", createInfo.position},
            {"Rotation", createInfo.rotation},
            {"Scale", createInfo.scale},
            {"Type", Hell::Enum::ToString(createInfo.type)},
        };
    }

    void to_json(nlohmann::json& j, const PowerPoleSetCreateInfo& createInfo) {
        j = nlohmann::json{
            {"SequencePoints", createInfo.sequencePoints},
            {"EditorName", createInfo.editorName}
        };
    }

    void to_json(nlohmann::json& j, const SharkCreateInfo& createInfo) {
        j = nlohmann::json{
            {"Position", createInfo.position},
            {"EditorName", createInfo.editorName}
        };
    }

    void to_json(nlohmann::json& j, const SpawnPointCreateInfo& createInfo) {
        j = nlohmann::json{
            {"Position", createInfo.position},
            {"CamEuler", createInfo.camEuler},
        };
    }

    void to_json(nlohmann::json& j, const StaircaseCreateInfo& createInfo) {
        j = nlohmann::json{
            {"Position", createInfo.position},
            {"Rotation", createInfo.rotation},
            {"StepCount", createInfo.stepCount},
            {"EditorName", createInfo.editorName}
        };
    }

    void to_json(nlohmann::json& j, const TreeCreateInfo& createInfo) {
        j = nlohmann::json{
            {"Position", createInfo.position},
            {"Rotation", createInfo.rotation},
            {"Scale", createInfo.scale},
            {"Type", Hell::Enum::ToString(createInfo.type)},
            {"EditorName", createInfo.editorName}
        };
    }

    void to_json(nlohmann::json& j, const WallCreateInfo& createInfo) {
        j = nlohmann::json{
            {"EditorName", createInfo.editorName},
            {"Height", createInfo.height},
            {"Material", createInfo.materialName},
            {"MiddleTrimHeight", createInfo.middleTrimHeight},
            {"Points", createInfo.points},
            {"TextureScale", createInfo.textureScale},
            {"TextureOffsetU", createInfo.textureOffsetU},
            {"TextureOffsetV", createInfo.textureOffsetV},
            {"TextureRotation", createInfo.textureRotation},
            {"TrimTypeCeiling", Hell::Enum::ToString(createInfo.ceilingTrimType)},
            {"TrimTypeFloor",  Hell::Enum::ToString(createInfo.floorTrimType)},
            {"UseReversePointOrder", createInfo.useReversePointOrder},
            {"WallType",  Hell::Enum::ToString(createInfo.wallType)}
        };
    }

    void to_json(nlohmann::json& j, const WindowCreateInfo& createInfo) {
        j = nlohmann::json{
            {"Position", createInfo.position},
            {"Rotation", createInfo.rotation}
        };
    }

    void to_json(nlohmann::json& j, const std::map<Hell::ivecXZ, std::string>& mapData) {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& m : mapData) {
            nlohmann::json item;
            item["name"] = m.second;   // sector name
            item["x"] = m.first.x;     // x coordinate
            item["z"] = m.first.z;     // z coordinate
            arr.push_back(item);
        }
        j = arr;
    }

    void from_json(const nlohmann::json& j, ChristmasLightsCreateInfo& info) {
        info.editorName = j.value("EditorName", UNDEFINED_STRING);

        info.sequencePoints = j.value("SequencePoints", std::vector<Unloved::SequencePoint>{});
        if (info.sequencePoints.empty()) {
            const std::vector<glm::vec3> points = j.value("Points", std::vector<glm::vec3>{});
            const std::vector<float> values = j.value("SagHeights", std::vector<float>{});

            info.sequencePoints.reserve(points.size());
            for (size_t i = 0; i < points.size(); i++) {
                Unloved::SequencePoint sequencePoint;
                sequencePoint.position = points[i];
                if (i < values.size()) {
                    sequencePoint.value = values[i];
                }
                info.sequencePoints.push_back(sequencePoint);
            }
        }

        info.spiral = j.value("Spiral", false);
        info.spiralRadius = j.value("SpiralRadius", 1.0f);
        info.spiarlHeight = j.value("SpiarlHeight", 1.0f);
        info.sprialTopCenter = j.value("SprialTopCenter", glm::vec3(0.0f));
    }

    void from_json(const nlohmann::json& j, Unloved::SequencePoint& sequencePoint) {
        sequencePoint.position = j.value("Position", glm::vec3(0.0f));
        sequencePoint.normal = j.value("Normal", glm::vec3(0.0f, 1.0f, 0.0f));
        sequencePoint.value = j.value("Value", 0.0f);
    }

    void from_json(const nlohmann::json& j, DDGIVolumeCreateInfo& createInfo) {
        createInfo.origin = j.value("Origin", glm::vec3(0.0f));
        createInfo.rotation = j.value("Rotation", glm::vec3(0.0f));
        createInfo.extents = j.value("Extents", glm::vec3(0.0f));
        createInfo.probeSpacing = j.value("ProbeSpacing", 0.75f);
        createInfo.editorName = j.value("EditorName", UNDEFINED_STRING);
        createInfo.saveToFile = j.value("SaveToFile", true);
    }

    void from_json(const nlohmann::json& j, DobermannCreateInfo& info) {
        info.position = j.value("Position", glm::vec3(0.0f));
        info.rotation = j.value("Rotation", j.value("EulerDirection", glm::vec3(0.0f)));
        info.editorName = j.value("EditorName", UNDEFINED_STRING);
    }

    void from_json(const nlohmann::json& j, DoorCreateInfo& info) {
        info.position = j.value("Position", glm::vec3(0.0f));
        info.rotation = j.value("Rotation", glm::vec3(0.0f));
        info.editorName = j.value("EditorName", UNDEFINED_STRING);
        info.type = Hell::Enum::FromString(j.value("Type", UNDEFINED_STRING), DoorType::UNDEFINED);
        info.materialTypeFront = Hell::Enum::FromString(j.value("MaterialTypeFront", UNDEFINED_STRING), DoorMaterialType::UNDEFINED);
        info.materialTypeBack = Hell::Enum::FromString(j.value("MaterialTypeBack", UNDEFINED_STRING), DoorMaterialType::UNDEFINED);
        info.materialTypeFrameFront = Hell::Enum::FromString(j.value("MaterialTypeFrameFront", UNDEFINED_STRING), DoorMaterialType::UNDEFINED);
        info.materialTypeFrameBack = Hell::Enum::FromString(j.value("MaterialTypeFrameBack", UNDEFINED_STRING), DoorMaterialType::UNDEFINED);
        info.hasDeadLock = j.value("HasDeadLock", false);
        info.deadLockedAtInit = j.value("DeadLockedAtStart", false);
        info.maxOpenValue = j.value("MaxOpenValue", 2.1f);
    }

    void from_json(const nlohmann::json& j, FenceCreateInfo& info) {
        info.controlPoints2D = j.value("ControlPoints2D", std::vector<glm::vec2>());
        info.editorName = j.value("EditorName", UNDEFINED_STRING);
    }

    void from_json(const nlohmann::json& j, FireplaceCreateInfo& info) {
        info.position = j.value("Position", glm::vec3(0.0f));
        info.rotation = j.value("Rotation", glm::vec3(0.0f));
        info.editorName = j.value("EditorName", UNDEFINED_STRING);
        info.type = Hell::Enum::FromString(j.value("Type", UNDEFINED_STRING), FireplaceType::UNDEFINED);
    }

    void from_json(const nlohmann::json& j, GenericObjectCreateInfo& info) {
        info.position = j.value("Position", glm::vec3(0.0f));
        info.rotation = j.value("Rotation", glm::vec3(0.0f));
        info.scale = j.value("Scale", glm::vec3(1.0f));
        info.editorName = j.value("EditorName", UNDEFINED_STRING);
        info.type = Hell::Enum::FromString(j.value("Type", UNDEFINED_STRING), GenericObjectType::UNDEFINED);
    }

    void from_json(const nlohmann::json& j, HouseLocation& houseLocation) {
        houseLocation.position = j.value("Position", glm::vec3(0.0f));
        houseLocation.rotation = j.value("Rotation", 0.0f);
        houseLocation.type = Hell::Enum::FromString(j.value("Type", UNDEFINED_STRING), HouseType::UNDEFINED);
    }

    void from_json(const nlohmann::json& j, WorldPlaneCreateInfo& info) {
        info.p0 = j.value("P0", glm::vec3(0.0f));
        info.p1 = j.value("P1", glm::vec3(0.0f));
        info.p2 = j.value("P2", glm::vec3(0.0f));
        info.p3 = j.value("P3", glm::vec3(0.0f));
        info.textureScale = j.value("TextureScale", 1.0f);
        info.textureOffsetU = j.value("TextureOffsetU", 0.0f);
        info.textureOffsetV = j.value("TextureOffsetV", 0.0f);
        info.textureRotation = j.value("TextureRotation", 0.0f);
        info.materialName = j.value("Material", "CheckerBoard");
        info.type = Hell::Enum::FromString(j.value("Type", UNDEFINED_STRING), WorldPlaneType::UNDEFINED);
        info.editorName = j.value("EditorName", UNDEFINED_STRING);
    }

    void from_json(const nlohmann::json& j, LadderCreateInfo& info) {
        info.position = j.value("Position", glm::vec3(0.0f));
        info.rotation = j.value("Rotation", glm::vec3(0.0f));
        info.stepCount = j.value("StepCount", 1);
        info.editorName = j.value("EditorName", UNDEFINED_STRING);
    }

    void from_json(const nlohmann::json& j, LightCreateInfo& info) {
        info.color = j.value("Color", glm::vec3(1.0f));
        info.position = j.value("Position", glm::vec3(0.0f));
        info.rotation = j.value("Rotation", glm::vec3(0.0f));
        info.forward = j.value("Forward", glm::vec3(0.0f, -1.0f, 0.0f));
        info.radius = j.value("Radius", 1.0f);
        info.saveToFile = j.value("SaveToFile", true);
        info.strength = j.value("Strength", 1.0f);
        info.twist = j.value("Twist", 0.0f);
        info.type = Hell::Enum::FromString(j.value("Type", "HANGING_LIGHT"), LightType::UNDEFINED);
        info.iesProfileType = Hell::Enum::FromString(j.value("IESProfileType", "NONE"), IESProfileType::NONE);
        info.iesExposure = j.value("IESExposure", 1.0f);
    }

    void from_json(const nlohmann::json& j, MermaidCreateInfo& info) {
        info.position = j.value("Position", glm::vec3(0.0f));
        info.rotation = j.value("Rotation", glm::vec3(0.0f));
        info.editorName = j.value("EditorName", UNDEFINED_STRING);
    }

    void from_json(const nlohmann::json& j, PianoCreateInfo& info) {
        info.position = j.value("Position", glm::vec3(0.0f));
        info.rotation = j.value("Rotation", glm::vec3(0.0f));
        info.soundFontName = j.value("SoundFontName", std::string("YamahaGrandLiteV2"));
    }

    void from_json(const nlohmann::json& j, PickUpCreateInfo& info) {
        info.position = j.value("Position", glm::vec3(0.0f));
        info.rotation = j.value("Rotation", glm::vec3(0.0f));
        info.type = Hell::Enum::FromString(j.value("Type", "UNDEFINED_STRING"), ItemType::UNDEFINED);
        info.respawn = j.value("Respawn", true);
        info.saveToFile = j.value("SaveToFile", true);
        info.disablePhysicsAtSpawn = j.value("DisablePhysicsAtSpawn", true);
        info.name = j.value("Name", UNDEFINED_STRING);
        info.editorName = j.value("EditorName", UNDEFINED_STRING);
    }

    void from_json(const nlohmann::json& j, PictureFrameCreateInfo& info) {
        info.position = j.value("Position", glm::vec3(0.0f));
        info.rotation = j.value("Rotation", glm::vec3(0.0f));
        info.scale = j.value("Scale", glm::vec3(0.0f));
        info.type = Hell::Enum::FromString(j.value("Type", UNDEFINED_STRING), PictureFrameType::UNDEFINED);
    }

    void from_json(const nlohmann::json& j, PowerPoleSetCreateInfo& info) {
        info.sequencePoints = j.value("SequencePoints", std::vector<Unloved::SequencePoint>{});
        if (info.sequencePoints.empty()) {
            const std::vector<glm::vec2> controlPoints2D = j.value("ControlPoints2D", std::vector<glm::vec2>());

            info.sequencePoints.reserve(controlPoints2D.size());
            for (const glm::vec2& point : controlPoints2D) {
                Unloved::SequencePoint sequencePoint;
                sequencePoint.position = glm::vec3(point.x, 0.0f, point.y);
                info.sequencePoints.push_back(sequencePoint);
            }
        }
        info.editorName = j.value("EditorName", UNDEFINED_STRING);
    }

    void from_json(const nlohmann::json& j, SharkCreateInfo& info) {
        info.position = j.value("Position", glm::vec3(0.0f));
        info.editorName = j.value("EditorName", UNDEFINED_STRING);
    }

    void from_json(const nlohmann::json& j, StaircaseCreateInfo& info) {
        info.position = j.value("Position", glm::vec3(0.0f));
        info.rotation = j.value("Rotation", glm::vec3(0.0f));
        info.stepCount = j.value("StepCount", 1);
        info.editorName = j.value("EditorName", UNDEFINED_STRING);
    }

    void from_json(const nlohmann::json& j, TreeCreateInfo& info) {
        info.position = j.value("Position", glm::vec3(0.0f));
        info.rotation = j.value("Rotation", glm::vec3(0.0f));
        info.rotation = j.value("Scale", glm::vec3(1.0f));
        info.type = Hell::Enum::FromString(j.value("Type", UNDEFINED_STRING), TreeType::UNDEFINED);
        info.editorName = j.value("EditorName", UNDEFINED_STRING);
    }

    void from_json(const nlohmann::json& j, WallCreateInfo& info) {
        info.height = j.value("Height", 2.4f);
        info.middleTrimHeight = j.value("MiddleTrimHeight", 2.4f);
        info.materialName = j.value("Material", "CheckerBoard");
        info.points = j.value("Points", std::vector<glm::vec3>{});
        info.textureScale = j.value("TextureScale", 1.0f);
        info.textureOffsetU = j.value("TextureOffsetU", 0.0f);
        info.textureOffsetV = j.value("TextureOffsetV", 0.0f);
        info.textureRotation = j.value("TextureRotation", 0.0f);
        info.ceilingTrimType = Hell::Enum::FromString(j.value("TrimTypeCeiling", "NONE"), TrimType::UNDEFINED);
        info.floorTrimType = Hell::Enum::FromString(j.value("TrimTypeFloor", "NONE"), TrimType::UNDEFINED);
        info.wallType = Hell::Enum::FromString(j.value("WallType", "NONE"), WallType::UNDEFINED);
        info.useReversePointOrder = j.value("UseReversePointOrder", false);
        info.editorName = j.value("EditorName", UNDEFINED_STRING);
    }

    void from_json(const nlohmann::json& j, WindowCreateInfo& info) {
        info.position = j.value("Position", glm::vec3(0.0f));
        info.rotation = j.value("Rotation", glm::vec3(0.0f));
    }

    void from_json(const nlohmann::json& j, SpawnPointCreateInfo& spawnPoint) {
        spawnPoint.position = j.value("Position", glm::vec3(0.0f));
        spawnPoint.camEuler = j.value("CamEuler", glm::vec3(0.0f));
    }

    void from_json(const nlohmann::json& j, glm::vec2& v) {
        try {
            std::array<float, 2> arr = j.get<std::array<float, 2>>();
            v = glm::vec2(arr[0], arr[1]);
        }
        catch (const nlohmann::json::exception& e) {
            v = glm::vec2(0.0f, 0.0f);
        }
    }

    void from_json(const nlohmann::json& j, glm::vec3& v) {
        try {
            std::array<float, 3> arr = j.get<std::array<float, 3>>();
            v = glm::vec3(arr[0], arr[1], arr[2]);
        }
        catch (const nlohmann::json::exception& e) {
            v = glm::vec3(0.0f, 0.0f, 0.0f);
        }
    }

    void from_json(const nlohmann::json& j, std::map<Hell::ivecXZ, std::string>& mapData) {
        mapData.clear();
        for (const auto& item : j) {
            int x = item.at("x").get<int>();
            int z = item.at("z").get<int>();
            std::string sectorName = item.at("name").get<std::string>();
            Hell::ivecXZ key(x, z);
            mapData[key] = sectorName;
        }
    }

    void from_json(const json& j, glm::mat4& m) {
        std::array<float, 16> a = j.get<std::array<float, 16>>();
        m = glm::make_mat4(a.data());
    }

    void from_json(const json& j, glm::quat& q) {
        if (j.is_array() && j.size() == 4) {
            auto a = j.get<std::array<float, 4>>();
            q = glm::quat{ a[3], a[0], a[1], a[2] };
        }
        else {
            q = glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f };
        }
    }
}

namespace JSON {

    bool LoadJsonFromFile(nlohmann::json& json, const std::string filepath) {
        // Open the file
        std::ifstream file(filepath);
        if (!file) {
            std::cout << "JSON::LoadJsonFromFile() failed to open file: " << filepath << "\n";
            return false;
        }

        // Read the entire file into a string stream
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        // Try to parse the JSON
        try {
            json = nlohmann::json::parse(buffer.str());
            return true;
        }
        catch (const nlohmann::json::parse_error& e) {
            std::cerr << "JSON::LoadJsonFromFile() failed to parse the file " << filepath << ": " << e.what() << "\n";
            return false;
        }

    }

    void SaveToFile(nlohmann::json& json, const std::string& filepath) {
        std::ofstream file(filepath);
        if (file.is_open()) {
            file << json.dump(4);
            file.close();
            std::cout << "Saved " << filepath << "\n";
        }
    }

    CreateInfoCollection CreateInfoCollectionFromJSONString(const std::string& jsonString) {
        if (jsonString.empty()) {
            return CreateInfoCollection();
        }

        nlohmann::json json = nlohmann::json::parse(jsonString);
        return CreateInfoCollectionFromJSONObject(json);
    }

    CreateInfoCollection CreateInfoCollectionFromJSONObject(nlohmann::json& json) {
        CreateInfoCollection createInfoCollection;
        createInfoCollection.christmasLights = json.value("ChristmasLights", std::vector<ChristmasLightsCreateInfo>{});
        createInfoCollection.ddgiVolumes = json.value("DDGIVolumes", std::vector<DDGIVolumeCreateInfo>{});
        createInfoCollection.dobermanns = json.value("Dobermanns", std::vector<DobermannCreateInfo>{});
        createInfoCollection.doors = json.value("Doors", std::vector<DoorCreateInfo>{});
        createInfoCollection.fences = json.value("Fences", std::vector<FenceCreateInfo>{});
        createInfoCollection.fireplaces = json.value("Fireplaces", std::vector<FireplaceCreateInfo>{});
        createInfoCollection.genericObjects = json.value("Drawers", std::vector<GenericObjectCreateInfo>{});
        createInfoCollection.worldPlanes = json.value("Planes", std::vector<WorldPlaneCreateInfo>{});
        createInfoCollection.ladders = json.value("Ladders", std::vector<LadderCreateInfo>{});
        createInfoCollection.lights = json.value("Lights", std::vector<LightCreateInfo>{});
        createInfoCollection.mermaids = json.value("Mermaids", std::vector<MermaidCreateInfo>{});
        createInfoCollection.pianos = json.value("Pianos", std::vector<PianoCreateInfo>{});
        createInfoCollection.pickUps = json.value("PickUps", std::vector<PickUpCreateInfo>{});
        createInfoCollection.pictureFrames = json.value("PictureFrames", std::vector<PictureFrameCreateInfo>{});
        createInfoCollection.powerPoleSets = json.value("PowerPoleSets", std::vector<PowerPoleSetCreateInfo>{});
        createInfoCollection.sharks = json.value("Sharks", std::vector<SharkCreateInfo>{});
        createInfoCollection.spawnPointsCampaign = json.value("CampaignSpawns", std::vector<SpawnPointCreateInfo>{});
        createInfoCollection.spawnPointsDeathMatch = json.value("DeathmatchSpawns", std::vector<SpawnPointCreateInfo>{});
        createInfoCollection.staircases = json.value("Staircases", std::vector<StaircaseCreateInfo>{});
        createInfoCollection.trees = json.value("Trees", std::vector<TreeCreateInfo>{});
        createInfoCollection.walls = json.value("Walls", std::vector<WallCreateInfo>{});
        createInfoCollection.windows = json.value("Windows", std::vector<WindowCreateInfo>{});

        return createInfoCollection;
    }

    std::string CreateInfoCollectionToJSON(CreateInfoCollection& createInfoCollection) {
        nlohmann::json json;
        json["ChristmasLights"] = createInfoCollection.christmasLights;
        json["DDGIVolumes"] = createInfoCollection.ddgiVolumes;
        json["Dobermanns"] = createInfoCollection.dobermanns;
        json["Doors"] = createInfoCollection.doors;
        json["Drawers"] = createInfoCollection.genericObjects;
        json["Fences"] = createInfoCollection.fences;
        json["Fireplaces"] = createInfoCollection.fireplaces;
        json["Ladders"] = createInfoCollection.ladders;
        json["Lights"] = createInfoCollection.lights;
        json["Mermaids"] = createInfoCollection.mermaids;
        json["Pianos"] = createInfoCollection.pianos;
        json["PickUps"] = createInfoCollection.pickUps;
        json["PictureFrames"] = createInfoCollection.pictureFrames;
        json["PowerPoleSets"] = createInfoCollection.powerPoleSets;
        json["Sharks"] = createInfoCollection.sharks;
        json["CampaignSpawns"] = createInfoCollection.spawnPointsCampaign;
        json["DeathmatchSpawns"] = createInfoCollection.spawnPointsDeathMatch;
        json["Planes"] = createInfoCollection.worldPlanes;
        json["Staircases"] = createInfoCollection.staircases;
        json["Trees"] = createInfoCollection.trees;
        json["Walls"] = createInfoCollection.walls;
        json["Windows"] = createInfoCollection.windows;

        return json.dump(2);
    }

    AdditionalMapData AdditionalMapDataFromJSON(const std::string& jsonString) {
        if (jsonString.empty()) {
            return AdditionalMapData();
        }

        nlohmann::json json = nlohmann::json::parse(jsonString);

        AdditionalMapData additionalMapData;
        additionalMapData.houseLocations = json.value("HouseLocations", std::vector<HouseLocation>{});

        return additionalMapData;
    }

    std::string AdditionalMapDataToJSON(AdditionalMapData& additionalMapData) {
        nlohmann::json json;

        json["HouseLocations"] = additionalMapData.houseLocations;

        return json.dump(2);
    }
}
