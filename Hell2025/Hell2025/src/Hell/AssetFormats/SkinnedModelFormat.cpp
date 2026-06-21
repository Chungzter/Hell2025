#include "AssetFormats.h"
#include "AssetFormatHeaders.h"

#include "Hell/Logging.h"

#include <cstring>
#include <fstream>

namespace Hell::AssetFormats {

    namespace {
        inline constexpr char SKINNED_MODEL_SIGNATURE[] = "HELL_SKINNED_MODEL";

        bool ReadSkinnedHeader(std::ifstream& file, SkinnedModelHeader& header) {
            file.read(header.signature, sizeof(header.signature));
            file.read(reinterpret_cast<char*>(&header.version), sizeof(header.version));
            file.read(reinterpret_cast<char*>(&header.nameLength), sizeof(header.nameLength));
            file.read(reinterpret_cast<char*>(&header.vertexCount), sizeof(header.vertexCount));
            file.read(reinterpret_cast<char*>(&header.indexCount), sizeof(header.indexCount));
            file.read(reinterpret_cast<char*>(&header.meshCount), sizeof(header.meshCount));
            file.read(reinterpret_cast<char*>(&header.nodeCount), sizeof(header.nodeCount));
            file.read(reinterpret_cast<char*>(&header.boneCount), sizeof(header.boneCount));
            file.read(reinterpret_cast<char*>(&header.timestamp), sizeof(header.timestamp));

            return file && header.version == 1 && std::memcmp(
                header.signature,
                SKINNED_MODEL_SIGNATURE,
                sizeof(header.signature)
            ) == 0;
        }

        void WriteSkinnedHeader(std::ofstream& file, const SkinnedModelHeader& header) {
            file.write(header.signature, sizeof(header.signature));
            file.write(reinterpret_cast<const char*>(&header.version), sizeof(header.version));
            file.write(reinterpret_cast<const char*>(&header.nameLength), sizeof(header.nameLength));
            file.write(reinterpret_cast<const char*>(&header.vertexCount), sizeof(header.vertexCount));
            file.write(reinterpret_cast<const char*>(&header.indexCount), sizeof(header.indexCount));
            file.write(reinterpret_cast<const char*>(&header.meshCount), sizeof(header.meshCount));
            file.write(reinterpret_cast<const char*>(&header.nodeCount), sizeof(header.nodeCount));
            file.write(reinterpret_cast<const char*>(&header.boneCount), sizeof(header.boneCount));
            file.write(reinterpret_cast<const char*>(&header.timestamp), sizeof(header.timestamp));
        }
    }

    bool ReadSkinnedModelMetadata(const std::string& path, SkinnedModelMetadata& outMetadata) {
        outMetadata = {};

        std::ifstream file(path, std::ios::binary);
        SkinnedModelHeader header{};
        if (!ReadSkinnedHeader(file, header)) {
            Logging::Error() << "AssetFormats::ReadSkinnedModelMetadata() found an invalid header in '" << path << "'\n";
            return false;
        }

        outMetadata.version = header.version;
        outMetadata.vertexCount = header.vertexCount;
        outMetadata.indexCount = header.indexCount;
        outMetadata.meshCount = header.meshCount;
        outMetadata.nodeCount = header.nodeCount;
        outMetadata.boneCount = header.boneCount;
        outMetadata.timestamp = header.timestamp;
        return true;
    }

    bool SaveSkinnedModel(const std::string& path, const SkinnedModelData& model) {
        std::ofstream file(path, std::ios::binary);
        if (!file) {
            Logging::Error() << "AssetFormats::SaveSkinnedModel() failed to open '" << path << "'\n";
            return false;
        }

        SkinnedModelHeader header{};
        std::memcpy(header.signature, SKINNED_MODEL_SIGNATURE, sizeof(header.signature));
        header.version = 1;
        header.nameLength = static_cast<uint32_t>(model.name.size());
        header.vertexCount = model.vertexCount;
        header.indexCount = model.indexCount;
        header.meshCount = model.GetMeshCount();
        header.nodeCount = model.GetNodeCount();
        header.boneCount = model.GetBoneCount();
        header.timestamp = model.timestamp;
        WriteSkinnedHeader(file, header);

        file.write(model.name.data(), model.name.size());

        for (const Node& node : model.nodes) {
            const uint32_t nameLength = static_cast<uint32_t>(node.name.size());
            file.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
            file.write(node.name.data(), nameLength);
            file.write(reinterpret_cast<const char*>(&node.parentIndex), sizeof(node.parentIndex));
            file.write(reinterpret_cast<const char*>(&node.inverseBindTransform), sizeof(node.inverseBindTransform));
        }

        for (const glm::mat4& boneOffset : model.boneOffsets) {
            file.write(reinterpret_cast<const char*>(&boneOffset), sizeof(boneOffset));
        }

        for (const auto& [boneName, boneIndex] : model.boneMapping) {
            const uint32_t nameLength = static_cast<uint32_t>(boneName.size());
            file.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
            file.write(boneName.data(), nameLength);
            file.write(reinterpret_cast<const char*>(&boneIndex), sizeof(boneIndex));
        }

        for (const SkinnedMeshData& mesh : model.meshes) {
            const uint32_t nameLength = static_cast<uint32_t>(mesh.name.size());
            const uint32_t vertexCount = static_cast<uint32_t>(mesh.vertices.size());
            const uint32_t indexCount = static_cast<uint32_t>(mesh.indices.size());

            file.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
            file.write(reinterpret_cast<const char*>(&vertexCount), sizeof(vertexCount));
            file.write(reinterpret_cast<const char*>(&indexCount), sizeof(indexCount));
            file.write(reinterpret_cast<const char*>(&mesh.localBaseVertex), sizeof(mesh.localBaseVertex));
            file.write(mesh.name.data(), nameLength);
            file.write(reinterpret_cast<const char*>(&mesh.aabbMin), sizeof(mesh.aabbMin));
            file.write(reinterpret_cast<const char*>(&mesh.aabbMax), sizeof(mesh.aabbMax));
            file.write(reinterpret_cast<const char*>(&mesh.requiresSkinning), sizeof(mesh.requiresSkinning));
            file.write(reinterpret_cast<const char*>(&mesh.nonDeformingBoneIndex), sizeof(mesh.nonDeformingBoneIndex));
            file.write(reinterpret_cast<const char*>(mesh.vertices.data()), mesh.vertices.size() * sizeof(Vertex));
            file.write(reinterpret_cast<const char*>(mesh.vertexWeights.data()), mesh.vertexWeights.size() * sizeof(VertexWeight));
            file.write(reinterpret_cast<const char*>(mesh.indices.data()), mesh.indices.size() * sizeof(uint32_t));
        }

        if (!file) {
            Logging::Error() << "AssetFormats::SaveSkinnedModel() failed while writing '" << path << "'\n";
            return false;
        }

        Logging::Debug() << "Saved skinned model '" << path << "'\n";
        return true;
    }

