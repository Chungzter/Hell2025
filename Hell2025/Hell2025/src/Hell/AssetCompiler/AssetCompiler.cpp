#include "AssetCompiler.h"
#include "AssimpImporter.h"

#include "Bvh/Cpu/CpuBvh.h"
#include "Hell/AssetFormats/AssetFormats.h"
#include "Hell/File.h"
#include "Hell/ImageTools/ImageTools.h"
#include "Hell/Logging.h"

namespace Hell::AssetCompiler {

    namespace {
        ModelBvhData BuildModelBvh(const ModelData& model) {
            ModelBvhData result;
            result.timestamp = model.timestamp;
            result.bvhs.reserve(model.meshes.size());

            for (const MeshData& mesh : model.meshes) {
                const uint64_t bvhId = Bvh::Cpu::CreateMeshBvhFromVertexData(mesh.vertices, mesh.indices);
                MeshBvh* meshBvh = Bvh::Cpu::GetMeshBvhById(bvhId);

                if (!meshBvh) {
                    Logging::Error() << "AssetCompiler failed to build BVH for mesh '" << mesh.name << "'\n";
                    Bvh::Cpu::DestroyMeshBvh(bvhId);
                    return {};
                }

                result.bvhs.push_back(*meshBvh);
                Bvh::Cpu::DestroyMeshBvh(bvhId);
            }

            return result;
        }

        void CompileTextures() {
            for (FileInfo& fileInfo : File::IterateDirectory("res/textures/compress_me", { "png", "jpg", "tga" })) {
                const std::string outputPath = "res/textures/compressed/" + fileInfo.name + ".dds";
                if (!File::Exists(outputPath)) {
                    ImageTools::CreateAndExportDDS(fileInfo.path, outputPath, true);
                    Logging::Debug() << "Exported " << outputPath << "\n";
                }
            }
        }

        void CompileModels() {
            for (FileInfo& fileInfo : File::IterateDirectory("res/models_raw", { "obj", "fbx" })) {
                const std::string outputPath = "res/models/" + fileInfo.name + ".model";
                const uint64_t sourceTimestamp = File::GetLastModifiedTime(fileInfo.path);

                AssetFormats::ModelMetadata metadata;
                const bool outputIsCurrent =
                    File::Exists(outputPath) &&
                    AssetFormats::ReadModelMetadata(outputPath, metadata) &&
                    metadata.timestamp == sourceTimestamp;

                if (!outputIsCurrent) {
                    ModelData model = ImportModel(fileInfo.path);
                    if (!model.meshes.empty()) {
                        AssetFormats::SaveModel(outputPath, model);
                    }
                }
            }
        }

        void CompileModelBvhs() {
            for (FileInfo& fileInfo : File::IterateDirectory("res/models", { "model" })) {
                const std::string modelPath = fileInfo.path;
                const std::string bvhPath = "res/models/bvh/" + fileInfo.name + ".bvh";

                AssetFormats::ModelMetadata modelMetadata;
                if (!AssetFormats::ReadModelMetadata(modelPath, modelMetadata)) {
                    continue;
                }

                AssetFormats::ModelBvhMetadata bvhMetadata;
                const bool outputIsCurrent =
                    File::Exists(bvhPath) &&
                    AssetFormats::ReadModelBvhMetadata(bvhPath, bvhMetadata) &&
                    bvhMetadata.timestamp == modelMetadata.timestamp;

                if (outputIsCurrent) {
                    continue;
                }

                ModelData model;
                if (!AssetFormats::LoadModel(modelPath, model)) {
                    continue;
                }

                ModelBvhData bvh = BuildModelBvh(model);
                if (bvh.bvhs.size() == model.meshes.size()) {
                    AssetFormats::SaveModelBvh(bvhPath, bvh);
                }
            }
        }

        void CompileSkinnedModels() {
            for (FileInfo& fileInfo : File::IterateDirectory("res/skinned_models_raw", { "obj", "fbx" })) {
                const std::string outputPath = "res/skinned_models/" + fileInfo.name + ".skinnedmodel";
                const uint64_t sourceTimestamp = File::GetLastModifiedTime(fileInfo.path);

                AssetFormats::SkinnedModelMetadata metadata;
                const bool outputIsCurrent =
                    File::Exists(outputPath) &&
                    AssetFormats::ReadSkinnedModelMetadata(outputPath, metadata) &&
                    metadata.timestamp == sourceTimestamp;

                if (!outputIsCurrent) {
                    SkinnedModelData model = ImportSkinnedModel(fileInfo.path);
                    if (!model.meshes.empty()) {
                        AssetFormats::SaveSkinnedModel(outputPath, model);
                    }
                }
            }
        }
    }

    void CompileOutOfDateAssets() {
        CompileTextures();
        CompileModels();
        CompileModelBvhs();
        CompileSkinnedModels();
    }
}
