#include "API/OpenGL/Renderer/GL_renderer.h"
#include "API/OpenGL/GL_backend.h"
#include "Debug/DebugDraw.h""
#include "Renderer/Renderer.h"
#include "Renderer/RenderDataManager.h"
#include "GlobalIllumination/GlobalIllumination.h"
#include "Viewport/ViewportManager.h"
#include "World/World.h"
#include "Util/Util.h"

#include <Game/Constants.h>
#include "Hell/BVH/BVH.h"
#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Core/GameOLD.h" // For Game::GetTotalTime(). It's a hack to prevent colorful probe glitch at start

namespace OpenGLRenderer {

    float g_time = 0.0f; // Hack to prevent colorful probe glitch at start

    GLuint g_pointCloudVao = 0;
    GLuint g_pointCloudVbo = 0;
    OpenGLTextureArray g_probeDistanceTextureArray;
    OpenGLTextureArray g_probeIrradianceTextureArray;

    void UploadPointCloud(DDGIVolume& ddgiVolume);
    void ComputePointCloudBaseColor(DDGIVolume& ddgiVolume);
    void ComputeProbePointIndices(DDGIVolume& ddgiVolume);

    void ResetProbeStates(DDGIVolume& ddgiVolume);
    void UpdateProbeStates(DDGIVolume& ddgiVolume);
    void UpdateDistanceTexture(DDGIVolume& ddgiVolume);
    void UpdateIrradianceTexture(DDGIVolume& ddgiVolume);
    void ComputePointCloudLighting(DDGIVolume& ddgiVolume);
    void ComputeProbeRelevance(DDGIVolume& ddgiVolume);
    void ComputeProbeDistance(DDGIVolume& ddgiVolume);
    void ComputeProbeDistanceBorder(DDGIVolume& ddgiVolume);
    void ComputeIrradianceDirtyPointCheck(DDGIVolume& ddgiVolume);
	void ComputeProbeIrradianceList(DDGIVolume& ddgiVolume);
    void ComputeProbeIrradianceDispatchArgs();
    void ComputeProbeIrradiance(DDGIVolume& ddgiVolume);
    void ComputeProbeIrradianceBorder(DDGIVolume& ddgiVolume);
    void ComputeIrradianceTexture(DDGIVolume& ddgiVolume);

    void ComputeProbePointIndices(DDGIVolume& ddgiVolume) {
        ProfilerOpenGLZoneFunctionLightGreen();

        const PointCloud& pointCloud = ddgiVolume.GetPointClound();
        const DDGIVolumeGPU ddgiVolumeGPU = ddgiVolume.GetGPUData();

        ClearSSBO("ProbeIndexCounter");

        UpdateSSBO("DDGIVolume", sizeof(DDGIVolumeGPU), &ddgiVolumeGPU);
        UpdateSSBO("PointCloudGridOffsets", pointCloud.GetGridCellOffsets().size() * sizeof(uint32_t), pointCloud.GetGridCellOffsets().data());
        UpdateSSBO("PointCloudGridCounts", pointCloud.GetGridCellCounts().size() * sizeof(uint32_t), pointCloud.GetGridCellCounts().data());

        //ReserveSSBO("PointCloudGridDirtyFlags", pointCloud.GetGridCellCounts().size() * sizeof(uint32_t));
        ReserveSSBO("PointCloudDirtyFlags", pointCloud.GetPointCount() * sizeof(uint32_t));
        ReserveSSBO("ProbePointIndices", sizeof(uint32_t) * ddgiVolume.GetProbePointIndexPoolSize());
        ReserveSSBO("ProbePointOffsets", sizeof(uint32_t) * ddgiVolume.GetTotalProbeCount());
        ReserveSSBO("ProbePointCounts", sizeof(uint32_t) * ddgiVolume.GetTotalProbeCount());

        BindSSBO(0, "DDGIVolume");
        BindSSBO(1, "PointCloudGridOffsets");
        BindSSBO(2, "PointCloudGridCounts");
        BindSSBO(3, "ProbePointIndices");
        BindSSBO(4, "ProbePointOffsets");
        BindSSBO(5, "ProbePointCounts");
        BindSSBO(6, "ProbeIndexCounter");
        BindSSBO(7, g_pointCloudVbo); // VBO bound as SSBO

        BindShader("ProbePointIndices");
        SetUniformVec3("u_gridMin", ddgiVolume.GetBoundsMin());
        SetUniformIVec3("u_gridDimensions", pointCloud.GetGridDimensions());
        SetUniformFloat("u_gridCellSize", pointCloud.GetGridCellSize());
        SetUniformInt("u_totalProbes", ddgiVolume.GetTotalProbeCount());

        glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);
        glDispatchCompute(ddgiVolume.GetTotalProbeCount(), 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
    }

