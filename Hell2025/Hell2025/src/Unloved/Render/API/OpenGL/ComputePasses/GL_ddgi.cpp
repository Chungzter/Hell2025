#include "Unloved/Render/API/OpenGL/GL_renderer.h"

#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Hell/BVH/BVH.h"
#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Time.h"

#include "Unloved/Common/Constants.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Render/RenderDataManager.h"

#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/Systems/DirtyTracker/DirtyTracker.h"
#include "Unloved/Systems/DDGI/DDGIManager.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/Session/Session.h" // For Session::GetSessionTime(). It's a hack to prevent colorful probe glitch at start
#include "Unloved/World/World.h"

namespace OpenGL::Renderer {

    float g_time = 0.0f; // Hack to prevent colorful probe glitch at start

    GLuint g_pointCloudVao = 0;
    GLuint g_pointCloudVbo = 0;
    OpenGLTextureArray g_probeDistanceTextureArray;
    OpenGLTextureArray g_probeIrradianceTextureArray;

    void UploadPointCloud(Unloved::DDGIVolume& ddgiVolume);
    void ComputePointCloudBaseColor(Unloved::DDGIVolume& ddgiVolume);
    void ComputeProbePointIndices(Unloved::DDGIVolume& ddgiVolume);

    void ResetProbeStates(Unloved::DDGIVolume& ddgiVolume);
    void UpdateProbeStates(Unloved::DDGIVolume& ddgiVolume);
    void UpdateDistanceTexture(Unloved::DDGIVolume& ddgiVolume);
    void UpdateIrradianceTexture(Unloved::DDGIVolume& ddgiVolume);
    void ComputePointCloudLighting(Unloved::DDGIVolume& ddgiVolume);
    void ComputeProbeRelevance(Unloved::DDGIVolume& ddgiVolume);
    void ComputeProbeDistance(Unloved::DDGIVolume& ddgiVolume);
    void ComputeProbeDistanceBorder(Unloved::DDGIVolume& ddgiVolume);
    void ComputeIrradianceDirtyPointCheck(Unloved::DDGIVolume& ddgiVolume);
	void ComputeProbeIrradianceList(Unloved::DDGIVolume& ddgiVolume);
    void ComputeProbeIrradianceDispatchArgs();
    void ComputeProbeIrradiance(Unloved::DDGIVolume& ddgiVolume);
    void ComputeProbeIrradianceBorder(Unloved::DDGIVolume& ddgiVolume);
    void ComputeIrradianceTexture(Unloved::DDGIVolume& ddgiVolume);

    void ComputeProbePointIndices(Unloved::DDGIVolume& ddgiVolume) {
        ProfilerOpenGLZoneFunctionLightGreen();

        const Unloved::PointCloud& pointCloud = ddgiVolume.GetPointClound();
        const DDGIVolumeGPU ddgiVolumeGPU = ddgiVolume.GetGPUData();

        OpenGL::ClearSSBO("ProbeIndexCounter");

        OpenGL::UpdateSSBO("DDGIVolume", sizeof(DDGIVolumeGPU), &ddgiVolumeGPU);
        OpenGL::UpdateSSBO("PointCloudGridOffsets", pointCloud.GetGridCellOffsets().size() * sizeof(uint32_t), pointCloud.GetGridCellOffsets().data());
        OpenGL::UpdateSSBO("PointCloudGridCounts", pointCloud.GetGridCellCounts().size() * sizeof(uint32_t), pointCloud.GetGridCellCounts().data());

        //OpenGL::ReserveSSBO("PointCloudGridDirtyFlags", pointCloud.GetGridCellCounts().size() * sizeof(uint32_t));
        OpenGL::ReserveSSBO("PointCloudDirtyFlags", pointCloud.GetPointCount() * sizeof(uint32_t));
        OpenGL::ReserveSSBO("ProbePointIndices", sizeof(uint32_t) * ddgiVolume.GetProbePointIndexPoolSize());
        OpenGL::ReserveSSBO("ProbePointOffsets", sizeof(uint32_t) * ddgiVolume.GetTotalProbeCount());
        OpenGL::ReserveSSBO("ProbePointCounts", sizeof(uint32_t) * ddgiVolume.GetTotalProbeCount());

        OpenGL::BindSSBO(0, "DDGIVolume");
        OpenGL::BindSSBO(1, "PointCloudGridOffsets");
        OpenGL::BindSSBO(2, "PointCloudGridCounts");
        OpenGL::BindSSBO(3, "ProbePointIndices");
        OpenGL::BindSSBO(4, "ProbePointOffsets");
        OpenGL::BindSSBO(5, "ProbePointCounts");
        OpenGL::BindSSBO(6, "ProbeIndexCounter");
        OpenGL::BindSSBO(7, g_pointCloudVbo); // VBO bound as SSBO

        OpenGL::BindShader("ProbePointIndices");
        OpenGL::SetUniformVec3("u_gridMin", ddgiVolume.GetBoundsMin());
        OpenGL::SetUniformIVec3("u_gridDimensions", pointCloud.GetGridDimensions());
        OpenGL::SetUniformFloat("u_gridCellSize", pointCloud.GetGridCellSize());
        OpenGL::SetUniformInt("u_totalProbes", ddgiVolume.GetTotalProbeCount());

        glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);
        OpenGL::DispatchCompute(ddgiVolume.GetTotalProbeCount(), 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
    }

