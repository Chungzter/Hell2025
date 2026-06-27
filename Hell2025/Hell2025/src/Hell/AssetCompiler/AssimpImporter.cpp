#include "AssimpImporter.h"

#include "Hell/Common/String.h"
#include "Hell/File.h"
#include "Hell/Math/Sanitize.h"

#include <assimp/matrix4x4.h>
#include <assimp/Importer.hpp>
#include <assimp/Scene.h>
#include <assimp/PostProcess.h>
#include <numeric>

#include <map>
#include <unordered_map>
#include <string>

#include <iostream> // TODO: cleanup logging

namespace {

    glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4& from) {
        constexpr float threshold = 1e-5f;
        glm::mat4 to;
        to[0][0] = Hell::Sanitize(from.a1, threshold); to[1][0] = Hell::Sanitize(from.a2, threshold); to[2][0] = Hell::Sanitize(from.a3, threshold); to[3][0] = Hell::Sanitize(from.a4, threshold);
        to[0][1] = Hell::Sanitize(from.b1, threshold); to[1][1] = Hell::Sanitize(from.b2, threshold); to[2][1] = Hell::Sanitize(from.b3, threshold); to[3][1] = Hell::Sanitize(from.b4, threshold);
        to[0][2] = Hell::Sanitize(from.c1, threshold); to[1][2] = Hell::Sanitize(from.c2, threshold); to[2][2] = Hell::Sanitize(from.c3, threshold); to[3][2] = Hell::Sanitize(from.c4, threshold);
        to[0][3] = Hell::Sanitize(from.d1, threshold); to[1][3] = Hell::Sanitize(from.d2, threshold); to[2][3] = Hell::Sanitize(from.d3, threshold); to[3][3] = Hell::Sanitize(from.d4, threshold);
        return to;
    }

}

namespace Hell::AssetCompiler {

    ModelData ImportModel(const std::string& filepath) {
        ModelData modelData;
        Assimp::Importer importer;
        importer.SetPropertyBool(AI_CONFIG_PP_FD_REMOVE, true);
        const aiScene* scene = importer.ReadFile(filepath,
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_ImproveCacheLocality |
            aiProcess_RemoveRedundantMaterials |
            aiProcess_FlipUVs
        );
        if (!scene) {
            std::cout << "LoadAndExportCustomFormat() failed to loaded model " << filepath << "\n";
            std::cerr << "Assimp Error: " << importer.GetErrorString() << "\n";
            return modelData;
        }
        modelData.name = File::GetName(filepath);
        modelData.meshCount = scene->mNumMeshes;
        modelData.meshes.resize(modelData.meshCount);
        modelData.timestamp = File::GetLastModifiedTime(filepath);

        // Pre allocate vector memory
        std::unordered_map<std::string, int> meshNameCounts;
        for (int i = 0; i < modelData.meshes.size(); i++) {
            MeshData& meshData = modelData.meshes[i];
            meshData.vertexCount = scene->mMeshes[i]->mNumVertices;
            meshData.indexCount = scene->mMeshes[i]->mNumFaces * 3;
            meshData.vertices.resize(meshData.vertexCount);
            meshData.indices.resize(meshData.indexCount);

            std::string rawName = scene->mMeshes[i]->mName.C_Str();
            // Remove blender naming mess
            rawName = rawName.substr(0, rawName.find('.'));

            meshNameCounts[rawName]++;
            if (meshNameCounts[rawName] > 1) {
                meshData.name = rawName + std::to_string(meshNameCounts[rawName]);
            }
            else {
                meshData.name = rawName;
            }
        }
        // Populate vectors
        for (int i = 0; i < modelData.meshes.size(); i++) {
            MeshData& meshData = modelData.meshes[i];
            const aiMesh* assimpMesh = scene->mMeshes[i];

            // Vertices
            for (unsigned int j = 0; j < meshData.vertexCount; j++) {
                meshData.vertices[j] = (Vertex{
                    // Pos
                    glm::vec3(assimpMesh->mVertices[j].x, assimpMesh->mVertices[j].y, assimpMesh->mVertices[j].z),
                    // Normal
                    glm::vec3(assimpMesh->mNormals[j].x, assimpMesh->mNormals[j].y, assimpMesh->mNormals[j].z),
                    // UV
                    assimpMesh->HasTextureCoords(0) ? glm::vec2(assimpMesh->mTextureCoords[0][j].x, assimpMesh->mTextureCoords[0][j].y) : glm::vec2(0.0f, 0.0f),
                    // Tangent
                    assimpMesh->HasTangentsAndBitangents() ? glm::vec3(assimpMesh->mTangents[j].x, assimpMesh->mTangents[j].y, assimpMesh->mTangents[j].z) : glm::vec3(0.0f)
                    });
                // Compute AABB
                meshData.aabbMin = glm::min(meshData.vertices[j].position, meshData.aabbMin);
                meshData.aabbMax = glm::max(meshData.vertices[j].position, meshData.aabbMax);
            }

            // Get indices
            for (unsigned int j = 0; j < assimpMesh->mNumFaces; j++) {
                const aiFace& face = assimpMesh->mFaces[j];
                unsigned int baseIndex = j * 3;
                meshData.indices[baseIndex] = face.mIndices[0];
                meshData.indices[baseIndex + 1] = face.mIndices[1];
                meshData.indices[baseIndex + 2] = face.mIndices[2];
            }

            // Normalize the normals for each vertex
            for (Vertex& vertex : meshData.vertices) {
                vertex.normal = glm::normalize(vertex.normal);
            }

            // Generate Tangents
            for (int j = 0; j < meshData.indices.size(); j += 3) {
                Vertex* vert0 = &meshData.vertices[meshData.indices[j]];
                Vertex* vert1 = &meshData.vertices[meshData.indices[j + 1]];
                Vertex* vert2 = &meshData.vertices[meshData.indices[j + 2]];
                glm::vec3 deltaPos1 = vert1->position - vert0->position;
                glm::vec3 deltaPos2 = vert2->position - vert0->position;
                glm::vec2 deltaUV1 = vert1->uv - vert0->uv;
                glm::vec2 deltaUV2 = vert2->uv - vert0->uv;
                float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
                glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
                glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;
                vert0->tangent = tangent;
                vert1->tangent = tangent;
                vert2->tangent = tangent;
            }

            modelData.aabbMin = glm::min(modelData.aabbMin, meshData.aabbMin);
            modelData.aabbMax = glm::max(modelData.aabbMax, meshData.aabbMax);
        }
        importer.FreeScene();
        return modelData;
    }

