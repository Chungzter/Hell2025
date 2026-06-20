#include "File.h"
#include <fstream>
#include "Bvh/Cpu/CpuBvh.h"
#include "Util.h"

#define PRINT_MODEL_BVH_HEADERS_ON_READ 0
#define PRINT_MODEL_BVH_HEADERS_ON_WRITE 0
#define PRINT_MESH_BVH_HEADERS_ON_READ 0
#define PRINT_MESH_BVH_HEADERS_ON_WRITE 0

#include <iostream> // TODO: cleanup logging

namespace File {

    void ExportModelBvh(const ModelData& modelData) {
        std::string outputPath = "res/models/bvh/" + modelData.name + ".bvh";
        std::cout << outputPath << "\n";
        std::ofstream file(outputPath, std::ios::binary);
        if (!file.is_open()) {
            std::cout << "Failed to open file for writing: " << outputPath << "\n";
            return;
        }

        // Fill the header
        ModelBvhHeader modelHeader;
        modelHeader.version = 1;
        modelHeader.meshCount = modelData.meshCount;
        modelHeader.timestamp = modelData.timestamp;
        MemCopyFileSignature(modelHeader.signature, HELL_MODEL_BVH_SIGNATURE);

        // Write the header
        file.write(reinterpret_cast<const char*>(&modelHeader), sizeof(ModelBvhHeader));

        #if PRINT_MODEL_BVH_HEADERS_ON_WRITE
        PrintModelBvhHeader(modelHeader, "Wrote model bvh header: " + outputPath);
        #endif

        // Write the mesh data
        for (const MeshData& meshData : modelData.meshes) {
            uint64_t meshbvhId = Bvh::Cpu::CreateMeshBvhFromVertexData(meshData.vertices, meshData.indices);
            MeshBvh* meshBvh = Bvh::Cpu::GetMeshBvhById(meshbvhId);
            
            MeshBvhHeader meshHeader;
            meshHeader.nodeCount = meshBvh->m_nodes.size();
            meshHeader.floatCount = meshBvh->m_triangleData.size();
            MemCopyFileSignature(meshHeader.signature, HELL_MESH_BVH_SIGNATURE);

            // Write the mesh header
            file.write(reinterpret_cast<const char*>(&meshHeader), sizeof(MeshBvhHeader));

            // Writes node data
            file.write(reinterpret_cast<const char*>(meshBvh->m_nodes.data()), meshBvh->m_nodes.size() * sizeof(BvhNode));
            file.write(reinterpret_cast<const char*>(meshBvh->m_triangleData.data()), meshBvh->m_triangleData.size() * sizeof(float));

            #if PRINT_MESH_BVH_HEADERS_ON_WRITE
            PrintMeshBvhHeader(meshHeader, "Wrote mesh bvh: " + meshData.name);
            #endif
            
            Bvh::Cpu::DestroyMeshBvh(meshbvhId);
        }
        file.close();
        std::cout << "Exported: " << outputPath << "\n";
    }


    ModelBvhData ImportModelBvh(const std::string& filepath) {
        ModelBvhData modelBvhData;

        // Bail if file does not exist
        if (!Util::FileExists(filepath)) {
            std::cout << "File::ImportMovelBvh() failed: " << filepath << " does not exist\n";
            return modelBvhData;
        }

        // Attempt to load the file
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            std::cout << "File::ImportMovelBvh() failed: could not open: " << filepath << "\n";
            return modelBvhData;
        }
        
        // Read the header
        ModelBvhHeader modelBvhHeader = ReadModelBvhHeader(filepath);

        // Bail if signature invalid
        if (!CompareFileSignature(modelBvhHeader.signature, HELL_MODEL_BVH_SIGNATURE)) {
            std::cout << "File::ImportMovelBvh() failed: invalid ModelBvhHeader signature '" << modelBvhHeader.signature << "'\n";
            return modelBvhData;
        }

        #if PRINT_MODEL_BVH_HEADERS_ON_READ
        PrintModelBvhHeader(modelBvhHeader, "Read model bvh header in: " + filepath);
        #endif

        // Seek to the end of the header
        file.seekg(sizeof(ModelBvhHeader), std::ios::beg);

        FileInfo fileInfo = Util::GetFileInfoFromPath(filepath);

        // Resize MeshBvh vector ready for import
        modelBvhData.bvhs.resize(modelBvhHeader.meshCount);

        // Load each MeshBvh
        for (uint64_t i = 0; i < modelBvhHeader.meshCount; i++) {
            MeshBvh& meshBvh = modelBvhData.bvhs[i];

            // Read the header
            MeshBvhHeader meshBvhHeader = {};
            file.read(reinterpret_cast<char*>(&meshBvhHeader), sizeof(MeshBvhHeader));

            // Bail if signature invalid
            if (!CompareFileSignature(meshBvhHeader.signature, HELL_MESH_BVH_SIGNATURE)) {
                std::cout << "File::ImportMovelBvh() failed: invalid MeshBvhHeader signature '" << meshBvhHeader.signature << "'\n";
                return modelBvhData;
            }

            #if PRINT_MESH_BVH_HEADERS_ON_READ
            PrintMeshBvhHeader(meshBvhHeader, "Read mesh bvh header in: " + filepath);
            #endif

            // Read data
            meshBvh.m_nodes.resize(meshBvhHeader.nodeCount);
            meshBvh.m_triangleData.resize(meshBvhHeader.floatCount);
            file.read(reinterpret_cast<char*>(meshBvh.m_nodes.data()), meshBvhHeader.nodeCount * sizeof(BvhNode));
            file.read(reinterpret_cast<char*>(meshBvh.m_triangleData.data()), meshBvhHeader.floatCount * sizeof(float));
        }
        file.close();

        return modelBvhData;
    }

    ModelBvhHeader File::ReadModelBvhHeader(const std::string& filepath) {
        ModelBvhHeader header = {};
        std::ifstream file(filepath, std::ios::binary);

        // Check for invalid file AFTER trying to open
        if (!file.is_open()) {
            std::cerr << "ReadModelBvhHeader() failed to open file " << filepath << "\n"; 
            header = {};
            return header;
        }

        // Read file data
        file.read(reinterpret_cast<char*>(&header), sizeof(ModelBvhHeader));

        // Error check stream state after read attempt
        if (!file) {
            std::cerr << "ReadModelBvhHeader() failed to read file " << filepath << "\n";
            header = {};
            return header;
        }

        return header;
    }
}