    OpenGLTextureArray& GetProbeDistanceTextureArray();
    OpenGLTextureArray& GetProbeIrradianceTextureArray();

    //void RenderSceneBvhTris(Unloved::DDGIVolume& ddgiVolume);

    void ResetDDGIProbes(Unloved::DDGIVolume& ddgiVolume) {
        OpenGL::ReserveSSBO("ProbeSHColor", sizeof(ProbeColor) * ddgiVolume.GetTotalProbeCount());
        OpenGL::ClearSSBO("ProbeSHColor");

        OpenGLTextureArray& probeIrradianceTexture = GetProbeIrradianceTextureArray();
        probeIrradianceTexture.Clear(0.0f, 0.0f, 0.0f, 0.0f);

        ResetProbeStates(ddgiVolume);
    }

    void UpdateGlobalIllumintation() {
        Hell::SlotMap<Unloved::DDGIVolume>& ddgiVolumes = Unloved::DDGIManager::GetVolumes();
        if (ddgiVolumes.empty()) return;

        Unloved::DDGIVolume& ddgiVolume = ddgiVolumes[0];
        const Unloved::PointCloud& pointCloud = ddgiVolume.GetPointClound();
        const uint32_t totalProbeCount = ddgiVolume.GetTotalProbeCount();

        // Reserve space before any reset/clear. Otherwise a later resize can replace cleared data with undefined buffer contents.
        OpenGL::ReserveSSBO("ProbeSHColor", sizeof(ProbeColor) * totalProbeCount);
        OpenGL::ReserveSSBO("ProbeDistanceIndices", sizeof(uint32_t) * totalProbeCount);
        OpenGL::ReserveSSBO("ProbeIrradianceIndices", sizeof(uint32_t) * totalProbeCount);
        OpenGL::ReserveSSBO("ProbeStates", sizeof(ProbeState) * totalProbeCount);

        if (ddgiVolume.PointCloudNeedsGPUUpload()) {
            UploadPointCloud(ddgiVolume);
            ComputeProbePointIndices(ddgiVolume);
            ComputePointCloudBaseColor(ddgiVolume);
            ResetDDGIProbes(ddgiVolume);
            ddgiVolume.MarkPointCloudAsUploaded();
        }

        ddgiVolume.UpdateDDGISceneBvh();

        uint64_t sceneBvhId = ddgiVolume.GetSceneBvhId();
        SceneBvh* sceneBvh = Hell::Bvh::GetSceneBvhById(sceneBvhId);
        if (!sceneBvh) return;

        const std::vector<BvhNode>& sceneNodes = sceneBvh->m_nodes;
        const std::vector<BvhNode>& meshBvhNodes = sceneBvh->m_meshNodes;
        const std::vector<GpuPrimitiveInstance>& entityInstances = sceneBvh->m_gpuInstances;
        const std::vector<BVHTriangle>& triangles = sceneBvh->m_triangles;

        const DDGIVolumeGPU ddgiVolumeGPU = ddgiVolume.GetGPUData();
        const std::vector<GPUAABB>& dirtyDoorABBBs = Unloved::DirtyTracker::GetDirtyDoorAABBs();

        // BVH data
        OpenGL::UpdateSSBO("SceneBvh", sceneNodes.size() * sizeof(BvhNode), sceneNodes.data());
        OpenGL::UpdateSSBO("MeshesBvh", meshBvhNodes.size() * sizeof(BvhNode), meshBvhNodes.data());
        OpenGL::UpdateSSBO("EntityInstances", entityInstances.size() * sizeof(GpuPrimitiveInstance), entityInstances.data());
        OpenGL::UpdateSSBO("TriangleData", triangles.size() * sizeof(BVHTriangle), triangles.data());

        // Probe/Pointcloud data
        OpenGL::UpdateSSBO("DDGIVolume", sizeof(DDGIVolumeGPU), &ddgiVolumeGPU);
        OpenGL::UpdateSSBO("DirtyDoorAABBs", dirtyDoorABBBs.size() * sizeof(GPUAABB), dirtyDoorABBBs.data());

        if (Unloved::DDGIManager::ConsumeProbeResetRequest()) {
            ResetDDGIProbes(ddgiVolume);
        }

        // Raytracing SSBOs stay persistently bound for whole GI pass
        OpenGL::BindSSBO(0, "EntityInstances");
        OpenGL::BindSSBO(1, "TriangleData");
        OpenGL::BindSSBO(2, "SceneBvh");
        OpenGL::BindSSBO(3, "MeshesBvh");

        UpdateDistanceTexture(ddgiVolume);
        UpdateIrradianceTexture(ddgiVolume);
        UpdateProbeStates(ddgiVolume);

        ComputePointCloudLighting(ddgiVolume);
		ComputeProbeRelevance(ddgiVolume);
        ComputeProbeDistance(ddgiVolume);
		ComputeProbeDistanceBorder(ddgiVolume);
        ComputeIrradianceDirtyPointCheck(ddgiVolume);
        ComputeProbeIrradianceList(ddgiVolume);
        ComputeProbeIrradianceDispatchArgs();
        ComputeProbeIrradiance(ddgiVolume);
        ComputeProbeIrradianceBorder(ddgiVolume);
        ComputeIrradianceTexture(ddgiVolume);

        //RenderSceneBvhTris(ddgiVolume);

        //DrawGPUBvhSceneNodes(ddgiVolume, RED);
        //DrawGPUBvhSceneLeafNodes(ddgiVolume, GREEN);
        //DrawRaytracingBvh(ddgiVolume);
    }