    void GrabSkeleton2(std::vector<Node>& nodes, const aiNode* pNode, int parentIndex) {
        // Create the joint node
        Node node;
        node.name = pNode->mName.C_Str();
        node.inverseBindTransform = aiMatrix4x4ToGlm(pNode->mTransformation);
        node.parentIndex = parentIndex;

        // Determine the current node's index and push it
        int currentIndex = static_cast<int>(nodes.size());
        nodes.push_back(node);

        // Recursively process children using the current node's index as parentIndex
        for (unsigned int i = 0; i < pNode->mNumChildren; i++) {
            GrabSkeleton2(nodes, pNode->mChildren[i], currentIndex);
        }
    }

    void GrabSkeleton(std::vector<Node>& nodes, const aiNode* pNode, int parentIndex) {
        // Skip nodes that contain mesh data to avoid name collisions with bone nodes
        // Still process children to keep the hierarchy intact
        if (pNode->mNumMeshes > 0) {
            for (unsigned int i = 0; i < pNode->mNumChildren; i++) {
                // Pass the received parentIndex down to bypass this mesh node
                GrabSkeleton(nodes, pNode->mChildren[i], parentIndex);
            }
            return;
        }

        Node node;
        node.name = pNode->mName.C_Str();
        node.inverseBindTransform = aiMatrix4x4ToGlm(pNode->mTransformation);
        node.parentIndex = parentIndex;

        // Determine the current node's index and push it
        int currentIndex = static_cast<int>(nodes.size());
        nodes.push_back(node);

        // Recursively process children using the current node's index as parentIndex
        for (unsigned int i = 0; i < pNode->mNumChildren; i++) {
            GrabSkeleton(nodes, pNode->mChildren[i], currentIndex);
        }
    }

