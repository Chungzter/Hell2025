#include "API/OpenGL/Renderer/GL_renderer.h"
#include "API/OpenGL/GL_backend.h"
#include "AssetManagement/AssetManager.h"
#include "Bvh/Gpu/Bvh.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderDataManager.h"
#include "GlobalIllumination/GlobalIllumination.h"
#include "Viewport/ViewportManager.h"
#include "World/World.h"
#include "Util/Util.h"

#include <Hell/Constants.h>
#include <Hell/Logging.h>

namespace OpenGLRenderer {

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

        ReserveSSBO("PointCloudGridDirtyFlags", pointCloud.GetGridCellCounts().size() * sizeof(uint32_t));
        ReserveSSBO("ProbePointIndices", sizeof(uint32_t) * ddgiVolume.GetProbePointIndexPoolSize());
        ReserveSSBO("ProbePointOffsets", sizeof(uint32_t) * ddgiVolume.GetTotalProbeCount());
        ReserveSSBO("ProbePointCounts", sizeof(uint32_t) * ddgiVolume.GetTotalProbeCount());

        BindSSBO("DDGIVolume", 0);
        BindSSBO("PointCloudGridOffsets", 1);
        BindSSBO("PointCloudGridCounts", 2);
        BindSSBO("ProbePointIndices", 3);
        BindSSBO("ProbePointOffsets", 4);
        BindSSBO("ProbePointCounts", 5);
        BindSSBO("ProbeIndexCounter", 6);
        BindSSBO(g_pointCloudVbo, 7); // VBO bound as SSBO

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
        }

        ddgiVolume.UpdateSceneBvh();

        uint64_t sceneBvhId = ddgiVolume.GetSceneBvhId();
        const std::vector<BvhNode>& sceneNodes = ddgiVolume.GetSceneNodes();
        const std::vector<BvhNode>& meshBvhNodes = Bvh::Gpu::GetMeshGpuBvhNodes();
        const std::vector<GpuPrimitiveInstance>& entityInstances = Bvh::Gpu::GetGpuEntityInstances(sceneBvhId);
        const std::vector<float>& triData = Bvh::Gpu::GetTriangleData();

        const DDGIVolumeGPU ddgiVolumeGPU = ddgiVolume.GetGPUData();
        const std::vector<GPUAABB>& dirtyDoorABBBs = World::GetDirtyDoorAABBS();

        // Bvh::Gpu::RenderSceneBvh(sceneBvhId, GREEN);
        //Bvh::Gpu::RenderSceneTris(sceneBvhId, GREEN);

        // BVH data
        UpdateSSBO("SceneBvh", sceneNodes.size() * sizeof(BvhNode), sceneNodes.data());
        UpdateSSBO("MeshesBvh", meshBvhNodes.size() * sizeof(BvhNode), meshBvhNodes.data());
        UpdateSSBO("EntityInstances", entityInstances.size() * sizeof(GpuPrimitiveInstance), entityInstances.data());
        UpdateSSBO("TriangleData", triData.size() * sizeof(float), triData.data());

        // Probe/Pointcloud data
        UpdateSSBO("DDGIVolume", sizeof(DDGIVolumeGPU), &ddgiVolumeGPU);
        UpdateSSBO("DirtyDoorAABBs", dirtyDoorABBBs.size() * sizeof(GPUAABB), dirtyDoorABBBs.data());

        // Reserve space for GPU updated SSBOS
        ReserveSSBO("ProbeSHColor", sizeof(ProbeColor) * ddgiVolume.GetTotalProbeCount());
        ReserveSSBO("ProbeDistanceIndices", sizeof(uint32_t) * ddgiVolume.GetTotalProbeCount());
        ReserveSSBO("ProbeIrradianceIndices", sizeof(uint32_t) * ddgiVolume.GetTotalProbeCount());
        ReserveSSBO("ProbeStates", sizeof(ProbeState) * ddgiVolume.GetTotalProbeCount());

        // Raytracing SSBOs stay persistently bound for whole GI pass
        BindSSBO("EntityInstances", 0);
        BindSSBO("TriangleData", 1);
        BindSSBO("SceneBvh", 2);
        BindSSBO("MeshesBvh", 3);

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
    }

    void UpdateProbeStates(DDGIVolume& ddgiVolume) {
        ProfilerOpenGLZoneFunctionLightGreen();

        BindSSBO("DDGIVolume", 4);
        BindSSBO("ProbeStates", 5);
        BindSSBO("DirtyDoorAABBs", 6);

        BindShader("ProbeStateUpdate");

        OpenGLShader* shader = GetShaderOLD("ProbeStateUpdate");
        shader->SetInt("u_dirtyDoorAABBCount", (int)World::GetDirtyDoorAABBS().size());

        DispatchCompute((ddgiVolume.GetTotalProbeCount() + 63) / 64, 1, 1);
    }

    void ComputePointCloudLighting(DDGIVolume& ddgiVolume) {
        ProfilerOpenGLZoneFunctionLightGreen();

        OpenGLShader* shader = GetShaderOLD("PointCloudLighting");
        shader->Bind();
        shader->SetInt("u_lightCount", World::GetLightCount());

        BindSSBO("Lights", 4);
        BindSSBO("LightAABBs", 5);
        BindSSBO(g_pointCloudVbo, 6);
        BindSSBO("Samplers", 7);

        glDispatchCompute((ddgiVolume.GetPointCloundPoints().size() + 127) / 128, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

	void ComputeProbeRelevance(DDGIVolume& ddgiVolume) {
		ProfilerOpenGLZoneFunctionLightGreen();


		ClearSSBO("ProbeIrradianceCounter");

		BindSSBO("ProbeStates", 4);
		BindSSBO("DDGIVolume", 5);
		BindSSBO("RendererData", 6);
		BindSSBO("ViewportData", 7);

		BindShader("ProbeRelevance");
		SetUniformVec3("u_viewPos", RenderDataManager::GetViewportData()[0].viewPos);
		SetUniformBool("u_msaaRenderer", Renderer::GetRendererMode() == RendererMode::MSAA);
		SetUniformBool("u_octNormals", Renderer::GetRendererMode() == RendererMode::RE_STYLE);

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

		if (Renderer::GetRendererMode() == RendererMode::MSAA) {
			OpenGLFrameBuffer& msaaRenderer = GetFrameBuffer("MSAA");
			int32_t quarterWidth = (msaaRenderer.GetWidth() + 3) / 4;
			int32_t quarterHeight = (msaaRenderer.GetHeight() + 3) / 4;

            BindTextureUnit(0, msaaRenderer.GetDepthAttachmentHandle());
			BindTextureUnit(1, msaaRenderer.GetColorAttachmentHandleByName("Normal"));
			glDispatchCompute((quarterWidth + 7) / 8, (quarterHeight + 7) / 8, 1);
		}
		else if (Renderer::GetRendererMode() == RendererMode::RE_STYLE) {
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

			BindTextureUnit(2, gBuffer.GetColorAttachmentHandleByName("Normal"));
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

        BindSSBO("DDGIVolume", 4);
        BindSSBO("ProbeStates", 5);
        BindSSBO("ProbeDistanceCounter", 6);
        BindSSBO("ProbeDistanceIndices", 7);
        BindSSBO("ProbeDistanceDispatchArgs", 8);

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
        BindSSBO("DDGIVolume", 4);

        OpenGLTextureArray& probeDistanceTexture = GetProbeDistanceTextureArray();
        BindImageTextureArray(0, probeDistanceTexture.GetHandle(), GL_READ_WRITE, GL_RG16F);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        glDispatchCompute((ddgiVolume.GetTotalProbeCount() + 63) / 64, 1, 1);
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    void ComputeIrradianceDirtyPointCheck(DDGIVolume& ddgiVolume) {
        ProfilerOpenGLZoneFunctionLightGreen();

        BindShader("ProbeIrradianceDirtyPointCheck");

        BindSSBO("ProbeStates", 4);
        BindSSBO("ProbePointIndices", 5);
        BindSSBO("ProbePointOffsets", 6);
        BindSSBO("ProbePointCounts", 7);
        BindSSBO("DDGIVolume", 8);
        BindSSBO(g_pointCloudVbo, 9); // VBO bound as SSBO

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        glDispatchCompute((ddgiVolume.GetTotalProbeCount() + 63) / 64, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

	void ComputeProbeIrradianceList(DDGIVolume& ddgiVolume) {
		ProfilerOpenGLZoneFunctionLightGreen();

		BindSSBO("ProbeStates", 4);
		BindSSBO("ProbeIrradianceCounter", 5);
		BindSSBO("ProbeIrradianceIndices", 6);
		BindSSBO("DDGIVolume", 7);

		BindShader("ProbeIrradianceList");

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		glDispatchCompute((ddgiVolume.GetTotalProbeCount() + 63) / 64, 1, 1);
    }

    void ComputeProbeIrradianceDispatchArgs() {
		ProfilerOpenGLZoneFunctionLightGreen();

		BindSSBO("ProbeIrradianceCounter", 4);
		BindSSBO("ProbeIrradianceDispatchArgs", 5);

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

        BindSSBO(g_pointCloudVbo, 4);
        BindSSBO("ProbeSHColor", 5);
        BindSSBO("ProbeIrradianceCounter", 6);
        BindSSBO("ProbeIrradianceIndices", 7);
        BindSSBO("DDGIVolume", 8);
        BindSSBO("ProbeStates", 9);
        BindSSBO("ProbePointIndices", 10);
        BindSSBO("ProbePointOffsets", 11);
        BindSSBO("ProbePointCounts", 12);

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

        BindSSBO("DDGIVolume", 4);
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
		if (BackEnd::RenderDocFound()) {
			Logging::Fatal() << "GlobalIllumination::CalculatePointCloudBaseColor() does not work without BIndless yet you fool.\n";
            return;
		}

        OpenGLShader* shader = GetShaderOLD("PointCloudBaseColor");
        if (!shader) return;

        const std::vector<CloudPointTextureInfo>& pointCloundTextureInfo = ddgiVolume.GetPointCloudTextureInfo();

        UpdateSSBO("PointCloudTextureInfo", pointCloundTextureInfo.size() * sizeof(CloudPointTextureInfo), pointCloundTextureInfo.data());

        // Ensure bindless texture IDs are in the Samplers ID, which is not the case if this runs the first frame of the renderer
        UpdateSSBO("Samplers", sizeof(GLuint64) * OpenGLBackEnd::GetBindlessTextureIDs().size(), OpenGLBackEnd::GetBindlessTextureIDs().data());

		BindSSBO("Samplers", 0);
		BindSSBO("PointCloudTextureInfo", 1);
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

        BindSSBO("Lights", 4);
        BindSSBO("PointCloudGridOffsets", 5);
        BindSSBO("PointCloudGridCounts", 6);
        BindSSBO("PointCloudGridDirtyFlags", 7);

		OpenGLRasterizerState state;
		state.depthTestEnabled = true;
		state.cullfaceEnable = true;
		state.blendEnable = false;
		state.depthMask = true;
		state.depthFunc = GL_GREATER;
		ForceRasterizerState(state);

		OpenGLFrameBuffer* fbo = nullptr;

		if (Renderer::GetRendererMode() == RendererMode::MSAA) {
			fbo = GetFrameBufferOLD("MSAA");
			if (!fbo) return;

			fbo->Bind();
			fbo->DrawBuffer("Lighting");
		}
		else if (Renderer::GetRendererMode() == RendererMode::RE_STYLE) {
			fbo = GetFrameBufferOLD("GBufferRE");
			if (!fbo) return;

			fbo->Bind();
			fbo->DrawBuffer("Lighting");
        }
		else {
			fbo = GetFrameBufferOLD("GBuffer");
			if (!fbo) return;

			fbo->Bind();
			fbo->DrawBuffer("FinalLighting");

			state.depthFunc = GL_LESS;
			ForceRasterizerState(state);
		}

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGLRenderer::SetViewport(fbo, viewport);
            shader->SetInt("u_viewportIndex", i);

			if (Renderer::GetRendererMode() == RendererMode::OLD_DEFERRED) {
				shader->SetMat4("u_projectionView", viewportData[i].projectionView);
			}
			else {
				shader->SetMat4("u_projectionView", viewportData[i].projectionViewReverseZ);
			}

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

        if (Renderer::GetRendererMode() == RendererMode::MSAA) {
            fbo = GetFrameBufferOLD("MSAA");
            if (!fbo) return;

            fbo->Bind();
            fbo->DrawBuffer("Lighting");

		}
		else if (Renderer::GetRendererMode() == RendererMode::RE_STYLE) {
			fbo = GetFrameBufferOLD("GBufferRE");
			if (!fbo) return;

			fbo->Bind();
			fbo->DrawBuffer("Lighting");
        }
        else {
            fbo = GetFrameBufferOLD("GBuffer");
			if (!fbo) return;

            fbo->Bind();
            fbo->DrawBuffer("FinalLighting");

			state.depthFunc = GL_LESS;
			ForceRasterizerState(state);
        }

        BindSSBO("ProbeSHColor", 6);
        BindSSBO("DDGIVolume", 7);
        BindSSBO("ProbeStates", 8);

        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());

        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();
        Mesh* mesh = AssetManager::GetMeshByModelNameMeshIndex("Sphere", 0);

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

			OpenGLRenderer::SetViewport(fbo, viewport);
			shader->SetInt("u_viewportIndex", i);

            if (Renderer::GetRendererMode() == RendererMode::OLD_DEFERRED) {
                shader->SetMat4("u_projectionView", viewportData[i].projectionView);
            }
            else {
				shader->SetMat4("u_projectionView", viewportData[i].projectionViewReverseZ);
            }

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

        BindSSBO("EntityInstances", 0);
        BindSSBO("TriangleData", 1);
        BindSSBO("SceneBvh", 2);
        BindSSBO("MeshesBvh", 3);
        BindSSBO("Lights", 4);

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

        BindSSBO("DDGIVolume", 7);

        shader->Bind();
        shader->SetMat4("u_projectionMatrix", viewportData[0].projection);
        shader->SetVec3("u_cameraPos", viewportData[0].viewPos);
        shader->SetMat4("u_viewMatrix", viewportData[0].view);
        shader->SetBool("u_useSH", Renderer::GetCurrentRendererSettings().irradianceUsesSH);
        shader->SetBool("u_msaaRenderer", (int)Renderer::GetRendererMode());

        BindSSBO("EntityInstances", 0);
        BindSSBO("TriangleData", 1);
        BindSSBO("SceneBvh", 2);
        BindSSBO("MeshesBvh", 3);
        BindSSBO("Lights", 4);
        BindSSBO("ProbeSHColor", 5);
        BindSSBO("ProbeStates", 6);
        BindSSBO("DDGIVolume", 7);
        BindSSBO("RendererData", 8);
        BindSSBO("ViewportData", 9);

        glBindImageTexture(0, fbo->GetColorAttachmentHandleByName("Color"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);

        if (Renderer::GetRendererMode() == RendererMode::MSAA) {
            OpenGLFrameBuffer* msaaRenderer = GetFrameBufferOLD("MSAA");
            if (!msaaRenderer) return;

            BindTextureUnit(6, msaaRenderer->GetColorAttachmentHandleByName("Normal"));
            BindTextureUnit(7, msaaRenderer->GetDepthAttachmentHandle());
        }
        else if (Renderer::GetRendererMode() == RendererMode::RE_STYLE) {
            // TODO
        }
        else {
            OpenGLFrameBuffer* gBuffer = GetFrameBufferOLD("GBuffer");
            if (!gBuffer) return;

            BindTextureUnit(1, gBuffer->GetColorAttachmentHandleByName("WorldPosition"));
            BindTextureUnit(2, gBuffer->GetColorAttachmentHandleByName("Normal"));
            BindTextureUnit(3, gBuffer->GetDepthAttachmentHandle());
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
            Renderer::DrawAABB(aabb, color);
        }
    }

    void DrawGPUBvhSceneLeafNodes(DDGIVolume& volume, const glm::vec4& color) {
        const std::vector<BvhNode>& sceneNodes = volume.GetSceneNodes();

        for (const BvhNode& node : sceneNodes) {
            if (node.primitiveCount > 0) {
                AABB aabb(node.boundsMin, node.boundsMax);
                Renderer::DrawAABB(aabb, color);
            }
        }
    }

    void DrawRaytracingBvh(DDGIVolume& volume) {
        const std::vector<BvhNode>& sceneNodes = volume.GetSceneNodes();
        const std::vector<BvhNode>& meshBvhNodes = Bvh::Gpu::GetMeshGpuBvhNodes();
        const std::vector<float>& triData = Bvh::Gpu::GetTriangleData();

        uint64_t sceneBvhId = volume.GetSceneBvhId();
        const std::vector<GpuPrimitiveInstance>& instances = Bvh::Gpu::GetGpuEntityInstances(sceneBvhId);

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
                                uint32_t floatIdx = meshNode.firstChildOrPrimitive + (j * 12);

                                glm::vec3 p0(triData[floatIdx], triData[floatIdx + 1], triData[floatIdx + 2]);
                                glm::vec3 e1(triData[floatIdx + 3], triData[floatIdx + 4], triData[floatIdx + 5]);
                                glm::vec3 e2(triData[floatIdx + 6], triData[floatIdx + 7], triData[floatIdx + 8]);

                                glm::vec3 p1 = p0 - e1;
                                glm::vec3 p2 = p0 + e2;

                                glm::vec3 worldP0 = instance.worldTransform * glm::vec4(p0, 1.0f);
                                glm::vec3 worldP1 = instance.worldTransform * glm::vec4(p1, 1.0f);
                                glm::vec3 worldP2 = instance.worldTransform * glm::vec4(p2, 1.0f);

                                Renderer::DrawLine(worldP0, worldP1, WHITE);
                                Renderer::DrawLine(worldP1, worldP2, WHITE);
                                Renderer::DrawLine(worldP2, worldP0, WHITE);
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