    void ResetProbeStates(Unloved::DDGIVolume& ddgiVolume) {
        std::vector<ProbeState> probeStates;
        probeStates.reserve(ddgiVolume.GetTotalProbeCount());

        for (uint32_t i = 0; i < ddgiVolume.GetTotalProbeCount(); i++) {
            ProbeState& probeState = probeStates.emplace_back();
            probeState.isActive = true;
            probeState.isRelevant = false;
            probeState.distanceCooldown = PROBE_MAX_DISTANCE_COOLDOWN;
            probeState.irradianceCooldown = PROBE_MAX_IRRADIANCE_COOLDOWN;
            probeState.relocationOffset = glm::vec3(0.0f);
        }

        OpenGL::UpdateSSBO("ProbeStates", probeStates.size() * sizeof(ProbeState), probeStates.data());

        g_time = 0.0f; // Hack to prevent colorful probe glitch at start
    }

    void UpdateProbeStates(Unloved::DDGIVolume& ddgiVolume) {
        ProfilerOpenGLZoneFunctionLightGreen();

        g_time += Hell::Time::DeltaTime(); // Hack to prevent colorful probe glitch at start

        OpenGL::BindSSBO(4, "DDGIVolume");
        OpenGL::BindSSBO(5, "ProbeStates");
        OpenGL::BindSSBO(6, "DirtyDoorAABBs");

        OpenGL::BindShader("ProbeStateUpdate");

        OpenGL::SetUniformInt("u_dirtyDoorAABBCount", (int)Unloved::DirtyTracker::GetDirtyDoorAABBs().size());
        OpenGL::SetUniformFloat("u_time", g_time); // Hack to prevent colorful probe glitch at start

        OpenGL::DispatchCompute((ddgiVolume.GetTotalProbeCount() + 63) / 64, 1, 1);
    }