    SkinnedModelData ImportSkinnedModel(const std::string& filepath) {
        SkinnedModelData modelData;

        unsigned int flags =
            aiProcess_LimitBoneWeights |
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_FlipUVs |
            aiProcess_CalcTangentSpace;

        // NEW_RIG_FILE
        const std::string modelName = File::GetName(filepath);
        if (modelName == "Knife" ||
            modelName == "Tokarev" ||
            modelName == "GoldenGlock" ||
            modelName == "SPAS" ||
            modelName == "P90") {
            flags =
                aiProcess_LimitBoneWeights |
                aiProcess_Triangulate |
                aiProcess_GenSmoothNormals |
                aiProcess_FlipUVs |
                aiProcess_CalcTangentSpace |
                aiProcess_GlobalScale; // This list adds this flag
        }

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(filepath.c_str(), flags);

        if (!scene) {
            std::cout << "Something fucked up loading your skinned model: " << filepath << "\n";
            std::cout << "Error: " << importer.GetErrorString() << "\n";
            return modelData;
        }

        modelData.name = File::GetName(filepath);
        modelData.meshes.resize(scene->mNumMeshes);
        modelData.timestamp = File::GetLastModifiedTime(filepath);
        modelData.vertexCount = 0u;
        modelData.indexCount = 0u;

        // Load bones
        int foundBoneCount = 0;
        for (int i = 0; i < scene->mNumMeshes; i++) {
            const aiMesh* assimpMesh = scene->mMeshes[i];

            for (unsigned int j = 0; j < assimpMesh->mNumBones; j++) {
                const aiBone* bone = assimpMesh->mBones[j];
                std::string boneName = bone->mName.data;

                // If this bone isn't mapped yet, add it to the global list
                bool inserted = modelData.boneMapping.find(boneName) != modelData.boneMapping.end();
                if (!inserted) {

                    // Map bone name to index
                    unsigned int boneIndex = foundBoneCount++;
                    modelData.boneMapping[boneName] = boneIndex;

                    // Store bone info
                    glm::mat4 boneOffset = aiMatrix4x4ToGlm(bone->mOffsetMatrix);
                    modelData.boneOffsets.push_back(boneOffset);

                    //if (modelData.name == "Knife") {
                    //    std::cout << boneName << ": " << boneIndex << "\n";
                    //}
                }
            }
        }

        // Recursively grab the skeleton
        GrabSkeleton(modelData.nodes, scene->mRootNode, -1);

        // Get vertex data
        std::unordered_map<std::string, int> meshNameCounts;
        for (int i = 0; i < scene->mNumMeshes; i++) {
            const aiMesh* assimpMesh = scene->mMeshes[i];

            SkinnedMeshData& meshData = modelData.meshes[i];;
            meshData.aabbMin = glm::vec3(std::numeric_limits<float>::max());
            meshData.aabbMax = glm::vec3(-std::numeric_limits<float>::max());
            meshData.vertexCount = assimpMesh->mNumVertices;
            meshData.indexCount = assimpMesh->mNumFaces * 3;

            std::string rawName = assimpMesh->mName.C_Str();
            meshNameCounts[rawName]++;
            if (meshNameCounts[rawName] > 1) {
                meshData.name = rawName + std::to_string(meshNameCounts[rawName]);
            }
            else {
                meshData.name = rawName;
            }

            meshData.vertices.reserve(meshData.vertexCount);
            meshData.vertexWeights.resize(meshData.vertexCount);
            meshData.indices.reserve(meshData.indexCount);

            // Get vertices
            for (unsigned int j = 0; j < meshData.vertexCount; j++) {
                Vertex& vertex = meshData.vertices.emplace_back();
                vertex.position = { assimpMesh->mVertices[j].x, assimpMesh->mVertices[j].y, assimpMesh->mVertices[j].z };
                vertex.normal = { assimpMesh->mNormals[j].x, assimpMesh->mNormals[j].y, assimpMesh->mNormals[j].z };
                vertex.normal = glm::normalize(vertex.normal);

                // avoid segfault if mesh lacks uvs and normalize to fix float drift
                if (assimpMesh->HasTangentsAndBitangents()) {
                    vertex.tangent = { assimpMesh->mTangents[j].x, assimpMesh->mTangents[j].y, assimpMesh->mTangents[j].z };
                    vertex.tangent = glm::normalize(vertex.tangent);
                }
                else {
                    vertex.tangent = glm::vec3(0.0f);
                }

                vertex.uv = { assimpMesh->HasTextureCoords(0) ? glm::vec2(assimpMesh->mTextureCoords[0][j].x, assimpMesh->mTextureCoords[0][j].y) : glm::vec2(0.0f, 0.0f) };

                meshData.aabbMin.x = std::min(meshData.aabbMin.x, vertex.position.x);
                meshData.aabbMin.y = std::min(meshData.aabbMin.y, vertex.position.y);
                meshData.aabbMin.z = std::min(meshData.aabbMin.z, vertex.position.z);
                meshData.aabbMax.x = std::max(meshData.aabbMax.x, vertex.position.x);
                meshData.aabbMax.y = std::max(meshData.aabbMax.y, vertex.position.y);
                meshData.aabbMax.z = std::max(meshData.aabbMax.z, vertex.position.z);
            }
            // Get indices
            for (unsigned int j = 0; j < assimpMesh->mNumFaces; j++) {
                const aiFace& Face = assimpMesh->mFaces[j];
                meshData.indices.push_back(Face.mIndices[0]);
                meshData.indices.push_back(Face.mIndices[1]);
                meshData.indices.push_back(Face.mIndices[2]);
            }

            // Get vertex weights and bone IDs
            std::vector<unsigned int> influenceCount(meshData.vertices.size(), 0);

            for (unsigned int i = 0; i < assimpMesh->mNumBones; i++) {
                std::string boneName = assimpMesh->mBones[i]->mName.data;
                unsigned int boneIndex = modelData.boneMapping[boneName];

                for (unsigned int j = 0; j < assimpMesh->mBones[i]->mNumWeights; j++) {
                    unsigned int vertexIndex = assimpMesh->mBones[i]->mWeights[j].mVertexId;
                    float weight = assimpMesh->mBones[i]->mWeights[j].mWeight;


                    VertexWeight& vertexWeight = meshData.vertexWeights[vertexIndex];

                    if (influenceCount[vertexIndex] < 4) {
                        switch (influenceCount[vertexIndex]) {
                        case 0:
                            vertexWeight.boneID.x = boneIndex;
                            vertexWeight.weight.x = weight;
                            break;
                        case 1:
                            vertexWeight.boneID.y = boneIndex;
                            vertexWeight.weight.y = weight;
                            break;
                        case 2:
                            vertexWeight.boneID.z = boneIndex;
                            vertexWeight.weight.z = weight;
                            break;
                        case 3:
                            vertexWeight.boneID.w = boneIndex;
                            vertexWeight.weight.w = weight;
                            break;
                        }
                        influenceCount[vertexIndex]++;
                    }
                }
            }

            // Ignore broken weights
            float threshold = 0.05f;
            for (unsigned int j = 0; j < meshData.vertexWeights.size(); j++) {
                VertexWeight& vertexWeight = meshData.vertexWeights[j];
                std::vector<float> validWeights;
                for (int i = 0; i < 4; ++i) {
                    if (vertexWeight.weight[i] < threshold) {
                        vertexWeight.weight[i] = 0.0f;
                    }
                    else {
                        validWeights.push_back(vertexWeight.weight[i]);
                    }
                }
                float sum = std::accumulate(validWeights.begin(), validWeights.end(), 0.0f);
                int validIndex = 0;
                for (int i = 0; i < 4; ++i) {
                    if (vertexWeight.weight[i] > 0.0f) {
                        vertexWeight.weight[i] = validWeights[validIndex] / sum;
                        validIndex++;
                    }
                }
            }

            // Check if all vertices have only one weight
            bool allVerticeHaveOnlyOneWeight = true;

            for (VertexWeight& vertexWeight : meshData.vertexWeights) {
                if (vertexWeight.weight.y != 0 &&
                    vertexWeight.weight.z != 0 &&
                    vertexWeight.weight.w != 0) {
                    allVerticeHaveOnlyOneWeight = false;
                }
            }

            // If they do, now check they all reference the same bone
            int foundBoneIndex = meshData.vertexWeights[0].boneID[0];
            bool allVerticesAlsoOnlyReferenceTheSameBone = true;

            if (allVerticeHaveOnlyOneWeight) {
                for (int i = 1; i < meshData.vertexWeights.size(); i++) {
                    VertexWeight& vertexWeight = meshData.vertexWeights[i];
                    if (vertexWeight.boneID.x != foundBoneIndex) {
                        allVerticesAlsoOnlyReferenceTheSameBone = false;
                        break;
                    }
                }
            }

            // SET THE BOOLEAN
            if (allVerticeHaveOnlyOneWeight && allVerticesAlsoOnlyReferenceTheSameBone) {
                meshData.requiresSkinning = false;
                meshData.nonDeformingBoneIndex = foundBoneIndex;
            }
            else {
                meshData.requiresSkinning = true;
                meshData.nonDeformingBoneIndex = -1;
            }

            std::cout << modelData.name << " [" << meshData.name << "]: " << Hell::String::FormatBool(meshData.requiresSkinning) << " " << foundBoneIndex << " nonDeformingBoneIndex " << meshData.vertexCount << " verts \n";

            modelData.vertexCount += (uint32_t)meshData.vertices.size();
            modelData.indexCount += (uint32_t)meshData.indices.size();
        }

        // Cleanup
        importer.FreeScene();

        return modelData;
    }
}