    OpenGLTextureArray& GetProbeDistanceTextureArray();
    OpenGLTextureArray& GetProbeIrradianceTextureArray();

    //void RenderSceneBvhTris(DDGIVolume& ddgiVolume);

    void UpdateGlobalIllumintation() {
        if (World::GetDDGIVolumes().empty()) return;

        uint64_t id = 0;
        for (DDGIVolume& volume : World::GetDDGIVolumes()) {
            id = volume.GetId();
            break;
        }

        DDGIVolume& ddgiVolume = *World::GetDDGIVolumeByObjectId(id);
        const PointCloud& pointCloud = ddgiVolume.GetPointClound();

        if (ddgiVolume.PointCloudNeedsGPUUpload()) {
            UploadPointCloud(ddgiVolume);
            ComputeProbePointIndices(ddgiVolume);
            ComputePointCloudBaseColor(ddgiVolume);
            ResetProbeStates(ddgiVolume);

            ddgiVolume.MarkPointCloudAsUploaded();

            ClearSSBO("ProbeSHColor"); // You never used to have to do this. Find out why!
        }

        ddgiVolume.UpdateSceneBvh();

        uint64_t sceneBvhId = ddgiVolume.GetSceneBvhId();
        SceneBvh* sceneBvh = Hell::Bvh::GetSceneBvhById(sceneBvhId);
        if (!sceneBvh) return;

        const std::vector<BvhNode>& sceneNodes = sceneBvh->m_nodes;
        const std::vector<BvhNode>& meshBvhNodes = sceneBvh->m_meshNodes;
        const std::vector<GpuPrimitiveInstance>& entityInstances = sceneBvh->m_gpuInstances;
        const std::vector<BVHTriangle>& triangles = sceneBvh->m_triangles;

        const DDGIVolumeGPU ddgiVolumeGPU = ddgiVolume.GetGPUData();
        const std::vector<GPUAABB>& dirtyDoorABBBs = World::GetDirtyDoorAABBS();

        // BVH data
        UpdateSSBO("SceneBvh", sceneNodes.size() * sizeof(BvhNode), sceneNodes.data());
        UpdateSSBO("MeshesBvh", meshBvhNodes.size() * sizeof(BvhNode), meshBvhNodes.data());
        UpdateSSBO("EntityInstances", entityInstances.size() * sizeof(GpuPrimitiveInstance), entityInstances.data());
        UpdateSSBO("TriangleData", triangles.size() * sizeof(BVHTriangle), triangles.data());

        // Probe/Pointcloud data
        UpdateSSBO("DDGIVolume", sizeof(DDGIVolumeGPU), &ddgiVolumeGPU);
        UpdateSSBO("DirtyDoorAABBs", dirtyDoorABBBs.size() * sizeof(GPUAABB), dirtyDoorABBBs.data());

        // Reserve space for GPU updated SSBOS
        ReserveSSBO("ProbeSHColor", sizeof(ProbeColor) * ddgiVolume.GetTotalProbeCount());
        ReserveSSBO("ProbeDistanceIndices", sizeof(uint32_t) * ddgiVolume.GetTotalProbeCount());
        ReserveSSBO("ProbeIrradianceIndices", sizeof(uint32_t) * ddgiVolume.GetTotalProbeCount());
        ReserveSSBO("ProbeStates", sizeof(ProbeState) * ddgiVolume.GetTotalProbeCount());

        // Raytracing SSBOs stay persistently bound for whole GI pass
        BindSSBO(0, "EntityInstances");
        BindSSBO(1, "TriangleData");
        BindSSBO(2, "SceneBvh");
        BindSSBO(3, "MeshesBvh");

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

    void ResetProbeStates(DDGIVolume& ddgiVolume) {
        std::vector<ProbeState> probeStates;
        probeStates.reserve(ddgiVolume.GetTotalProbeCount());

        for (uint32_t i = 0; i < ddgiVolume.GetTotalProbeCount(); i++) {
            ProbeState& probeState = probeStates.emplace_back();
            probeState.isActive = true;
            probeState.isRelevant = true;
            probeState.distanceCooldown = PROBE_MAX_DISTANCE_COOLDOWN;
            probeState.irradianceCooldown = PROBE_MAX_IRRADIANCE_COOLDOWN;
            probeState.relocationOffset = glm::vec3(0.0f);
        }

        UpdateSSBO("ProbeStates", probeStates.size() * sizeof(ProbeState), probeStates.data());

        g_time = 0.0f; // Hack to prevent colorful probe glitch at start
    }

    void UpdateProbeStates(DDGIVolume& ddgiVolume) {
        ProfilerOpenGLZoneFunctionLightGreen();

        g_time += GameOLD::GetDeltaTime(); // Hack to prevent colorful probe glitch at start

        BindSSBO(4, "DDGIVolume");
        BindSSBO(5, "ProbeStates");
        BindSSBO(6, "DirtyDoorAABBs");

        BindShader("ProbeStateUpdate");

        SetUniformInt("u_dirtyDoorAABBCount", (int)World::GetDirtyDoorAABBS().size());
        SetUniformFloat("u_time", g_time); // Hack to prevent colorful probe glitch at start

        DispatchCompute((ddgiVolume.GetTotalProbeCount() + 63) / 64, 1, 1);
    }

    void ComputePointCloudLighting(DDGIVolume& ddgiVolume) {
        ProfilerOpenGLZoneFunctionLightGreen();

        OpenGLShader* shader = GetShaderOLD("PointCloudLighting");
        shader->Bind();
        shader->SetInt("u_lightCount", World::GetLightCount());

        BindSSBO(4, "Lights");
        BindSSBO(5, "LightAABBs");
        BindSSBO(6, g_pointCloudVbo);
        BindSSBO(7, "Samplers");
        BindSSBO(8, "PointCloudDirtyFlags");

        glDispatchCompute((ddgiVolume.GetPointCloundPoints().size() + 127) / 128, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

	void ComputeProbeRelevance(DDGIVolume& ddgiVolume) {
		ProfilerOpenGLZoneFunctionLightGreen();

		ClearSSBO("ProbeIrradianceCounter");

		BindSSBO(4, "ProbeStates");
		BindSSBO(5, "DDGIVolume");
		BindSSBO(6, "RendererData");
		BindSSBO(7, "ViewportData");

		BindShader("ProbeRelevance");
		SetUniformVec3("u_viewPos", RenderDataManager::GetViewportData()[0].viewPos);
		SetUniformBool("u_msaaRenderer", false); // TODO: remove me from shader
		SetUniformBool("u_octNormals", Renderer::GetRendererMode() == RendererMode::RE_STYLE);

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

		if (Renderer::GetRendererMode() == RendererMode::RE_STYLE) {
			OpenGLFrameBuffer& gBuffer = GetFrameBuffer("GBufferRE");
			int32_t quarterWidth = (gBuffer.GetWidth() + 3) / 4;
			int32_t quarterHeight = (gBuffer.GetHeight() + 3) / 4;

			BindTextureUnit(2, gBuffer.GetColorAttachmentHandleByName("NormalXYRoughnessMisc"));
			BindTextureUnit(3, gBuffer.GetDepthAttachmentHandle());
			glDispatchCompute((quarterWidth + 7) / 8, (quarterHeight + 7) / 8, 1);
		}
		else {
			OpenGLFrameBuffer& gBuffer = GetFrameBuffer("GBuffer");
			int32_t quarterWidth = (gBuffer.GetWidth() + 3) / 4;
			int32_t quarterHeight = (gBuffer.GetHeight() + 3) / 4;

			BindTextureUnit(2, gBuffer.GetColorAttachmentHandleByName("NormalXYRoughnessMisc"));
			BindTextureUnit(3, gBuffer.GetDepthAttachmentHandle());
			glDispatchCompute((quarterWidth + 7) / 8, (quarterHeight + 7) / 8, 1);
		}
	}

    void ComputeProbeDistance(DDGIVolume& ddgiVolume) {
        ProfilerOpenGLZoneFunctionLightGreen();

        OpenGLShader* distanceShader = GetShaderOLD("ProbeDistance");
        OpenGLShader* listShader = GetShaderOLD("ProbeDistanceList");
        OpenGLShader* argsShader = GetShaderOLD("ProbeDistanceDispatchArgs");

        if (!distanceShader || !listShader || !argsShader) return;

        OpenGLTextureArray& probeDistanceTexture = GetProbeDistanceTextureArray();
        BindImageTextureArray(0, probeDistanceTexture.GetHandle(), GL_READ_WRITE, GL_RG16F);

        static int frameIndex = 0;
        frameIndex++;

        ClearSSBO("ProbeDistanceCounter");

        BindSSBO(4, "DDGIVolume");
        BindSSBO(5, "ProbeStates");
        BindSSBO(6, "ProbeDistanceCounter");
        BindSSBO(7, "ProbeDistanceIndices");
        BindSSBO(8, "ProbeDistanceDispatchArgs");

        BindShader("ProbeDistanceList");
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        glDispatchCompute((ddgiVolume.GetTotalProbeCount() + 63) / 64, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        BindShader("ProbeDistanceDispatchArgs");
        glDispatchCompute(1, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);

        distanceShader->Bind();
        distanceShader->SetInt("u_frameIndex", frameIndex);
        BindDispatchBuffer("ProbeDistanceDispatchArgs");
        glDispatchComputeIndirect(0);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    void ComputeProbeDistanceBorder(DDGIVolume& ddgiVolume) {
        ProfilerOpenGLZoneFunctionLightGreen();

        OpenGLShader* shader = GetShaderOLD("ProbeDistanceBorder");
        if (!shader) return;

        BindShader("ProbeDistanceBorder");
        BindSSBO(4, "DDGIVolume");

        OpenGLTextureArray& probeDistanceTexture = GetProbeDistanceTextureArray();
        BindImageTextureArray(0, probeDistanceTexture.GetHandle(), GL_READ_WRITE, GL_RG16F);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        glDispatchCompute((ddgiVolume.GetTotalProbeCount() + 63) / 64, 1, 1);
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    void ComputeIrradianceDirtyPointCheck(DDGIVolume& ddgiVolume) {
        ProfilerOpenGLZoneFunctionLightGreen();

        BindShader("ProbeIrradianceDirtyPointCheck");

        BindSSBO(4, "ProbeStates");
        BindSSBO(5, "ProbePointIndices");
        BindSSBO(6, "ProbePointOffsets");
        BindSSBO(7, "ProbePointCounts");
        BindSSBO(8, "DDGIVolume");
        BindSSBO(9, g_pointCloudVbo); // VBO bound as SSBO
        BindSSBO(10, "PointCloudDirtyFlags");

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        //glDispatchCompute((ddgiVolume.GetTotalProbeCount() + 63) / 64, 1, 1);
        glDispatchCompute((ddgiVolume.GetTotalProbeCount() + 31) / 32, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

	void ComputeProbeIrradianceList(DDGIVolume& ddgiVolume) {
		ProfilerOpenGLZoneFunctionLightGreen();

		BindSSBO(4, "ProbeStates");
		BindSSBO(5, "ProbeIrradianceCounter");
		BindSSBO(6, "ProbeIrradianceIndices");
		BindSSBO(7, "DDGIVolume");

		BindShader("ProbeIrradianceList");

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		glDispatchCompute((ddgiVolume.GetTotalProbeCount() + 63) / 64, 1, 1);
    }

    void ComputeProbeIrradianceDispatchArgs() {
		ProfilerOpenGLZoneFunctionLightGreen();

		BindSSBO(4, "ProbeIrradianceCounter");
		BindSSBO(5, "ProbeIrradianceDispatchArgs");

        BindShader("ProbeLightingDispatchArgs");

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		glDispatchCompute(1, 1, 1);
    }

    void ComputeProbeIrradiance(DDGIVolume& ddgiVolume) {
        ProfilerOpenGLZoneFunctionLightGreen();

        OpenGLShader* shader = GetShaderOLD("ProbeIrradiance");
        if (!shader) return;

        static int frameIndex = 0;
        frameIndex++;

        BindSSBO(4, g_pointCloudVbo);
        BindSSBO(5, "ProbeSHColor");
        BindSSBO(6, "ProbeIrradianceCounter");
        BindSSBO(7, "ProbeIrradianceIndices");
        BindSSBO(8, "DDGIVolume");
        BindSSBO(9, "ProbeStates");
        BindSSBO(10, "ProbePointIndices");
        BindSSBO(11, "ProbePointOffsets");
        BindSSBO(12, "ProbePointCounts");

        shader->Bind();
        shader->SetFloat("u_pointCloudSpacing", ddgiVolume.GetPointCloudSpacing());
        shader->SetInt("u_pointCount", (int32_t)ddgiVolume.GetPointCloudCount());
        shader->SetInt("u_frameIndex", frameIndex);
        shader->SetBool("u_useSH", Renderer::GetCurrentRendererSettings().irradianceUsesSH);

        // These can go soon:
        const PointCloud& pointCloud = ddgiVolume.GetPointClound();
        shader->SetVec3("u_gridMin", ddgiVolume.GetBoundsMin());
        shader->SetIVec3("u_gridDimensions", pointCloud.GetGridDimensions());
        shader->SetFloat("u_gridCellSize", pointCloud.GetGridCellSize());

        OpenGLTextureArray& probeIrradianceTexture = GetProbeIrradianceTextureArray();
        BindImageTextureArray(0, probeIrradianceTexture.GetHandle(), GL_READ_WRITE, GL_RGBA16F);

        BindDispatchBuffer("ProbeIrradianceDispatchArgs");

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);
        glDispatchComputeIndirect(0);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    void ComputeProbeIrradianceBorder(DDGIVolume& ddgiVolume) {
        ProfilerOpenGLZoneFunctionLightGreen();

        OpenGLShader* shader = GetShaderOLD("ProbeIrradianceBorder");
        if (!shader) return;

        BindSSBO(4, "DDGIVolume");
        shader->Bind();

        OpenGLTextureArray& irradianceTexture = GetProbeIrradianceTextureArray();
        BindImageTextureArray(0, irradianceTexture.GetHandle(), GL_READ_WRITE, GL_RGBA16F);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        glDispatchCompute((ddgiVolume.GetTotalProbeCount() + 63) / 64, 1, 1);
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

	void ComputePointCloudBaseColor(DDGIVolume& ddgiVolume) {
		// TODO:
		// If RenderDoc was detected then you gotta do this differently
		if (Hell::BackEnd::RenderDocFound()) {
			Logging::Fatal() << "GlobalIllumination::CalculatePointCloudBaseColor() does not work without BIndless yet you fool.\n";
            return;
		}

        OpenGLShader* shader = GetShaderOLD("PointCloudBaseColor");
        if (!shader) return;

        const std::vector<CloudPointTextureInfo>& pointCloundTextureInfo = ddgiVolume.GetPointCloudTextureInfo();

        UpdateSSBO("PointCloudTextureInfo", pointCloundTextureInfo.size() * sizeof(CloudPointTextureInfo), pointCloundTextureInfo.data());

        // Ensure bindless texture IDs are in the Samplers ID, which is not the case if this runs the first frame of the renderer
        UpdateSSBO("Samplers", sizeof(GLuint64) * OpenGLBackEnd::GetBindlessTextureIDs().size(), OpenGLBackEnd::GetBindlessTextureIDs().data());

		BindSSBO(0, "Samplers");
		BindSSBO(1, "PointCloudTextureInfo");
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, g_pointCloudVbo);

		GLuint numGroupsX = (ddgiVolume.GetPointCloudCount() + 127) / 128;

		shader->Bind();
        shader->DispatchCompute(numGroupsX, 1, 1);

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
    }

    void UploadPointCloud(DDGIVolume& ddgiVolume) {
        if (g_pointCloudVao == 0) {
            glGenVertexArrays(1, &g_pointCloudVao);
            glGenBuffers(1, &g_pointCloudVbo);
        }

        const std::vector<CloudPoint>& pointCloud = ddgiVolume.GetPointCloundPoints();

        // Point cloud
        glBindBuffer(GL_ARRAY_BUFFER, g_pointCloudVbo);
        glBindVertexArray(g_pointCloudVao);
        glBufferData(GL_ARRAY_BUFFER, sizeof(CloudPoint) * pointCloud.size(), pointCloud.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(CloudPoint), (void*)offsetof(CloudPoint, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(CloudPoint), (void*)offsetof(CloudPoint, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(CloudPoint), (void*)offsetof(CloudPoint, directLighting));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(CloudPoint), (void*)offsetof(CloudPoint, baseColor));

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        Logging::Debug() << "Uploaded point cloud to GPU (" << pointCloud.size() << " points)\n";
    }

    void DrawPointCloud(DDGIVolume& ddgiVolume) {
        if (g_pointCloudVao == 0) return;

        OpenGLShader* shader = GetShaderOLD("DebugPointCloud");
        OpenGLFrameBuffer* gBuffer = GetFrameBufferOLD("GBuffer");

        if (!gBuffer) return;
        if (!shader) return;

        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();

        const PointCloud& pointCloud = ddgiVolume.GetPointClound();

        shader->Bind();
        shader->SetIVec3("u_pointCloudGridDimensions", pointCloud.GetGridDimensions());
        shader->SetFloat("u_pointCloudCellSize", pointCloud.GetGridCellSize());
        shader->SetVec3("u_volumeMinBounds", ddgiVolume.GetBoundsMin());

        BindSSBO(4, "Lights");
        BindSSBO(5, "PointCloudGridOffsets");
        BindSSBO(6, "PointCloudGridCounts");
        BindSSBO(7, "PointCloudDirtyFlags");

		OpenGLRasterizerState state;
		state.depthTestEnabled = true;
		state.cullfaceEnable = true;
		state.blendEnable = false;
		state.depthMask = true;
		state.depthFunc = GL_GREATER;
		ForceRasterizerState(state);

		OpenGLFrameBuffer* fbo = nullptr;

		if (Renderer::GetRendererMode() == RendererMode::RE_STYLE) {
			fbo = GetFrameBufferOLD("GBufferRE");
			if (!fbo) return;

			fbo->Bind();
			fbo->DrawBuffer("Lighting");
        }
		else {
			fbo = GetFrameBufferOLD("GBuffer");
			if (!fbo) return;

			fbo->Bind();
			fbo->DrawBuffer("Lighting");

			state.depthFunc = GL_LESS;
			ForceRasterizerState(state);
		}

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGLRenderer::SetViewport(fbo, viewport);
            shader->SetInt("u_viewportIndex", i);
            shader->SetMat4("u_projectionView", viewportData[i].projectionViewReverseZ);

            glBindVertexArray(g_pointCloudVao);
            glDrawArrays(GL_POINTS, 0, ddgiVolume.GetPointCloudCount());
            glBindVertexArray(0);
        }
    }

    void DrawPointCloudGrid(DDGIVolume& ddgiVolume) {
        ddgiVolume.GetPointClound().DebugDrawGrid();
    }

    void DrawProbes(DDGIVolume& ddgiVolume) {
        OpenGLShader* shader = GetShaderOLD("DebugProbes");

        if (!shader) return;

		shader->Bind();
		shader->SetInt("u_probeDebugState", static_cast<int>(Renderer::GetCurrentRendererSettings().probeDebugState));
        shader->SetBool("u_useSH", Renderer::GetCurrentRendererSettings().irradianceUsesSH);

        OpenGLTextureArray& probeDistanceTexture = GetProbeDistanceTextureArray();
        BindTextureUnit(0, probeDistanceTexture.GetHandle());

        OpenGLTextureArray& probeIrradianceTexture = GetProbeIrradianceTextureArray();
        BindTextureUnit(1, probeIrradianceTexture.GetHandle());

		OpenGLRasterizerState state;
		state.depthTestEnabled = true;
		state.cullfaceEnable = true;
		state.blendEnable = false;
		state.depthMask = true;
		state.depthFunc = GL_GREATER;
		ForceRasterizerState(state);

        OpenGLFrameBuffer* fbo = nullptr;


		if (Renderer::GetRendererMode() == RendererMode::RE_STYLE) {
			fbo = GetFrameBufferOLD("GBufferRE");
			if (!fbo) return;

			fbo->Bind();
			fbo->DrawBuffer("Lighting");
        }
        else {
            fbo = GetFrameBufferOLD("GBuffer");
			if (!fbo) return;

            fbo->Bind();
            fbo->DrawBuffer("Lighting");
        }

        BindSSBO(6, "ProbeSHColor");
        BindSSBO(7, "DDGIVolume");
        BindSSBO(8, "ProbeStates");

        glBindVertexArray(Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());

        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();
        Model* sphereModel = Hell::ResourceManager::GetModelByName("Sphere");
        if (!sphereModel || sphereModel->GetMeshIndices().empty()) return;
        if (sphereModel->GetMeshCount() == 0) return;

        uint32_t meshId = sphereModel->GetMeshIndices()[0];
        Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshId);
        if (!mesh) return;

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

			OpenGLRenderer::SetViewport(fbo, viewport);
			shader->SetInt("u_viewportIndex", i);
            shader->SetMat4("u_projectionView", viewportData[i].projectionViewReverseZ);

			glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), ddgiVolume.GetTotalProbeCount(), mesh->baseVertex);
        }
    }

    void RaytracedSceneDebug() {
        ProfilerOpenGLZoneFunctionLightGreen();

        OpenGLFrameBuffer* fbo = GetFrameBufferOLD("IndirectDiffuse");
        OpenGLShader* shader = GetShaderOLD("RaytraceScene");

        if (!fbo) return;
        if (!shader) return;

        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();

        shader->Bind();
        shader->SetMat4("u_projectionMatrix", viewportData[0].projection);
        shader->SetMat4("u_viewMatrix", viewportData[0].view);

        BindSSBO(0, "EntityInstances");
        BindSSBO(1, "TriangleData");
        BindSSBO(2, "SceneBvh");
        BindSSBO(3, "MeshesBvh");
        BindSSBO(4, "Lights");

        glBindImageTexture(0, fbo->GetColorAttachmentHandleByName("Color"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
        glDispatchCompute(fbo->GetWidth() / 8, fbo->GetHeight() / 8, 1);
    }

    void ComputeIrradianceTexture(DDGIVolume& ddgiVolume) {
        ProfilerOpenGLZoneFunctionLightGreen();

        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();

        OpenGLFrameBuffer* fbo = GetFrameBufferOLD("IndirectDiffuse");
        OpenGLShader* shader = GetShaderOLD("ProbeIrradianceTexture");

        if (!fbo) return;
        if (!shader) return;

        shader->Bind();
        shader->SetMat4("u_projectionMatrix", viewportData[0].projection);
        shader->SetVec3("u_cameraPos", viewportData[0].viewPos);
        shader->SetMat4("u_viewMatrix", viewportData[0].view);
        shader->SetBool("u_useSH", Renderer::GetCurrentRendererSettings().irradianceUsesSH);

        BindSSBO(0, "EntityInstances");
        BindSSBO(1, "TriangleData");
        BindSSBO(2, "SceneBvh");
        BindSSBO(3, "MeshesBvh");
        BindSSBO(4, "Lights");
        BindSSBO(5, "ProbeSHColor");
        BindSSBO(6, "ProbeStates");
        BindSSBO(7, "DDGIVolume");
        BindSSBO(8, "RendererData");
        BindSSBO(9, "ViewportData");

        BindImageTexture(0, fbo->GetColorAttachmentHandleByName("Color"), GL_WRITE_ONLY, GL_R11F_G11F_B10F);

        if (Renderer::GetRendererMode() == RendererMode::RE_STYLE) {
            OpenGLFrameBuffer* gBuffer = GetFrameBufferOLD("GBufferRE");
            if (!gBuffer) return;

            BindTextureUnit(2, gBuffer->GetColorAttachmentHandleByName("NormalXYRoughnessMisc"));
            BindTextureUnit(3, gBuffer->GetDepthAttachmentHandle());

            shader->SetBool("u_octalNormals", true);
        }
        else {
            OpenGLFrameBuffer* gBuffer = GetFrameBufferOLD("GBuffer");
            if (!gBuffer) return;

            BindTextureUnit(2, gBuffer->GetColorAttachmentHandleByName("NormalXYRoughnessMisc"));
            BindTextureUnit(3, gBuffer->GetDepthAttachmentHandle());

            shader->SetBool("u_octalNormals", false);
        }

        OpenGLTextureArray& probeDistanceTexture = GetProbeDistanceTextureArray();
        BindTextureUnit(4, probeDistanceTexture.GetHandle());

        OpenGLTextureArray& probeIrradianceTexture = GetProbeIrradianceTextureArray();
        BindTextureUnit(5, probeIrradianceTexture.GetHandle());

        glDispatchCompute(fbo->GetWidth() / TILE_SIZE, fbo->GetHeight() / TILE_SIZE, 1);
    }

    void UpdateDistanceTexture(DDGIVolume& ddgiVolume) {
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

    void UpdateIrradianceTexture(DDGIVolume& ddgiVolume) {
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

    void DrawGPUBvhSceneNodes(DDGIVolume& volume, const glm::vec4& color) {
        const std::vector<BvhNode>& sceneNodes = volume.GetSceneNodes();

        for (const BvhNode& node : sceneNodes) {
            AABB aabb(node.boundsMin, node.boundsMax);
            DebugDraw::DrawAABB(aabb, color);
        }
    }

    void DrawGPUBvhSceneLeafNodes(DDGIVolume& volume, const glm::vec4& color) {
        const std::vector<BvhNode>& sceneNodes = volume.GetSceneNodes();

        for (const BvhNode& node : sceneNodes) {
            if (node.primitiveCount > 0) {
                AABB aabb(node.boundsMin, node.boundsMax);
                DebugDraw::DrawAABB(aabb, color);
            }
        }
    }

    void DrawRaytracingBvh(DDGIVolume& volume) {
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