    bool LoadSkinnedModel(const std::string& path, SkinnedModelData& outModel) {
        outModel = {};

        std::ifstream file(path, std::ios::binary);
        SkinnedModelHeader header{};
        if (!ReadSkinnedHeader(file, header)) {
            Logging::Error() << "AssetFormats::LoadSkinnedModel() found an invalid header in '" << path << "'\n";
            return false;
        }

        outModel.name.resize(header.nameLength);
        file.read(outModel.name.data(), outModel.name.size());
        outModel.vertexCount = header.vertexCount;
        outModel.indexCount = header.indexCount;
        outModel.timestamp = header.timestamp;
        outModel.nodes.resize(header.nodeCount);
        outModel.boneOffsets.resize(header.boneCount);
        outModel.meshes.resize(header.meshCount);

        for (Node& node : outModel.nodes) {
            uint32_t nameLength = 0;
            file.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
            node.name.resize(nameLength);
            file.read(node.name.data(), node.name.size());
            file.read(reinterpret_cast<char*>(&node.parentIndex), sizeof(node.parentIndex));
            file.read(reinterpret_cast<char*>(&node.inverseBindTransform), sizeof(node.inverseBindTransform));
        }

        for (glm::mat4& boneOffset : outModel.boneOffsets) {
            file.read(reinterpret_cast<char*>(&boneOffset), sizeof(boneOffset));
        }

        for (uint32_t i = 0; i < header.boneCount; ++i) {
            uint32_t nameLength = 0;
            unsigned int boneIndex = 0;
            file.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));

            std::string boneName(nameLength, '\0');
            file.read(boneName.data(), boneName.size());
            file.read(reinterpret_cast<char*>(&boneIndex), sizeof(boneIndex));
            outModel.boneMapping[boneName] = boneIndex;
        }

        for (SkinnedMeshData& mesh : outModel.meshes) {
            uint32_t nameLength = 0;
            file.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
            file.read(reinterpret_cast<char*>(&mesh.vertexCount), sizeof(mesh.vertexCount));
            file.read(reinterpret_cast<char*>(&mesh.indexCount), sizeof(mesh.indexCount));
            file.read(reinterpret_cast<char*>(&mesh.localBaseVertex), sizeof(mesh.localBaseVertex));

            mesh.name.resize(nameLength);
            file.read(mesh.name.data(), mesh.name.size());
            file.read(reinterpret_cast<char*>(&mesh.aabbMin), sizeof(mesh.aabbMin));
            file.read(reinterpret_cast<char*>(&mesh.aabbMax), sizeof(mesh.aabbMax));
            file.read(reinterpret_cast<char*>(&mesh.requiresSkinning), sizeof(mesh.requiresSkinning));
            file.read(reinterpret_cast<char*>(&mesh.nonDeformingBoneIndex), sizeof(mesh.nonDeformingBoneIndex));

            mesh.vertices.resize(mesh.vertexCount);
            mesh.vertexWeights.resize(mesh.vertexCount);
            mesh.indices.resize(mesh.indexCount);
            file.read(reinterpret_cast<char*>(mesh.vertices.data()), mesh.vertices.size() * sizeof(Vertex));
            file.read(reinterpret_cast<char*>(mesh.vertexWeights.data()), mesh.vertexWeights.size() * sizeof(VertexWeight));
            file.read(reinterpret_cast<char*>(mesh.indices.data()), mesh.indices.size() * sizeof(uint32_t));
        }

        if (!file) {
            Logging::Error() << "AssetFormats::LoadSkinnedModel() failed while reading '" << path << "'\n";
            outModel = {};
            return false;
        }

        return true;
    }
}