    void ComputePointCloudLighting(Unloved::DDGIVolume& ddgiVolume) {
        ProfilerOpenGLZoneFunctionLightGreen();

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("PointCloudLighting");
        OpenGL::BindShader("PointCloudLighting");
        OpenGL::SetUniformInt("u_lightCount", Unloved::World::GetLightCount());

        OpenGL::BindSSBO(4, "Lights");
        OpenGL::BindSSBO(5, "LightAABBs");
        OpenGL::BindSSBO(6, g_pointCloudVbo);
        OpenGL::BindSSBO(7, "Samplers");
        OpenGL::BindSSBO(8, "PointCloudDirtyFlags");

        OpenGL::DispatchCompute((ddgiVolume.GetPointCloundPoints().size() + 127) / 128, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

	void ComputeProbeRelevance(Unloved::DDGIVolume& ddgiVolume) {
		ProfilerOpenGLZoneFunctionLightGreen();

		OpenGL::ClearSSBO("ProbeIrradianceCounter");

		OpenGL::BindSSBO(4, "ProbeStates");
		OpenGL::BindSSBO(5, "DDGIVolume");
		OpenGL::BindSSBO(6, "RendererData");
		OpenGL::BindSSBO(7, "ViewportData");

		OpenGL::BindShader("ProbeRelevance");
		OpenGL::SetUniformVec3("u_viewPos", Unloved::RenderDataManager::GetViewportData()[0].viewPos);
		OpenGL::SetUniformBool("u_msaaRenderer", false); // TODO: remove me from shader
		OpenGL::SetUniformBool("u_octNormals", Unloved::Renderer::GetRendererMode() == RendererMode::RE_STYLE);

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

		if (Unloved::Renderer::GetRendererMode() == RendererMode::RE_STYLE) {
			OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
			int32_t quarterWidth = (gBuffer.GetWidth() + 3) / 4;
			int32_t quarterHeight = (gBuffer.GetHeight() + 3) / 4;

			OpenGL::BindTextureUnit(2, gBuffer.GetColorAttachmentHandleByName("NormalXYRoughnessMisc"));
			OpenGL::BindTextureUnit(3, gBuffer.GetDepthAttachmentHandle());
			OpenGL::DispatchCompute((quarterWidth + 7) / 8, (quarterHeight + 7) / 8, 1);
		}
		else {
			OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
			int32_t quarterWidth = (gBuffer.GetWidth() + 3) / 4;
			int32_t quarterHeight = (gBuffer.GetHeight() + 3) / 4;

			OpenGL::BindTextureUnit(2, gBuffer.GetColorAttachmentHandleByName("NormalXYRoughnessMisc"));
			OpenGL::BindTextureUnit(3, gBuffer.GetDepthAttachmentHandle());
			OpenGL::DispatchCompute((quarterWidth + 7) / 8, (quarterHeight + 7) / 8, 1);
		}
	}

    void ComputeProbeDistance(Unloved::DDGIVolume& ddgiVolume) {
        ProfilerOpenGLZoneFunctionLightGreen();

        OpenGLShader* distanceShader = OpenGL::ResourceManager::GetShaderPtr("ProbeDistance");
        OpenGLShader* listShader = OpenGL::ResourceManager::GetShaderPtr("ProbeDistanceList");
        OpenGLShader* argsShader = OpenGL::ResourceManager::GetShaderPtr("ProbeDistanceDispatchArgs");

        if (!distanceShader || !listShader || !argsShader) return;

        OpenGLTextureArray& probeDistanceTexture = GetProbeDistanceTextureArray();
        OpenGL::BindImageTextureArray(0, probeDistanceTexture.GetHandle(), GL_READ_WRITE, GL_RG16F);

        static int frameIndex = 0;
        frameIndex++;

        OpenGL::ClearSSBO("ProbeDistanceCounter");

        OpenGL::BindSSBO(4, "DDGIVolume");
        OpenGL::BindSSBO(5, "ProbeStates");
        OpenGL::BindSSBO(6, "ProbeDistanceCounter");
        OpenGL::BindSSBO(7, "ProbeDistanceIndices");
        OpenGL::BindSSBO(8, "ProbeDistanceDispatchArgs");

        OpenGL::BindShader("ProbeDistanceList");
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        OpenGL::DispatchCompute((ddgiVolume.GetTotalProbeCount() + 63) / 64, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        OpenGL::BindShader("ProbeDistanceDispatchArgs");
        OpenGL::DispatchCompute(1, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);

        OpenGL::BindShader("ProbeDistance");
        OpenGL::SetUniformInt("u_frameIndex", frameIndex);
        OpenGL::BindDispatchBuffer("ProbeDistanceDispatchArgs");
        OpenGL::DispatchComputeIndirect();
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    void ComputeProbeDistanceBorder(Unloved::DDGIVolume& ddgiVolume) {
        ProfilerOpenGLZoneFunctionLightGreen();

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("ProbeDistanceBorder");
        if (!shader) return;

        OpenGL::BindShader("ProbeDistanceBorder");
        OpenGL::BindSSBO(4, "DDGIVolume");

        OpenGLTextureArray& probeDistanceTexture = GetProbeDistanceTextureArray();
        OpenGL::BindImageTextureArray(0, probeDistanceTexture.GetHandle(), GL_READ_WRITE, GL_RG16F);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        OpenGL::DispatchCompute((ddgiVolume.GetTotalProbeCount() + 63) / 64, 1, 1);
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    void ComputeIrradianceDirtyPointCheck(Unloved::DDGIVolume& ddgiVolume) {
        ProfilerOpenGLZoneFunctionLightGreen();

        OpenGL::BindShader("ProbeIrradianceDirtyPointCheck");

        OpenGL::BindSSBO(4, "ProbeStates");
        OpenGL::BindSSBO(5, "ProbePointIndices");
        OpenGL::BindSSBO(6, "ProbePointOffsets");
        OpenGL::BindSSBO(7, "ProbePointCounts");
        OpenGL::BindSSBO(8, "DDGIVolume");
        OpenGL::BindSSBO(9, g_pointCloudVbo); // VBO bound as SSBO
        OpenGL::BindSSBO(10, "PointCloudDirtyFlags");

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        //OpenGL::DispatchCompute((ddgiVolume.GetTotalProbeCount() + 63) / 64, 1, 1);
        OpenGL::DispatchCompute((ddgiVolume.GetTotalProbeCount() + 31) / 32, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

	void ComputeProbeIrradianceList(Unloved::DDGIVolume& ddgiVolume) {
		ProfilerOpenGLZoneFunctionLightGreen();

		OpenGL::BindSSBO(4, "ProbeStates");
		OpenGL::BindSSBO(5, "ProbeIrradianceCounter");
		OpenGL::BindSSBO(6, "ProbeIrradianceIndices");
		OpenGL::BindSSBO(7, "DDGIVolume");

		OpenGL::BindShader("ProbeIrradianceList");

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		OpenGL::DispatchCompute((ddgiVolume.GetTotalProbeCount() + 63) / 64, 1, 1);
    }

    void ComputeProbeIrradianceDispatchArgs() {
		ProfilerOpenGLZoneFunctionLightGreen();

		OpenGL::BindSSBO(4, "ProbeIrradianceCounter");
		OpenGL::BindSSBO(5, "ProbeIrradianceDispatchArgs");

        OpenGL::BindShader("ProbeLightingDispatchArgs");

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		OpenGL::DispatchCompute(1, 1, 1);
    }

    void ComputeProbeIrradiance(Unloved::DDGIVolume& ddgiVolume) {
        ProfilerOpenGLZoneFunctionLightGreen();

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("ProbeIrradiance");
        if (!shader) return;

        static int frameIndex = 0;
        frameIndex++;

        OpenGL::BindSSBO(4, g_pointCloudVbo);
        OpenGL::BindSSBO(5, "ProbeSHColor");
        OpenGL::BindSSBO(6, "ProbeIrradianceCounter");
        OpenGL::BindSSBO(7, "ProbeIrradianceIndices");
        OpenGL::BindSSBO(8, "DDGIVolume");
        OpenGL::BindSSBO(9, "ProbeStates");
        OpenGL::BindSSBO(10, "ProbePointIndices");
        OpenGL::BindSSBO(11, "ProbePointOffsets");
        OpenGL::BindSSBO(12, "ProbePointCounts");

        OpenGL::BindShader("ProbeIrradiance");
        OpenGL::SetUniformFloat("u_pointCloudSpacing", ddgiVolume.GetPointCloudSpacing());
        OpenGL::SetUniformInt("u_pointCount", (int32_t)ddgiVolume.GetPointCloudCount());
        OpenGL::SetUniformInt("u_frameIndex", frameIndex);
        OpenGL::SetUniformBool("u_useSH", Unloved::Renderer::GetCurrentRendererSettings().irradianceUsesSH);

        // These can go soon:
        const Unloved::PointCloud& pointCloud = ddgiVolume.GetPointClound();
        OpenGL::SetUniformVec3("u_gridMin", ddgiVolume.GetBoundsMin());
        OpenGL::SetUniformIVec3("u_gridDimensions", pointCloud.GetGridDimensions());
        OpenGL::SetUniformFloat("u_gridCellSize", pointCloud.GetGridCellSize());

        OpenGLTextureArray& probeIrradianceTexture = GetProbeIrradianceTextureArray();
        OpenGL::BindImageTextureArray(0, probeIrradianceTexture.GetHandle(), GL_READ_WRITE, GL_RGBA16F);

        OpenGL::BindDispatchBuffer("ProbeIrradianceDispatchArgs");

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);
        OpenGL::DispatchComputeIndirect();
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    void ComputeProbeIrradianceBorder(Unloved::DDGIVolume& ddgiVolume) {
        ProfilerOpenGLZoneFunctionLightGreen();

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("ProbeIrradianceBorder");
        if (!shader) return;

        OpenGL::BindSSBO(4, "DDGIVolume");
        OpenGL::BindShader("ProbeIrradianceBorder");

        OpenGLTextureArray& irradianceTexture = GetProbeIrradianceTextureArray();
        OpenGL::BindImageTextureArray(0, irradianceTexture.GetHandle(), GL_READ_WRITE, GL_RGBA16F);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        OpenGL::DispatchCompute((ddgiVolume.GetTotalProbeCount() + 63) / 64, 1, 1);
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

	void ComputePointCloudBaseColor(Unloved::DDGIVolume& ddgiVolume) {
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("PointCloudBaseColor");
        if (!shader) return;

        const std::vector<Unloved::CloudPointTextureInfo>& pointCloundTextureInfo = ddgiVolume.GetPointCloudTextureInfo();

        OpenGL::UpdateSSBO("PointCloudTextureInfo", pointCloundTextureInfo.size() * sizeof(Unloved::CloudPointTextureInfo), pointCloundTextureInfo.data());

        // Ensure bindless texture IDs are in the Samplers ID, which is not the case if this runs the first frame of the renderer
        OpenGL::UpdateSSBO("Samplers", sizeof(GLuint64) * OpenGL::BackEnd::GetBindlessTextureIDs().size(), OpenGL::BackEnd::GetBindlessTextureIDs().data());

		OpenGL::BindSSBO(0, "Samplers");
		OpenGL::BindSSBO(1, "PointCloudTextureInfo");
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, g_pointCloudVbo);

		GLuint numGroupsX = (ddgiVolume.GetPointCloudCount() + 127) / 128;

		OpenGL::BindShader("PointCloudBaseColor");
        OpenGL::DispatchCompute(numGroupsX, 1, 1);

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
    }

    void UploadPointCloud(Unloved::DDGIVolume& ddgiVolume) {
        if (g_pointCloudVao == 0) {
            glGenVertexArrays(1, &g_pointCloudVao);
            glGenBuffers(1, &g_pointCloudVbo);
        }

        const std::vector<Unloved::CloudPoint>& pointCloud = ddgiVolume.GetPointCloundPoints();

        // Point cloud
        glBindBuffer(GL_ARRAY_BUFFER, g_pointCloudVbo);
        glBindVertexArray(g_pointCloudVao);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Unloved::CloudPoint) * pointCloud.size(), pointCloud.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Unloved::CloudPoint), (void*)offsetof(Unloved::CloudPoint, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Unloved::CloudPoint), (void*)offsetof(Unloved::CloudPoint, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Unloved::CloudPoint), (void*)offsetof(Unloved::CloudPoint, directLighting));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Unloved::CloudPoint), (void*)offsetof(Unloved::CloudPoint, baseColor));

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        Logging::Debug() << "Uploaded point cloud to GPU (" << pointCloud.size() << " points)\n";
    }

    void DrawPointCloud(Unloved::DDGIVolume& ddgiVolume) {
        if (g_pointCloudVao == 0) return;

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("DebugPointCloud");
        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");

        if (!gBuffer) return;
        if (!shader) return;

        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();

        const Unloved::PointCloud& pointCloud = ddgiVolume.GetPointClound();

        OpenGL::BindShader("DebugPointCloud");
        OpenGL::SetUniformIVec3("u_pointCloudGridDimensions", pointCloud.GetGridDimensions());
        OpenGL::SetUniformFloat("u_pointCloudCellSize", pointCloud.GetGridCellSize());
        OpenGL::SetUniformVec3("u_volumeMinBounds", ddgiVolume.GetBoundsMin());

        OpenGL::BindSSBO(4, "Lights");
        OpenGL::BindSSBO(5, "PointCloudGridOffsets");
        OpenGL::BindSSBO(6, "PointCloudGridCounts");
        OpenGL::BindSSBO(7, "PointCloudDirtyFlags");

		OpenGLRasterizerState state;
		state.depthTestEnabled = true;
		state.cullfaceEnable = true;
		state.blendEnable = false;
		state.depthMask = true;
		state.depthFunc = GL_GREATER;
		OpenGL::RasterizerStateManager::ForceRasterizerState(state);

		OpenGLFrameBuffer* fbo = nullptr;

		if (Unloved::Renderer::GetRendererMode() == RendererMode::RE_STYLE) {
			fbo = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
			if (!fbo) return;

			fbo->Bind();
			fbo->DrawBuffer("Lighting");
        }
		else {
			fbo = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
			if (!fbo) return;

			fbo->Bind();
			fbo->DrawBuffer("Lighting");

			state.depthFunc = GL_LESS;
			OpenGL::RasterizerStateManager::ForceRasterizerState(state);
		}

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(fbo, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);
            OpenGL::SetUniformMat4("u_projectionView", viewportData[i].projectionViewReverseZ);

            glBindVertexArray(g_pointCloudVao);
            glDrawArrays(GL_POINTS, 0, ddgiVolume.GetPointCloudCount());
            glBindVertexArray(0);
        }
    }

    void DrawPointCloudGrid(Unloved::DDGIVolume& ddgiVolume) {
        ddgiVolume.GetPointClound().DebugDrawGrid();
    }

    void DrawProbes(Unloved::DDGIVolume& ddgiVolume) {
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("DebugProbes");

        if (!shader) return;

		OpenGL::BindShader("DebugProbes");
		OpenGL::SetUniformInt("u_probeDebugState", static_cast<int>(Unloved::Renderer::GetCurrentRendererSettings().probeDebugState));
        OpenGL::SetUniformBool("u_useSH", Unloved::Renderer::GetCurrentRendererSettings().irradianceUsesSH);

        OpenGLTextureArray& probeDistanceTexture = GetProbeDistanceTextureArray();
        OpenGL::BindTextureUnit(0, probeDistanceTexture.GetHandle());

        OpenGLTextureArray& probeIrradianceTexture = GetProbeIrradianceTextureArray();
        OpenGL::BindTextureUnit(1, probeIrradianceTexture.GetHandle());

		OpenGLRasterizerState state;
		state.depthTestEnabled = true;
		state.cullfaceEnable = true;
		state.blendEnable = false;
		state.depthMask = true;
		state.depthFunc = GL_GREATER;
		OpenGL::RasterizerStateManager::ForceRasterizerState(state);

        OpenGLFrameBuffer* fbo = nullptr;


		if (Unloved::Renderer::GetRendererMode() == RendererMode::RE_STYLE) {
			fbo = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
			if (!fbo) return;

			fbo->Bind();
			fbo->DrawBuffer("Lighting");
        }
        else {
            fbo = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
			if (!fbo) return;

            fbo->Bind();
            fbo->DrawBuffer("Lighting");
        }

        OpenGL::BindSSBO(6, "ProbeSHColor");
        OpenGL::BindSSBO(7, "DDGIVolume");
        OpenGL::BindSSBO(8, "ProbeStates");

        glBindVertexArray(OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());

        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();
        Model* sphereModel = Hell::ResourceManager::GetModelByName("Sphere");
        if (!sphereModel || sphereModel->GetMeshIndices().empty()) return;
        if (sphereModel->GetMeshCount() == 0) return;

        uint32_t meshId = sphereModel->GetMeshIndices()[0];
        Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshId);
        if (!mesh) return;

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

			OpenGL::Renderer::SetViewport(fbo, viewport);
			OpenGL::SetUniformInt("u_viewportIndex", i);
            OpenGL::SetUniformMat4("u_projectionView", viewportData[i].projectionViewReverseZ);

			glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), ddgiVolume.GetTotalProbeCount(), mesh->baseVertex);
        }
    }

    void RaytracedSceneDebug() {
        ProfilerOpenGLZoneFunctionLightGreen();

        OpenGLFrameBuffer* fbo = OpenGL::ResourceManager::GetFrameBufferPtr("IndirectDiffuse");
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("RaytraceScene");

        if (!fbo) return;
        if (!shader) return;

        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();

        OpenGL::BindShader("RaytraceScene");
        OpenGL::SetUniformMat4("u_projectionMatrix", viewportData[0].projection);
        OpenGL::SetUniformMat4("u_viewMatrix", viewportData[0].view);

        OpenGL::BindSSBO(0, "EntityInstances");
        OpenGL::BindSSBO(1, "TriangleData");
        OpenGL::BindSSBO(2, "SceneBvh");
        OpenGL::BindSSBO(3, "MeshesBvh");
        OpenGL::BindSSBO(4, "Lights");

        glBindImageTexture(0, fbo->GetColorAttachmentHandleByName("Color"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
        OpenGL::DispatchCompute(fbo->GetWidth() / 8, fbo->GetHeight() / 8, 1);
    }

    void ComputeIrradianceTexture(Unloved::DDGIVolume& ddgiVolume) {
        ProfilerOpenGLZoneFunctionLightGreen();

        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();

        OpenGLFrameBuffer* fbo = OpenGL::ResourceManager::GetFrameBufferPtr("IndirectDiffuse");
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("ProbeIrradianceTexture");

        if (!fbo) return;
        if (!shader) return;

        OpenGL::BindShader("ProbeIrradianceTexture");
        OpenGL::SetUniformMat4("u_projectionMatrix", viewportData[0].projection);
        OpenGL::SetUniformVec3("u_cameraPos", viewportData[0].viewPos);
        OpenGL::SetUniformMat4("u_viewMatrix", viewportData[0].view);
        OpenGL::SetUniformBool("u_useSH", Unloved::Renderer::GetCurrentRendererSettings().irradianceUsesSH);

        OpenGL::BindSSBO(0, "EntityInstances");
        OpenGL::BindSSBO(1, "TriangleData");
        OpenGL::BindSSBO(2, "SceneBvh");
        OpenGL::BindSSBO(3, "MeshesBvh");
        OpenGL::BindSSBO(4, "Lights");
        OpenGL::BindSSBO(5, "ProbeSHColor");
        OpenGL::BindSSBO(6, "ProbeStates");
        OpenGL::BindSSBO(7, "DDGIVolume");
        OpenGL::BindSSBO(8, "RendererData");
        OpenGL::BindSSBO(9, "ViewportData");

        OpenGL::BindImageTexture(0, fbo->GetColorAttachmentHandleByName("Color"), GL_WRITE_ONLY, GL_R11F_G11F_B10F);

        if (Unloved::Renderer::GetRendererMode() == RendererMode::RE_STYLE) {
            OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
            if (!gBuffer) return;

            OpenGL::BindTextureUnit(2, gBuffer->GetColorAttachmentHandleByName("NormalXYRoughnessMisc"));
            OpenGL::BindTextureUnit(3, gBuffer->GetDepthAttachmentHandle());

            OpenGL::SetUniformBool("u_octalNormals", true);
        }
        else {
            OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
            if (!gBuffer) return;

            OpenGL::BindTextureUnit(2, gBuffer->GetColorAttachmentHandleByName("NormalXYRoughnessMisc"));
            OpenGL::BindTextureUnit(3, gBuffer->GetDepthAttachmentHandle());

            OpenGL::SetUniformBool("u_octalNormals", false);
        }

        OpenGLTextureArray& probeDistanceTexture = GetProbeDistanceTextureArray();
        OpenGL::BindTextureUnit(4, probeDistanceTexture.GetHandle());

        OpenGLTextureArray& probeIrradianceTexture = GetProbeIrradianceTextureArray();
        OpenGL::BindTextureUnit(5, probeIrradianceTexture.GetHandle());

        OpenGL::DispatchCompute(fbo->GetWidth() / TILE_SIZE, fbo->GetHeight() / TILE_SIZE, 1);
    }

    void UpdateDistanceTexture(Unloved::DDGIVolume& ddgiVolume) {
        uint32_t probeCountX = ddgiVolume.GetProbeCountX();
        uint32_t probeCountY = ddgiVolume.GetProbeCountY();
        uint32_t probeCountZ = ddgiVolume.GetProbeCountZ();

        uint32_t layerWidth = probeCountX * 16;
        uint32_t layerHeight = probeCountZ * 16;
        uint32_t layerCount = probeCountY;

        // Skip if texture is already the correct size
        if (g_probeDistanceTextureArray.GetWidth() == layerWidth &&
            g_probeDistanceTextureArray.GetHeight() == layerHeight &&
            g_probeDistanceTextureArray.GetCount() == layerCount) {
            return;
        }

        g_probeDistanceTextureArray.AllocateMemory(layerWidth, layerHeight, GL_RG16F, 1, layerCount);
        g_probeDistanceTextureArray.SetMinFilter(TextureFilter::LINEAR);
        g_probeDistanceTextureArray.SetMagFilter(TextureFilter::LINEAR);
        g_probeDistanceTextureArray.SetWrapMode(TextureWrapMode::CLAMP_TO_EDGE);

        float maxDist = ddgiVolume.GetProbeSpacing() * 1.5f;
        float clearValues[4] = { maxDist, maxDist * maxDist, 0.0f, 0.0f };

        // Pre fill entire texture array to max distance
        glClearTexImage(g_probeDistanceTextureArray.GetHandle(), 0, GL_RG, GL_FLOAT, clearValues);
    }

    void UpdateIrradianceTexture(Unloved::DDGIVolume& ddgiVolume) {
        uint32_t probeCountX = ddgiVolume.GetProbeCountX();
        uint32_t probeCountY = ddgiVolume.GetProbeCountY();
        uint32_t probeCountZ = ddgiVolume.GetProbeCountZ();

        uint32_t layerWidth = probeCountX * 8;
        uint32_t layerHeight = probeCountZ * 8;
        uint32_t layerCount = probeCountY;

        // Skip if texture is already the correct size
        if (g_probeIrradianceTextureArray.GetWidth() == layerWidth &&
            g_probeIrradianceTextureArray.GetHeight() == layerHeight &&
            g_probeIrradianceTextureArray.GetCount() == layerCount) {
            return;
        }

        g_probeIrradianceTextureArray.AllocateMemory(layerWidth, layerHeight, GL_RGBA16F, 1, layerCount);
        g_probeIrradianceTextureArray.SetMinFilter(TextureFilter::LINEAR);
        g_probeIrradianceTextureArray.SetMagFilter(TextureFilter::LINEAR);
        g_probeIrradianceTextureArray.SetWrapMode(TextureWrapMode::CLAMP_TO_EDGE);

        // Pre fill entire texture array to pitch black
        float clearValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        glClearTexImage(g_probeIrradianceTextureArray.GetHandle(), 0, GL_RGBA, GL_FLOAT, clearValues);
    }

    void DrawGPUBvhSceneNodes(Unloved::DDGIVolume& volume, const glm::vec4& color) {
        const std::vector<BvhNode>& sceneNodes = volume.GetSceneNodes();

        for (const BvhNode& node : sceneNodes) {
            AABB aabb(node.boundsMin, node.boundsMax);
            DebugDraw::DrawAABB(aabb, color);
        }
    }

    void DrawGPUBvhSceneLeafNodes(Unloved::DDGIVolume& volume, const glm::vec4& color) {
        const std::vector<BvhNode>& sceneNodes = volume.GetSceneNodes();

        for (const BvhNode& node : sceneNodes) {
            if (node.primitiveCount > 0) {
                AABB aabb(node.boundsMin, node.boundsMax);
                DebugDraw::DrawAABB(aabb, color);
            }
        }
    }

    void DrawRaytracingBvh(Unloved::DDGIVolume& volume) {
        uint64_t sceneBvhId = volume.GetSceneBvhId();
        SceneBvh* sceneBvh = Hell::Bvh::GetSceneBvhById(sceneBvhId);
        if (!sceneBvh) return;

        const std::vector<BvhNode>& sceneNodes = sceneBvh->m_nodes;
        const std::vector<BvhNode>& meshBvhNodes = sceneBvh->m_meshNodes;
        const std::vector<BVHTriangle>& triangles = sceneBvh->m_triangles;
        const std::vector<GpuPrimitiveInstance>& instances = sceneBvh->m_gpuInstances;

        if (sceneNodes.empty()) return;

        uint32_t sceneStack[32];
        uint32_t sceneStackSize = 0;

        // push scene root node
        sceneStack[sceneStackSize++] = 0;

        // walk scene bvh
        while (sceneStackSize > 0) {
            uint32_t sceneNodeIndex = sceneStack[--sceneStackSize];
            const BvhNode& sceneNode = sceneNodes[sceneNodeIndex];

            if (sceneNode.primitiveCount > 0) {
                // walk instances in scene leaf node
                for (uint32_t i = 0; i < sceneNode.primitiveCount; ++i) {
                    uint32_t instanceIdx = sceneNode.firstChildOrPrimitive + i;
                    const GpuPrimitiveInstance& instance = instances[instanceIdx];

                    // skip house
                    // if (instance.rootNodeIndex == 0) continue;

                    uint32_t meshStack[32];
                    uint32_t meshStackSize = 0;

                    // push mesh root node
                    meshStack[meshStackSize++] = instance.rootNodeIndex;

                    // walk mesh bvh
                    while (meshStackSize > 0) {
                        uint32_t meshNodeIndex = meshStack[--meshStackSize];
                        const BvhNode& meshNode = meshBvhNodes[meshNodeIndex];

                        if (meshNode.primitiveCount > 0) {
                            // draw triangles in mesh leaf node
                            for (uint32_t j = 0; j < meshNode.primitiveCount; ++j) {
                                uint32_t floatOffset = meshNode.firstChildOrPrimitive + (j * 12);
                                const BVHTriangle& triangle = triangles[floatOffset / 12];

                                glm::vec3 p0 = glm::vec3(triangle.v0_and_e1x);
                                glm::vec3 e1 = glm::vec3(triangle.v0_and_e1x.w, triangle.e1yz_and_e2xy.x, triangle.e1yz_and_e2xy.y);
                                glm::vec3 e2 = glm::vec3(triangle.e1yz_and_e2xy.z, triangle.e1yz_and_e2xy.w, triangle.e2z_and_normal.x);

                                glm::vec3 p1 = p0 - e1;
                                glm::vec3 p2 = p0 + e2;

                                glm::vec3 worldP0 = instance.worldTransform * glm::vec4(p0, 1.0f);
                                glm::vec3 worldP1 = instance.worldTransform * glm::vec4(p1, 1.0f);
                                glm::vec3 worldP2 = instance.worldTransform * glm::vec4(p2, 1.0f);

                                DebugDraw::DrawLine(worldP0, worldP1, WHITE);
                                DebugDraw::DrawLine(worldP1, worldP2, WHITE);
                                DebugDraw::DrawLine(worldP2, worldP0, WHITE);
                            }
                        }
                        else {
                            // push internal mesh children
                            meshStack[meshStackSize++] = meshNode.firstChildOrPrimitive;
                            meshStack[meshStackSize++] = meshNode.firstChildOrPrimitive + 1;
                        }
                    }
                }
            }
            else {
                // push internal scene children
                sceneStack[sceneStackSize++] = sceneNode.firstChildOrPrimitive;
                sceneStack[sceneStackSize++] = sceneNode.firstChildOrPrimitive + 1;
            }
        }
    }

    OpenGLTextureArray& GetProbeDistanceTextureArray() {
        return g_probeDistanceTextureArray;
    }

    OpenGLTextureArray& GetProbeIrradianceTextureArray() {
        return g_probeIrradianceTextureArray;
    }
}
