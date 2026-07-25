#include "RenderDataManager.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Common/Bit.h"
#include "Hell/Logging.h"
#include "Hell/Math/Math.h"
#include "Hell/Math/Transform.h"
#include "Hell/Profiling/CPUProfiler.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/UI/UIBackEnd.h"

#include "Unloved/Camera/Frustum.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Config/Config.h"
#include "Unloved/Config/FlashlightConfig.h"
#include "Unloved/Editor/Editor.h"
#include "Unloved/Objects/Renderables/AnimatedGameObject.h"
#include "Unloved/Systems/Mirrors/MirrorManager.h"
#include "Unloved/Systems/BloodOLD/BloodSystemOLD.h"
#include "Unloved/Systems/ShadowMaps/ShadowMapManager.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/World/World.h"

#include <span>

// Get me out of here
#include "World/LegacyWorld.h"
#include <vector>
#include "Hell/Input.h"
#include "Unloved/Render/RendererConstants.h"
#include "Timer.hpp"
#include "Unloved/Common/Constants.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Render/RendererUtil.h"
//
#include "../../../res/shaders/common/flags.glsl"

namespace Input = Hell::Input;

using namespace Hell;

namespace Unloved::RenderDataManager {
    using namespace Unloved;

    DrawCommandsSet g_drawCommandsSet;
    FlashLightShadowMapDrawInfo g_flashLightShadowMapDrawInfo;
    RendererData g_rendererData;
    std::vector<DrawIndexedIndirectCommand> g_drawCommandsUI;
    std::vector<GPULight> g_gpuLights;

	std::vector<RenderItem> g_renderItemsProcedural;
    std::vector<RenderItem> g_renderItems;
    std::vector<RenderItem> g_renderItemsBlended;
    std::vector<RenderItem> g_renderItemsAlphaDiscarded;
    std::vector<RenderItem> g_renderItemsHair;
    std::vector<RenderItem> g_renderItemsGlass;
	std::vector<RenderItem> g_renderItemsMirror;
    std::vector<RenderItem> g_renderItemsPlastic;
    std::vector<RenderItem> g_renderItemsStainedGlass;
    std::vector<RenderItem> g_renderItemsToiletWater;

    std::vector<RenderItem> g_renderItemsPointLightShadows;
    std::vector<RenderItem> g_renderItemsStaticPointLightShadows;
    std::vector<RenderItem> g_renderItemsDynamicPointLightShadows;
    std::vector<RenderItem> g_renderItemsMoonLightShadows;

    // Emissive
    std::vector<RenderItem> g_renderItemsEmissive;

    std::vector<RenderItem> g_shadowCasterRenderItems;

    std::vector<RenderItem> g_renderItemsOutline;
    std::vector<RenderItem> g_shadowMapRenderItems;

    std::vector<RenderItem> g_instanceData;
    std::vector<RenderItem> g_glassInstanceData;
    std::vector<GlassLightRange> g_glassLightRanges;
    std::vector<uint32_t> g_glassLightIndices;
    std::vector<ViewportData> g_viewportData;

    std::vector<DecalPaintingInfo> g_decalPaintingInfo;

    std::vector<BloodDecalInstanceData> g_bloodScreenSpaceDecalInstances;

    std::vector<glm::mat4> g_skinningTransforms;
    std::vector<glm::mat4> g_previousSkinningTransforms;
    std::vector<RenderItem> g_combinedSkinnedRenderItems;

    std::vector<RenderItem> g_skinnedRenderItemsDefault;
    std::vector<RenderItem> g_skinnedRenderItemsAlphaDiscard;
    std::vector<RenderItem> g_skinnedRenderItemsBlended;
    std::vector<RenderItem> g_skinnedRenderItemsHair;

    std::vector<RenderItem> g_skinnedNonDeformingSkinnedMeshRenderItems;
    std::vector<RenderItem> g_skinnedNonDeformingSkinnedMeshRenderItemsAlphaDiscard;
    std::vector<RenderItem> g_skinnedNonDeformingSkinnedMeshRenderItemsBlended;
    std::vector<RenderItem> g_skinnedNonDeformingSkinnedMeshRenderItemsHair;

    std::vector<SpriteSheetRenderItem> g_spriteSheetRenderItems;
    std::vector<SpriteSheetRenderItem> g_spriteSheetInstanceData;

    std::vector<RenderItem> g_nonDeformingSkinnedMeshRenderItemsDepthPeeledTransparent;
    uint32_t g_baseSkinnedVertex = 0;
    int32_t g_blackTextureIndex = -1;

    std::vector<float> g_shadowCascadeLevels{ 5.0f, 10.0f, 20.0f, 40.0f }; // WARNING! YOU have a duplicate of this in GL_renderer.h

    std::vector<SkinningJob> g_skinningJobs;
    std::vector<SkinningDispatchGroup> g_skinningDispatchGroups;

    std::vector<TransientRayQueryBLASInstance> g_transientRayQueryBLASInstances;
    std::vector<RayQueryMultiMeshBLAS> g_rayQueryMultiMeshBLASes;
    std::vector<RayQueryBLASInstance> g_rayQueryBLASInstances;
    uint64_t g_proceduralRayQueryBLASId = 0;

    uint64_t g_frameIndex = 0;
    glm::vec2 g_jitterPx = glm::vec2(0.0f);


    const std::vector<SkinningJob>& GetSkinningJobs()                           { return g_skinningJobs; }
    const std::vector<SkinningDispatchGroup>& GetSkinningDispatchGroups()       { return g_skinningDispatchGroups; }
    const std::vector<TransientRayQueryBLASInstance>& GetTransientRayQueryBLASInstances() { return g_transientRayQueryBLASInstances; }
    const std::vector<RayQueryMultiMeshBLAS>& GetRayQueryMultiMeshBLASes()             { return g_rayQueryMultiMeshBLASes; }
    const std::vector<RayQueryBLASInstance>& GetRayQueryBLASInstances()                   { return g_rayQueryBLASInstances; }

    void CreateGPULights();
    void UpdateViewportData();
    void UpdateRendererData();
    void UpdateDrawCommandsSet();
    void UpdatePointLightShadowMapDrawCommands();
    void ClearPointLightShadowMapDrawCommands(PointLightShadowMapDrawCommands& drawCommands);
    void CreatePointLightShadowMapDrawCommands(PointLightShadowMapDrawCommands& drawCommands, const std::vector<ShadowMapInfo>& shadowMaps, std::vector<RenderItem>& renderItems, bool includeSkinned, bool includeProcedural);

    // Compute skinning
    void CreateSkinningData(); // name me better
    void CreateSkinningDistpachGroups();

    void CreateDrawCommands(std::vector<DrawIndexedIndirectCommand>& drawCommands, std::vector<RenderItem>& renderItems, Unloved::Frustum* frustum, int viewportIndex, bool ignoreNonShadowCasters = false);
    void CreateHeightMapDrawCommands(std::vector<DrawIndexedIndirectCommand>& drawCommands, Unloved::Frustum& frustum, int viewportIndex);
    void CreateDrawCommandsSkinned(std::vector<DrawIndexedIndirectCommand>& commands, std::vector<RenderItem>& renderItems, int viewportIndex, Unloved::Frustum* frustum = nullptr);
    void CreateDrawCommandsNonDeformingSkinned(std::vector<DrawIndexedIndirectCommand>& commands, std::vector<RenderItem>& renderItems, int viewportIndex, Unloved::Frustum* frustum = nullptr);

    void CreateMultiDrawIndirectCommands(std::vector<DrawIndexedIndirectCommand>& commands, std::span<RenderItem> renderItems, int viewportIndex, int instanceOffset);
    void CreateMultiDrawIndirectCommandsSkinned(std::vector<DrawIndexedIndirectCommand>& commands, std::span<RenderItem> renderItems, int viewportIndex, int instanceOffset);
    void CreateMultiDrawIndirectCommandsSkinnedNonDeforming(std::vector<DrawIndexedIndirectCommand>& commands, std::span<RenderItem> renderItems, int viewportIndex, int instanceOffset);

    void CreateDrawCommandProcedural(std::vector<DrawIndexedIndirectCommand>& drawCommands, std::vector<RenderItem>& renderItems, Unloved::Frustum* frustum, int viewportIndex);
	void CreateProceduralMultiDrawIndirectCommands(std::vector<DrawIndexedIndirectCommand>& commands, std::span<RenderItem> renderItems, int viewportIndex, int instanceOffset);
    void CreateSpriteSheetDrawCommands();

    void CreateShadowCubeMapMultiDrawIndirectCommands(std::vector<DrawIndexedIndirectCommand>& commands, std::vector<RenderItem>& renderItems, uint32_t faceIndex, Light* light, BlendingMode blendingModeFilter);
    void CreateMoonLightShadowMapDrawCommands();
    void ClearDrawCommandsSet();
    void SortDrawCommandRenderItems();
    void CreateViewportDrawCommands();
    void CreateFlashLightShadowMapDrawCommands();
    void UpdateBloodScreenSpaceDecalInstances();


    int EncodeBaseInstance(int playerIndex, int instanceOffset);
    void DecodeBaseInstance(int baseInstance, int& playerIndex, int& instanceOffset);

    void BeginFrame() {
        // Compute skinning
        g_skinningJobs.clear();
        g_skinningDispatchGroups.clear();
        g_skinningTransforms.clear();
        g_previousSkinningTransforms.clear();
        g_baseSkinnedVertex = 0;

        // Vulkan raytracing
        g_transientRayQueryBLASInstances.clear();
        g_rayQueryMultiMeshBLASes.clear();
        g_rayQueryBLASInstances.clear();

        // Skinned (deforming)
        g_combinedSkinnedRenderItems.clear();
        g_skinnedRenderItemsDefault.clear();
        g_skinnedRenderItemsAlphaDiscard.clear();
        g_skinnedRenderItemsBlended.clear();
        g_skinnedRenderItemsHair.clear();

        // Skinned (non deforming)
        g_skinnedNonDeformingSkinnedMeshRenderItems.clear();
        g_skinnedNonDeformingSkinnedMeshRenderItemsAlphaDiscard.clear();
        g_skinnedNonDeformingSkinnedMeshRenderItemsBlended.clear();
        g_skinnedNonDeformingSkinnedMeshRenderItemsHair.clear();

		g_nonDeformingSkinnedMeshRenderItemsDepthPeeledTransparent.clear();

		g_renderItemsProcedural.clear();

        g_spriteSheetRenderItems.clear();
        g_spriteSheetInstanceData.clear();

        g_renderItems.clear();
		g_renderItemsMirror.clear();
		g_renderItemsPlastic.clear();
        g_renderItemsBlended.clear();
        g_renderItemsAlphaDiscarded.clear();
        g_renderItemsHair.clear();
        g_renderItemsEmissive.clear();

        g_renderItemsPointLightShadows.clear();
        g_renderItemsStaticPointLightShadows.clear();
        g_renderItemsDynamicPointLightShadows.clear();
        g_renderItemsMoonLightShadows.clear();

        // Think about better names for these containers below
        g_renderItemsOutline.clear();
        g_decalPaintingInfo.clear();
		g_shadowCasterRenderItems.clear();
		g_renderItemsStainedGlass.clear();
		g_renderItemsGlass.clear();

        g_drawCommandsUI.clear();

        g_blackTextureIndex = ResourceManager::GetTextureBindlessIndexByName("Black");

        // TAA Jitter
        g_frameIndex++;
        g_jitterPx = glm::vec2(Hell::Random::Halton(g_frameIndex, 2u), Hell::Random::Halton(g_frameIndex, 3u)) - 0.5f;
    }

    void Update() {
        ProfilerCPUZoneFunction();

        CreateGPULights();
        CreateSkinningDistpachGroups();

        UpdateViewportData();
        UpdateRendererData();
        UpdateDrawCommandsSet();
        UpdateDrawCommandsUI();
        UpdatePointLightShadowMapDrawCommands();
    }

    void UpdateDrawCommandsUI() {
        const std::vector<RenderItemUI>& renderItems = UIBackEnd::GetRenderItems();
        g_drawCommandsUI.resize(renderItems.size());

        for (uint32_t i = 0; i < renderItems.size(); i++) {
            const RenderItemUI& renderItem = renderItems[i];
            DrawIndexedIndirectCommand& command = g_drawCommandsUI[i];
            command.indexCount = renderItem.indexCount;
            command.instanceCount = 1;
            command.firstIndex = renderItem.baseIndex;
            command.baseVertex = renderItem.baseVertex;
            command.baseInstance = i;
        }
    }

    const std::vector<DrawIndexedIndirectCommand>& GetDrawCommandsUI() {
        return g_drawCommandsUI;
    }

    void UpdateViewportData() {
        const RendererSettings& rendererSettings = Renderer::GetCurrentRendererSettings();
        const Resolutions& resolutions = Config::GetResolutions();

        g_viewportData.resize(4);

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            g_viewportData[i].colorTint = WHITE;
            g_viewportData[i].colorContrast = 1.0f;
            g_viewportData[i].isInShop = false;

            glm::mat4 viewMatrix = glm::mat4(1);
            if (Editor::IsOpen()) {
                viewMatrix = Editor::GetViewportViewMatrix(i);
                g_viewportData[i].orthoSize = Editor::GetEditorOrthoSize(i);
				g_viewportData[i].isOrtho = true;
				g_viewportData[i].fov = 1.0f;

				g_viewportData[i].vignetteIntensityScalar = 0.0f;
				g_viewportData[i].vignetteColor = glm::vec4(0.0f);
            }
            else {
                g_viewportData[i].orthoSize = 0.0f;
                g_viewportData[i].isOrtho = false;
                g_viewportData[i].fov = Unloved::Session::GetLocalPlayerFovByViewportIndex(i);

                Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i);
                if (player) {
                    g_viewportData[i].colorTint = glm::vec4(player->GetViewportColorTint(), 1.0f);
                    g_viewportData[i].colorContrast = player->GetViewportContrast();

                    if (player->IsDead()) {
                        viewMatrix = player->m_deathCamViewMatrix;
                    }
                    else {
                        viewMatrix = Unloved::Session::GetLocalPlayerCameraByViewportIndex(i)->GetViewMatrix();
                    }

                    g_viewportData[i].isInShop = player->IsInShop();

                    g_viewportData[i].vignetteIntensityScalar = player->GetVignettIntensityScalar();
					g_viewportData[i].vignetteColor = glm::vec4(player->GetVignetteColor(), 0.0f);
                }
            }
            glm::mat4 inverseView = glm::inverse(viewMatrix);
            glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
            glm::vec3 cameraRight = glm::vec3(inverseView[0]);
            glm::vec3 cameraUp = glm::vec3(inverseView[1]);
            glm::vec3 cameraForward = -glm::vec3(inverseView[2]);

            // Is there any previous data?
            bool previousDataExists = !Hell::Math::NearlyEqual(g_viewportData[i].previousProjectionView, glm::mat4(1.0f));

            // Previous
            if (previousDataExists) {
                g_viewportData[i].previousProjectionView = g_viewportData[i].projectionView;
            }

            g_viewportData[i].prevProjectionView = g_viewportData[i].projectionView;
            g_viewportData[i].prevProjectionViewReverseZ = g_viewportData[i].projectionViewReverseZ;

            g_viewportData[i].cameraForward = glm::vec4(cameraForward, 0.0f);
            g_viewportData[i].cameraRight = glm::vec4(cameraRight, 0.0f);
			g_viewportData[i].cameraUp = glm::vec4(cameraUp, 0.0f);
			g_viewportData[i].projection = viewport->GetProjectionMatrix();
            g_viewportData[i].inverseProjection = glm::inverse(g_viewportData[i].projection);
            g_viewportData[i].view = viewMatrix;
            g_viewportData[i].inverseView = inverseView;
            g_viewportData[i].projectionView = g_viewportData[i].projection * g_viewportData[i].view;
            g_viewportData[i].inverseProjectionView = glm::inverse(g_viewportData[i].projectionView);
            g_viewportData[i].skyboxProjectionView = viewport->GetPerpsectiveMatrix() * g_viewportData[i].view;
            g_viewportData[i].width = (int)(resolutions.gBuffer.x * viewport->GetSize().x);
            g_viewportData[i].height = (int)(resolutions.gBuffer.y * viewport->GetSize().y);
            g_viewportData[i].xOffset = (int)(resolutions.gBuffer.x * viewport->GetPosition().x);
            g_viewportData[i].yOffset = (int)(resolutions.gBuffer.y * viewport->GetPosition().y);
            g_viewportData[i].posX = viewport->GetPosition().x;
            g_viewportData[i].posY = viewport->GetPosition().y;
            g_viewportData[i].sizeX = viewport->GetSize().x;
            g_viewportData[i].sizeY = viewport->GetSize().y;
            g_viewportData[i].viewPos = g_viewportData[i].inverseView[3];

			g_viewportData[i].projectionReverseZ = viewport->GetProjectionMatrixReverseZ();
			g_viewportData[i].inverseProjectionReverseZ = glm::inverse(g_viewportData[i].projectionReverseZ);
			g_viewportData[i].projectionViewReverseZ = g_viewportData[i].projectionReverseZ * g_viewportData[i].view;
			g_viewportData[i].inverseProjectionViewReverseZ = glm::inverse(g_viewportData[i].projectionViewReverseZ);

            glm::vec2 projectionShiftNdc(0.0f);
            if (Renderer::GetCurrentRendererSettings().enableTAA) {
                const glm::vec2 viewportSize(
                    static_cast<float>(g_viewportData[i].width),
                    static_cast<float>(g_viewportData[i].height)
                );

                projectionShiftNdc = static_cast<float>(-rendererSettings.taaJitterScale) * g_jitterPx / viewportSize;
            }

            // GLM uses column-major indexing: matrix[column][row]. Pre-multiplying
            // applies a homogeneous clip-space translation after projection.
            glm::mat4 jitterMatrix(1.0f);
            jitterMatrix[3][0] = projectionShiftNdc.x;
            jitterMatrix[3][1] = projectionShiftNdc.y;

            g_viewportData[i].jitteredProjectionViewReverseZ = jitterMatrix * g_viewportData[i].projectionViewReverseZ;
            g_viewportData[i].inverseJitteredProjectionViewReverseZ = glm::inverse(g_viewportData[i].jitteredProjectionViewReverseZ);

            // If no previous then use current frame values
            if (previousDataExists) {
                g_viewportData[i].previousProjectionView = g_viewportData[i].projectionView;
            }

            viewport->GetFrustum().Update(g_viewportData[i].projectionView);

            g_viewportData[i].frustumPlane0 = viewport->GetFrustum().GetPlane(0);
            g_viewportData[i].frustumPlane1 = viewport->GetFrustum().GetPlane(1);
            g_viewportData[i].frustumPlane2 = viewport->GetFrustum().GetPlane(2);
            g_viewportData[i].frustumPlane3 = viewport->GetFrustum().GetPlane(3);
            g_viewportData[i].frustumPlane4 = viewport->GetFrustum().GetPlane(4);
            g_viewportData[i].frustumPlane5 = viewport->GetFrustum().GetPlane(5);

            // Flashlight
            if (Editor::IsOpen()) {
                g_viewportData[i].flashlightModifer = 0;
                g_viewportData[i].flashlightProjectionView = glm::mat4(1);
                g_viewportData[i].flashlightDir = glm::vec4(0.0f);
                g_viewportData[i].flashlightPosition = glm::vec4(0.0f);
            }
            else {
                Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i);
                if (player) {
                    g_viewportData[i].flashlightProjectionView = player->GetFlashlightProjectionView();
                    g_viewportData[i].flashlightDir = glm::vec4(player->GetFlashlightDirection(), 0.0f);
                    g_viewportData[i].flashlightPosition = glm::vec4(player->GetFlashlightPosition(), 0.0f);
                    g_viewportData[i].flashlightModifer = player->GetFlashLightModifer();
                }
            }

            // CSM matrices
            glm::vec3 lightDir = Unloved::World::GetMoonlightDirection();
            float viewportWidth = g_viewportData[i].width;
            float viewportHeight = g_viewportData[i].height;
            float fov = g_viewportData[i].fov;
            const std::vector<glm::mat4> lightProjectionViews = RendererUtil::GetLightProjectionViews(viewMatrix, lightDir, g_shadowCascadeLevels, viewportWidth, viewportHeight, fov);

            if (lightProjectionViews.size() != SHADOW_CASCADE_COUNT) Logging::Error() << "INCORRECT SIZE: " << lightProjectionViews.size();
            for (int j = 0; j < SHADOW_CASCADE_COUNT && j < lightProjectionViews.size(); j++) {
                g_viewportData[i].csmLightProjectionView[j] = lightProjectionViews[j];
            }
        }
    }

    void UpdateRendererData() {
        const RendererSettings& rendererSettings = Renderer::GetCurrentRendererSettings();
        const Resolutions& resolutions = Config::GetResolutions();
        const Config::Flashlight::Settings& flashlightSettings = Config::Flashlight::GetSettings();
        IESProfile* flashlightIESProfile = Hell::ResourceManager::GetIESProfilePtr(flashlightSettings.iesProfile);

        g_rendererData.nearPlane = Config::GetNearPlane();
        g_rendererData.farPlane = Config::GetFarPlane();
        g_rendererData.gBufferWidth = (float)resolutions.gBuffer.x;
        g_rendererData.gBufferHeight = (float)resolutions.gBuffer.y;
        g_rendererData.hairBufferWidth = (float)resolutions.hair.x;
        g_rendererData.hairBufferHeight = (float)resolutions.hair.y;
        g_rendererData.splitscreenMode = (int)Session::GetSplitscreenMode();
        g_rendererData.time = Session::GetSessionTime();
        g_rendererData.rendererOverrideState = (int)rendererSettings.rendererOverrideState;
        g_rendererData.normalizedMouseX = Hell::Math::MapRange(Input::GetMouseX(), 0, Hell::BackEnd::GetCurrentWindowWidth(), 0.0f, 1.0f);
        g_rendererData.normalizedMouseY = Hell::Math::MapRange(Input::GetMouseY(), 0, Hell::BackEnd::GetCurrentWindowHeight(), 0.0f, 1.0f);
        g_rendererData.tileCountX = Renderer::GetTileCountX();
        g_rendererData.tileCountY = Renderer::GetTileCountY();
        g_rendererData.lightCount = static_cast<uint32_t>(g_gpuLights.size());
        g_rendererData.moonLightDir = glm::vec4(World::GetMoonlightDirection(), 0.0f);
        g_rendererData.enableDDGI = rendererSettings.enableDDGI;
        g_rendererData.enableDDGIReflections = rendererSettings.enableDDGIReflections;
        g_rendererData.enableIndirectSpecular = rendererSettings.enableIndirectSpecular;
        g_rendererData.taaJitterPx = g_jitterPx;
        g_rendererData.enableTAA = rendererSettings.enableTAA;
        g_rendererData.indirectSpecularFactor = rendererSettings.indirectSpecularFactor;
        g_rendererData.indirectSpecularRoughnessDampening = rendererSettings.indirectSpecularRoughnessDampening;
        g_rendererData.directPointShadowMode = static_cast<uint32_t>(rendererSettings.directPointShadowMode);

        g_rendererData.flashlightColor = glm::vec4(flashlightSettings.color, 1.0f);
        g_rendererData.flashlightRange = flashlightSettings.range;
        g_rendererData.flashlightFalloffExponent = flashlightSettings.falloffExponent;
        g_rendererData.flashlightBrightness = flashlightSettings.brightness;
        g_rendererData.flashlightIESConeScale = flashlightSettings.iesConeScale;
        g_rendererData.flashlightIESInnerAngle = flashlightSettings.iesInnerAngle;
        g_rendererData.flashlightIESOuterAngle = flashlightSettings.iesOuterAngle;
        g_rendererData.flashlightIESContrast = flashlightSettings.iesContrast;
        g_rendererData.flashlightIESVerticalScale = flashlightIESProfile ? flashlightIESProfile->GetVScale() : 0.0f;
        g_rendererData.flashlightIESVerticalBias = flashlightIESProfile ? flashlightIESProfile->GetVBias() : 0.0f;
        g_rendererData.flashlightIESHorizontalBias = flashlightIESProfile ? flashlightIESProfile->GetHBias() : 0.0f;
        g_rendererData.flashlightIESTextureIndex = flashlightIESProfile ? static_cast<int32_t>(flashlightIESProfile->GetTextureIndex()) : -1;
        g_rendererData.flashlightIESEnabled = flashlightSettings.iesEnabled ? 1u : 0u;
        g_rendererData.flashlightCenterSpotRange = flashlightSettings.centerSpotRange;
        g_rendererData.flashlightCenterSpotFalloffExponent = flashlightSettings.centerSpotFalloffExponent;
        g_rendererData.flashlightCenterSpotBrightness = flashlightSettings.centerSpotBrightness;
        g_rendererData.flashlightCenterSpotInnerAngle = flashlightSettings.centerSpotInnerAngle;
        g_rendererData.flashlightCenterSpotOuterAngle = flashlightSettings.centerSpotOuterAngle;
        g_rendererData.flashlightCenterSpotEnabled = flashlightSettings.centerSpotEnabled ? 1u : 0u;
    }

    void SortRenderItems(std::vector<RenderItem>& renderItems) {
        std::sort(renderItems.begin(), renderItems.end(), [](const RenderItem& a, const RenderItem& b) {
            return a.meshId < b.meshId;
        });
    }

    void SortRenderItemsByMeshId(std::vector<RenderItem>& renderItems) {
        std::sort(renderItems.begin(), renderItems.end(), [](const RenderItem& a, const RenderItem& b) {
            return a.meshId < b.meshId;
        });
    }

    bool ValidateRenderItemMeshRange(const RenderItem& renderItem, const char* submitFunction) {
        if (renderItem.vertexCount != 0 && renderItem.indexCount != 0) {
            return true;
        }

        Logging::Error()
            << submitFunction << " rejected a RenderItem with an empty mesh range: meshId=" << renderItem.meshId
            << ", vertexCount=" << renderItem.vertexCount
            << ", indexCount=" << renderItem.indexCount
            << ", baseVertex=" << renderItem.baseVertex
            << ", baseIndex=" << renderItem.baseIndex << "\n";

        __debugbreak();

        return false;
    }

    void SetDrawCommandMeshRange(DrawIndexedIndirectCommand& command, const RenderItem& renderItem) {
        command.indexCount = renderItem.indexCount;
        command.firstIndex = renderItem.baseIndex;
        command.baseVertex = renderItem.baseVertex;
    }

    void CreateMoonLightShadowMapDrawCommands() {
        ProfilerCPUZone("Moon shadow commands");

        auto& set = g_drawCommandsSet;
        int viewportCount = 4;
        int cascadeCount = SHADOW_CASCADE_COUNT;

        // Clear last frames draw commands
        for (int x = 0; x < viewportCount; x++) {
            for (int y = 0; y < cascadeCount; y++) {
                set.moonLightCascades[x][y].clear();
            }
        }

        //std::vector<RenderItem> potentialRenderItems = g_renderItems; // First start with everything in the scene
        //potentialRenderItems.insert(potentialRenderItems.end(), g_shadowCasterRenderItems.begin(), g_shadowCasterRenderItems.end());

        Unloved::Frustum frustum;

        for (int i = 0; i < viewportCount; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport || !viewport->IsVisible()) continue;

            for (int j = 0; j < cascadeCount; j++) {
                frustum.Update(g_viewportData[i].csmLightProjectionView[j]);

                // Store the instance offset for this player
                int instanceStart = g_instanceData.size();

                // Preallocate an estimate
                g_instanceData.reserve(g_instanceData.size() + g_renderItemsMoonLightShadows.size());

                // Append new render items to the global instance data if its within this cascade's frustum
                for (const RenderItem& renderItem : g_renderItemsMoonLightShadows) {
                    if (frustum.IntersectsAABBFast(renderItem)) {
                        g_instanceData.push_back(renderItem);
                        //DebugDraw::DrawAABB(AABB(renderItem.aabbMin, renderItem.aabbMax), YELLOW);
                    }
                }

                // Create indirect draw commands using the stored offset
                std::span<RenderItem> instanceView(g_instanceData.begin() + instanceStart, g_instanceData.end());
                CreateMultiDrawIndirectCommands(set.moonLightCascades[i][j], instanceView, -1, instanceStart);
            }
        }
    }

	const std::vector<RenderItem>& GetNonDeformingSkinnedMeshRenderItems() {
		return g_skinnedNonDeformingSkinnedMeshRenderItems;
	}

	const std::vector<RenderItem>& GetNonDeformingSkinnedMeshRenderItemsAlphaDiscard() {
		return g_skinnedNonDeformingSkinnedMeshRenderItemsAlphaDiscard;
	}

	const std::vector<RenderItem>& GetNonDeformingSkinnedMeshRenderItemsDepthPeeledTransparent() {
		return g_nonDeformingSkinnedMeshRenderItemsDepthPeeledTransparent;
	}

    void CreateSpriteSheetDrawCommands() {
        ProfilerCPUZone("Sprite sheet commands");

        g_spriteSheetInstanceData.clear();

        Mesh* quadMesh = Hell::ResourceManager::GetQuadMesh();
        if (!quadMesh) return;

        auto& set = g_drawCommandsSet;

        for (int viewportIndex = 0; viewportIndex < 4; viewportIndex++) {
            std::vector<DrawIndexedIndirectCommand>& commands = set.spriteSheets[viewportIndex];
            commands.clear();

            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(viewportIndex);
            if (!viewport || !viewport->IsVisible()) continue;

            const int instanceStart = static_cast<int>(g_spriteSheetInstanceData.size());
            const glm::vec3 cameraPosition = glm::vec3(g_viewportData[viewportIndex].viewPos);

            for (const SpriteSheetRenderItem& renderItem : g_spriteSheetRenderItems) {
                if (renderItem.exclusiveViewportIndex != -1 && renderItem.exclusiveViewportIndex != viewportIndex) continue;

                glm::vec3 position = glm::vec3(renderItem.modelMatrix[3]);
                glm::vec3 toCamera = position - cameraPosition;
                float distanceSquared = glm::dot(toCamera, toCamera);

                size_t insertIndex = g_spriteSheetInstanceData.size();
                g_spriteSheetInstanceData.push_back(renderItem);

                while (insertIndex > static_cast<size_t>(instanceStart)) {
                    const SpriteSheetRenderItem& previousRenderItem = g_spriteSheetInstanceData[insertIndex - 1];
                    glm::vec3 previousPosition = glm::vec3(previousRenderItem.modelMatrix[3]);
                    glm::vec3 previousToCamera = previousPosition - cameraPosition;
                    float previousDistanceSquared = glm::dot(previousToCamera, previousToCamera);

                    if (previousDistanceSquared >= distanceSquared) {
                        break;
                    }

                    g_spriteSheetInstanceData[insertIndex] = previousRenderItem;
                    insertIndex--;
                }

                g_spriteSheetInstanceData[insertIndex] = renderItem;
            }

            const uint32_t instanceCount = static_cast<uint32_t>(g_spriteSheetInstanceData.size() - instanceStart);
            if (instanceCount == 0) continue;

            DrawIndexedIndirectCommand& command = commands.emplace_back();
            command.indexCount = quadMesh->indexCount;
            command.instanceCount = instanceCount;
            command.firstIndex = quadMesh->baseIndex;
            command.baseVertex = quadMesh->baseVertex;
            command.baseInstance = EncodeBaseInstance(viewportIndex, instanceStart);
        }
    }


    void CreateGlassDrawCommands() {
        ProfilerCPUZone("Glass commands");

        // Reuse these every frame
        static std::vector<RenderItem> visibleGlassRenderItems[4];

        auto& set = g_drawCommandsSet;
        // Pack all viewport glass into one SSBO
        g_glassInstanceData.clear();
        g_glassLightRanges.clear();
        g_glassLightIndices.clear();

        for (int viewportIndex = 0; viewportIndex < 4; viewportIndex++) {
            std::vector<RenderItem>& renderItems = visibleGlassRenderItems[viewportIndex];
            std::vector<DrawIndexedIndirectCommand>& drawCommands = set.glassDrawCommands[viewportIndex];
            renderItems.clear();
            drawCommands.clear();

            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(viewportIndex);
            if (!viewport->IsVisible()) continue;

            Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(viewportIndex);
            if (!player) continue;

            Unloved::Frustum& frustum = viewport->GetFrustum();

            // Cull glass for this viewport
            for (const RenderItem& renderItem : g_renderItemsGlass) {
                if (frustum.IntersectsAABB(AABB(renderItem.aabbMin, renderItem.aabbMax))) {
                    renderItems.push_back(renderItem);
                }
            }

            // Glass needs to render back to front
            std::sort(renderItems.begin(), renderItems.end(), [player](const RenderItem& a, const RenderItem& b) {
                float distA = glm::distance(player->GetCameraPosition(), glm::vec3(a.modelMatrix[3]));
                float distB = glm::distance(player->GetCameraPosition(), glm::vec3(b.modelMatrix[3]));
                return distA > distB;
            });

            // Build the commands and matching instance data together
            for (const RenderItem& renderItem : renderItems) {
                DrawIndexedIndirectCommand command;
                SetDrawCommandMeshRange(command, renderItem);

                command.instanceCount = 1;

                // baseInstance indexes the packed glass SSBO
                command.baseInstance = static_cast<uint32_t>(g_glassInstanceData.size());
                drawCommands.push_back(command);

                GlassLightRange lightRange{};
                lightRange.offset = static_cast<uint32_t>(g_glassLightIndices.size());

                const AABB renderItemBounds(glm::vec3(renderItem.aabbMin), glm::vec3(renderItem.aabbMax));

                // Cull the full light list once per glass instance
                for (uint32_t lightIndex = 0; lightIndex < g_gpuLights.size(); lightIndex++) {
                    const GPULight& light = g_gpuLights[lightIndex];
                    const glm::vec3 lightBoundsMin = glm::vec3(light.worldBoundsMin);
                    const glm::vec3 lightBoundsMax = glm::vec3(light.worldBoundsMax);

                    if (!renderItemBounds.IntersectsAABB(lightBoundsMin, lightBoundsMax)) continue;

                    const glm::vec3 lightPosition(light.posX, light.posY, light.posZ);
                    if (!renderItemBounds.IntersectsSphere(lightPosition, light.radius)) continue;

                    g_glassLightIndices.push_back(lightIndex);
                }

                lightRange.count = static_cast<uint32_t>(g_glassLightIndices.size()) - lightRange.offset;

                g_glassInstanceData.push_back(renderItem);
                g_glassLightRanges.push_back(lightRange);
            }
        }
    }

    void ClearDrawCommandsSet() {
        ProfilerCPUZone("Clear commands");

        g_instanceData.clear();
        auto& set = g_drawCommandsSet;

        for (int i = 0; i < 4; i++) {
            set.standard[i].clear();
            set.blended[i].clear();
            set.alphaDiscard[i].clear();
            set.hair[i].clear();
            set.mirrorRenderItems[i].clear();
            set.plastic[i].clear();
            set.procedural[i].clear();
            set.heightMap[i].clear();
            set.emissive[i].clear();
            set.spriteSheets[i].clear();

            g_flashLightShadowMapDrawInfo.flashlightShadowMapGeometry[i].clear();
            g_flashLightShadowMapDrawInfo.heightMapChunkIndices[i].clear();
        }
    }

    void SortDrawCommandRenderItems() {
        ProfilerCPUZone("Sort render items");

        SortRenderItems(g_renderItems);
        SortRenderItems(g_renderItemsBlended);
        SortRenderItems(g_renderItemsAlphaDiscarded);
        SortRenderItems(g_renderItemsHair);
        SortRenderItems(g_renderItemsPlastic);
        SortRenderItemsByMeshId(g_renderItemsProcedural);

        SortRenderItems(g_renderItemsPointLightShadows);
        SortRenderItems(g_renderItemsStaticPointLightShadows);
        SortRenderItems(g_renderItemsDynamicPointLightShadows);
        SortRenderItems(g_renderItemsMoonLightShadows);

        SortRenderItems(g_renderItemsEmissive);
    }

    void CreateViewportDrawCommands() {
        ProfilerCPUZone("Viewport commands");

        auto& set = g_drawCommandsSet;

        // Lil hack to include bullet decals in mirrors
        int count = g_renderItems.size() + g_renderItemsAlphaDiscarded.size();
        std::vector<RenderItem> potentialMirrorItems;
        potentialMirrorItems.reserve(count);
        potentialMirrorItems.insert(potentialMirrorItems.end(), g_renderItems.begin(), g_renderItems.end());
        potentialMirrorItems.insert(potentialMirrorItems.end(), g_renderItemsAlphaDiscarded.begin(), g_renderItemsAlphaDiscarded.end());

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            Unloved::Frustum& frustum = viewport->GetFrustum();
            CreateDrawCommands(set.standard[i], g_renderItems, &frustum, i);
            CreateDrawCommands(set.standard[i], g_renderItemsMirror, &frustum, i);
            CreateDrawCommands(set.blended[i], g_renderItemsBlended, &frustum, i);
            CreateDrawCommands(set.alphaDiscard[i], g_renderItemsAlphaDiscarded, &frustum, i);
            CreateDrawCommands(set.hair[i], g_renderItemsHair, &frustum, i);
            CreateDrawCommands(set.plastic[i], g_renderItemsPlastic, &frustum, i);
            CreateDrawCommands(set.emissive[i], g_renderItemsEmissive, &frustum, i);
            CreateDrawCommandProcedural(set.procedural[i], g_renderItemsProcedural, &frustum, i);
            CreateHeightMapDrawCommands(set.heightMap[i], frustum, i);

            if (Mirror* mirror = Unloved::MirrorManager::GetMirrorByObjectId(viewport->GetMirrorId())) {
                CreateDrawCommands(set.mirrorRenderItems[i], potentialMirrorItems, mirror->GetFrustum(i), i);
            }
        }
    }

    void CreateFlashLightShadowMapDrawCommands() {
        ProfilerCPUZone("Flashlight commands");

        for (int playerIndex = 0; playerIndex < Unloved::Session::GetLocalPlayerCount(); playerIndex++) {
            Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(playerIndex);
            if (!player) continue;

            Unloved::Frustum flashLightFrustum = player->GetFlashlightFrustum();

            // Build multi draw commands for regular geometry
            CreateDrawCommands(g_flashLightShadowMapDrawInfo.flashlightShadowMapGeometry[playerIndex], g_renderItems, &flashLightFrustum, playerIndex, true);

            // Frustum cull the heightmap chunks
            std::vector<HeightMapChunk>& chunks = LegacyWorld::GetHeightMapChunks();
            for (int i = 0; i < chunks.size(); i++) {
                HeightMapChunk& chunk = chunks[i];
                if (flashLightFrustum.IntersectsAABBFast(AABB(chunk.aabbMin, chunk.aabbMax))) {
                    g_flashLightShadowMapDrawInfo.heightMapChunkIndices[playerIndex].push_back(i);
                }
            }
        }
    }

    void UpdateBloodScreenSpaceDecalInstances() {
        ProfilerCPUZone("Blood decals");

        std::vector<BloodScreenSpaceDecal>& bloodScreenSpaceDecals = Unloved::BloodSystemOLD::GetBloodScreenSpaceDecals();
        std::sort(bloodScreenSpaceDecals.begin(), bloodScreenSpaceDecals.end(), [](const BloodScreenSpaceDecal& a, const BloodScreenSpaceDecal& b) {
            return a.m_type < b.m_type;
        });

        int instanceCount = static_cast<int>(bloodScreenSpaceDecals.size());
        g_bloodScreenSpaceDecalInstances.resize(instanceCount);

        int bloodDecalTextureIndices[4] = { -1, -1, -1, -1 };
        glm::vec2 bloodDecalAspectScales[4] = {
            glm::vec2(1.0f),
            glm::vec2(1.0f),
            glm::vec2(1.0f),
            glm::vec2(1.0f)
        };

        if (instanceCount > 0) {
            const char* bloodDecalTextureNames[4] = {
                "BloodDecal4",
                "BloodDecal6",
                "BloodDecal7",
                "BloodDecal9"
            };

            for (int type = 0; type < 4; type++) {
                Texture* texture = Hell::ResourceManager::GetTextureByName(bloodDecalTextureNames[type]);
                if (!texture) continue;

                float textureWidth = glm::max(static_cast<float>(texture->GetWidth()), 1.0f);
                float textureHeight = glm::max(static_cast<float>(texture->GetHeight()), 1.0f);
                float shortestTextureSide = glm::min(textureWidth, textureHeight);

                bloodDecalTextureIndices[type] = texture->GetBindlessIndex();
                bloodDecalAspectScales[type] = glm::vec2(textureWidth, textureHeight) / shortestTextureSide;
            }
        }

        for (int i = 0; i < instanceCount; i++) {
            BloodScreenSpaceDecal& decal = bloodScreenSpaceDecals[i];
            BloodDecalInstanceData& instance = g_bloodScreenSpaceDecalInstances[i];
            instance.modelMatrix = decal.GetModelMatrix();
            instance.inverseModelMatrix = decal.GetInverseModelMatrix();
            instance.type = decal.GetType();
            instance.textureIndex = -1;
            instance.aspectScaleX = 1.0f;
            instance.aspectScaleY = 1.0f;

            if (instance.type < 0 || instance.type >= 4) continue;

            instance.textureIndex = bloodDecalTextureIndices[instance.type];
            instance.aspectScaleX = bloodDecalAspectScales[instance.type].x;
            instance.aspectScaleY = bloodDecalAspectScales[instance.type].y;
        }
    }

    void UpdateDrawCommandsSet() {
        ProfilerCPUZone("Draw command sets");

        ClearDrawCommandsSet();
        CreateGlassDrawCommands();

        SortDrawCommandRenderItems();
        CreateViewportDrawCommands();

        CreateSpriteSheetDrawCommands();

        CreateSkinningData();


        // CSM render items (moon light shadow maps)
        CreateMoonLightShadowMapDrawCommands();

        CreateFlashLightShadowMapDrawCommands();

        UpdateBloodScreenSpaceDecalInstances();
    }

    void ClearPointLightShadowMapDrawCommands(PointLightShadowMapDrawCommands& drawCommands) {
        for (int shadowMapIndex = 0; shadowMapIndex < MAX_SHADOW_MAP_ARRAY_LEVELS; shadowMapIndex++) {
            for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
                drawCommands.assetGeometry[shadowMapIndex][faceIndex].clear();
                drawCommands.assetGeometryAlphaDiscard[shadowMapIndex][faceIndex].clear();
                drawCommands.assetGeometryHair[shadowMapIndex][faceIndex].clear();
                drawCommands.assetGeometrySkinned[shadowMapIndex][faceIndex].clear();
                drawCommands.assetGeometrySkinnedAlphaDiscard[shadowMapIndex][faceIndex].clear();
                drawCommands.assetGeometrySkinnedHair[shadowMapIndex][faceIndex].clear();
                drawCommands.assetGeometrySkinnedNonDeforming[shadowMapIndex][faceIndex].clear();
                drawCommands.assetGeometrySkinnedNonDeformingAlphaDiscard[shadowMapIndex][faceIndex].clear();
                drawCommands.assetGeometrySkinnedNonDeformingHair[shadowMapIndex][faceIndex].clear();
                drawCommands.procedural[shadowMapIndex][faceIndex].clear();
            }
        }
    }

    void CreatePointLightShadowMapDrawCommands(PointLightShadowMapDrawCommands& drawCommands, const std::vector<ShadowMapInfo>& shadowMaps, std::vector<RenderItem>& renderItems, bool includeSkinned, bool includeProcedural) {
        for (const ShadowMapInfo& shadowMapInfo : shadowMaps) {
            Light* light = Unloved::World::GetLightByObjectId(shadowMapInfo.lightId);
            if (!light) continue;

            const int shadowMapIndex = shadowMapInfo.shadowMapIndex;
            if (shadowMapIndex == -1) continue;

            for (uint32_t faceIndex = 0; faceIndex < 6; faceIndex++) {
                Unloved::Frustum* frustum = light->GetFrustumByFaceIndex(faceIndex);
                if (!frustum) continue;

                CreateShadowCubeMapMultiDrawIndirectCommands(drawCommands.assetGeometry[shadowMapIndex][faceIndex], renderItems, faceIndex, light, BlendingMode::DEFAULT);
                CreateShadowCubeMapMultiDrawIndirectCommands(drawCommands.assetGeometryAlphaDiscard[shadowMapIndex][faceIndex], renderItems, faceIndex, light, BlendingMode::ALPHA_DISCARD);
                CreateShadowCubeMapMultiDrawIndirectCommands(drawCommands.assetGeometryHair[shadowMapIndex][faceIndex], renderItems, faceIndex, light, BlendingMode::HAIR);

                if (includeSkinned) {
                    CreateDrawCommandsSkinned(drawCommands.assetGeometrySkinned[shadowMapIndex][faceIndex], g_skinnedRenderItemsDefault, -1, frustum);
                    CreateDrawCommandsSkinned(drawCommands.assetGeometrySkinnedAlphaDiscard[shadowMapIndex][faceIndex], g_skinnedRenderItemsAlphaDiscard, -1, frustum);
                    CreateDrawCommandsSkinned(drawCommands.assetGeometrySkinnedHair[shadowMapIndex][faceIndex], g_skinnedRenderItemsHair, -1, frustum);
                    CreateDrawCommandsNonDeformingSkinned(drawCommands.assetGeometrySkinnedNonDeforming[shadowMapIndex][faceIndex], g_skinnedNonDeformingSkinnedMeshRenderItems, -1, frustum);
                    CreateDrawCommandsNonDeformingSkinned(drawCommands.assetGeometrySkinnedNonDeformingAlphaDiscard[shadowMapIndex][faceIndex], g_skinnedNonDeformingSkinnedMeshRenderItemsAlphaDiscard, -1, frustum);
                    CreateDrawCommandsNonDeformingSkinned(drawCommands.assetGeometrySkinnedNonDeformingHair[shadowMapIndex][faceIndex], g_skinnedNonDeformingSkinnedMeshRenderItemsHair, -1, frustum);
                }

                if (includeProcedural) {
                    CreateDrawCommandProcedural(drawCommands.procedural[shadowMapIndex][faceIndex], g_renderItemsProcedural, frustum, -1);
                }
            }
        }
    }

    void UpdatePointLightShadowMapDrawCommands() {
        DrawCommandsSet& set = g_drawCommandsSet;

        ClearPointLightShadowMapDrawCommands(set.staticHiResShadowMapDrawCommands);
        ClearPointLightShadowMapDrawCommands(set.staticLowResShadowMapDrawCommands);
        ClearPointLightShadowMapDrawCommands(set.compositeHiResShadowMapDrawCommands);
        ClearPointLightShadowMapDrawCommands(set.compositeLowResShadowMapDrawCommands);

        CreatePointLightShadowMapDrawCommands(set.staticHiResShadowMapDrawCommands, ShadowMapManager::GetStaticDirtyHiResShadowMaps(), g_renderItemsStaticPointLightShadows, false, true);
        CreatePointLightShadowMapDrawCommands(set.staticLowResShadowMapDrawCommands, ShadowMapManager::GetStaticDirtyLowResShadowMaps(), g_renderItemsStaticPointLightShadows, false, true);

        // Both modes feed the same composite pass. Only its caster set changes.
        const bool staticCacheEnabled = ShadowMapManager::StaticCacheEnabled();
        std::vector<RenderItem>& compositeRenderItems = staticCacheEnabled ? g_renderItemsDynamicPointLightShadows : g_renderItemsPointLightShadows;
        const bool includeProcedural = !staticCacheEnabled;
        CreatePointLightShadowMapDrawCommands(set.compositeHiResShadowMapDrawCommands, ShadowMapManager::GetCompositeDirtyHiResShadowMaps(), compositeRenderItems, true, includeProcedural);
        CreatePointLightShadowMapDrawCommands(set.compositeLowResShadowMapDrawCommands, ShadowMapManager::GetCompositeDirtyLowResShadowMaps(), compositeRenderItems, true, includeProcedural);
    }

    void CreateDrawCommandProcedural(std::vector<DrawIndexedIndirectCommand>& drawCommands, std::vector<RenderItem>& renderItems, Unloved::Frustum* frustum, int viewportIndex) {
		// Store the instance offset for this list of commands
		int instanceStart = g_instanceData.size();

		// Preallocate an estimate
		g_instanceData.reserve(g_instanceData.size() + renderItems.size());

		// Append new render items to the global instance data
		for (const RenderItem& renderItem : renderItems) {
			// Skip culling if no frustum is bound, otherwise test intersection
			if (!frustum || frustum->IntersectsAABBFast(renderItem)) {
				g_instanceData.push_back(renderItem);
			}
		}

		// Create indirect draw commands using the stored offset
		std::span<RenderItem> instanceView(g_instanceData.begin() + instanceStart, g_instanceData.end());
		CreateProceduralMultiDrawIndirectCommands(drawCommands, instanceView, viewportIndex, instanceStart);
    }

    void CreateProceduralMultiDrawIndirectCommands(std::vector<DrawIndexedIndirectCommand>& commands, std::span<RenderItem> renderItems, int viewportIndex, int instanceOffset) {
        commands.reserve(commands.size() + renderItems.size());

        DrawIndexedIndirectCommand* currentCommand = nullptr;
        uint32_t currentMeshId = 0;

        for (const RenderItem& renderItem : renderItems) {
            if (!currentCommand || renderItem.meshId != currentMeshId) {
                currentMeshId = renderItem.meshId;

                currentCommand = &commands.emplace_back();
                SetDrawCommandMeshRange(*currentCommand, renderItem);
                currentCommand->baseInstance = EncodeBaseInstance(viewportIndex, instanceOffset);
                currentCommand->instanceCount = 1;
            }
            else {
                currentCommand->instanceCount++;
            }

            instanceOffset++;
        }
    }


    void CreateDrawCommands(std::vector<DrawIndexedIndirectCommand>& drawCommands, std::vector<RenderItem>& renderItems, Unloved::Frustum* frustum, int viewportIndex, bool ignoreNonShadowCasters) {
        // Store the instance offset for this list of commands
        int instanceStart = g_instanceData.size();

        // Preallocate an estimate
        g_instanceData.reserve(g_instanceData.size() + renderItems.size());

        // Append new render items to the global instance data
        for (const RenderItem& renderItem : renderItems) {
            bool shadowCasting = ((renderItem.shadowFlags & SHADOW_FLAG_POINT_LIGHT) != 0u);

            if (ignoreNonShadowCasters && !shadowCasting) continue;
            if (renderItem.ignoredViewportIndex != -1 && renderItem.ignoredViewportIndex == viewportIndex) continue;
            if (renderItem.exclusiveViewportIndex != -1 && renderItem.exclusiveViewportIndex != viewportIndex) continue;

            // If you supplied no frustum, then it passes no matter what
            if (!frustum) {
                g_instanceData.push_back(renderItem);
            }
            // Frustum cull it
            else if (frustum->IntersectsAABBFast(renderItem)) {
                g_instanceData.push_back(renderItem);
            }
        }

        // Create indirect draw commands using the stored offset
        std::span<RenderItem> instanceView(g_instanceData.begin() + instanceStart, g_instanceData.end());
        CreateMultiDrawIndirectCommands(drawCommands, instanceView, viewportIndex, instanceStart);
    }

    void CreateHeightMapDrawCommands(std::vector<DrawIndexedIndirectCommand>& drawCommands, Unloved::Frustum& frustum, int viewportIndex) {
        std::vector<HeightMapChunk>& chunks = LegacyWorld::GetHeightMapChunks();
        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("HeightMapGeometry");

        Transform transform;
        transform.scale = glm::vec3(HEIGHTMAP_SCALE_XZ, HEIGHTMAP_SCALE_Y, HEIGHTMAP_SCALE_XZ);
        glm::mat4 modelMatrix = transform.to_mat4();
        glm::mat4 inverseModelMatrix = glm::inverse(modelMatrix);

        int32_t materialIndex = Hell::ResourceManager::GetMaterialIndexByName("Ground_MudVeg");
        if (materialIndex == -1) {
            materialIndex = Hell::ResourceManager::GetMaterialIndexByName("CheckerBoard");
        }
        if (Editor::IsOpen() && Editor::GetEditorMode() == EditorMode::MAP_HEIGHT_EDITOR) {
            materialIndex = Hell::ResourceManager::GetMaterialIndexByName("CheckerBoard");
        }

        drawCommands.reserve(chunks.size());
        g_instanceData.reserve(g_instanceData.size() + chunks.size());

        for (HeightMapChunk& chunk : chunks) {
            if (Editor::IsClosed() && !frustum.IntersectsAABBFast(AABB(chunk.aabbMin, chunk.aabbMax))) continue;

            Mesh* mesh = meshBuffer.GetMeshById(chunk.meshId);
            if (!mesh) continue;

            RenderItem& renderItem = g_instanceData.emplace_back();
            renderItem.modelMatrix = modelMatrix;
            renderItem.prevModelMatrix = modelMatrix;
            renderItem.inverseModelMatrix = inverseModelMatrix;
            renderItem.aabbMin = glm::vec4(chunk.aabbMin, 0.0f);
            renderItem.aabbMax = glm::vec4(chunk.aabbMax, 0.0f);
            renderItem.vertexCount = mesh->vertexCount;
            renderItem.indexCount = mesh->indexCount;
            renderItem.baseVertex = mesh->baseVertex;
            renderItem.baseIndex = mesh->baseIndex;
            renderItem.materialIndex = materialIndex;
            renderItem.meshId = chunk.meshId;

            DrawIndexedIndirectCommand& command = drawCommands.emplace_back();
            command.indexCount = mesh->indexCount;
            command.instanceCount = 1;
            command.firstIndex = mesh->baseIndex;
            command.baseVertex = mesh->baseVertex;
            command.baseInstance = EncodeBaseInstance(viewportIndex, static_cast<int>(g_instanceData.size()) - 1);
        }
    }

    void CreateDrawCommandsSkinned(std::vector<DrawIndexedIndirectCommand>& commands, std::vector<RenderItem>& renderItems, int viewportIndex, Unloved::Frustum* frustum) {
        // Clear any commands from last frame
        commands.clear();

        // Iterate the viewports and build the draw commands
        int instanceStart = g_instanceData.size();

        // Preallocate an estimate
        g_instanceData.reserve(g_instanceData.size() + renderItems.size());

        // Append new render items to the global instance data
        for (const RenderItem& renderItem : renderItems) {
            if (renderItem.ignoredViewportIndex != -1 && renderItem.ignoredViewportIndex == viewportIndex) continue;
            if (renderItem.exclusiveViewportIndex != -1 && renderItem.exclusiveViewportIndex != viewportIndex) continue;

            // Cull the whole animated object for shadow maps
            if (frustum) {
                uint64_t objectId = 0;
                Hell::Bit::UnpackUint64(renderItem.objectIdLowerBit, renderItem.objectIdUpperBit, objectId);

                AnimatedGameObject* animatedGameObject = World::GetAnimatedGameObjectByObjectId(objectId);
                if (!animatedGameObject) continue;
                if (!frustum->IntersectsAABBFast(animatedGameObject->GetSkinnedAABB())) continue;
            }

            g_instanceData.push_back(renderItem);
        }

        // Create indirect draw commands using the stored offset
        std::span<RenderItem> instanceView(g_instanceData.begin() + instanceStart, g_instanceData.end());
        CreateMultiDrawIndirectCommandsSkinned(commands, instanceView, viewportIndex, instanceStart);
    }

    void CreateDrawCommandsNonDeformingSkinned(std::vector<DrawIndexedIndirectCommand>& commands, std::vector<RenderItem>& renderItems, int viewportIndex, Unloved::Frustum* frustum) {
        // Clear any commands from last frame
        commands.clear();

        // Iterate the viewports and build the draw commands
        int instanceStart = g_instanceData.size();

        // Preallocate an estimate
        g_instanceData.reserve(g_instanceData.size() + renderItems.size());

        // Append new render items to the global instance data
        for (const RenderItem& renderItem : renderItems) {
            if (renderItem.ignoredViewportIndex != -1 && renderItem.ignoredViewportIndex == viewportIndex) continue;
            if (renderItem.exclusiveViewportIndex != -1 && renderItem.exclusiveViewportIndex != viewportIndex) continue;

            // Cull the whole animated object for shadow maps
            if (frustum) {
                uint64_t objectId = 0;
                Hell::Bit::UnpackUint64(renderItem.objectIdLowerBit, renderItem.objectIdUpperBit, objectId);

                AnimatedGameObject* animatedGameObject = World::GetAnimatedGameObjectByObjectId(objectId);
                if (!animatedGameObject) continue;
                if (!frustum->IntersectsAABBFast(animatedGameObject->GetSkinnedAABB())) continue;
            }

            g_instanceData.push_back(renderItem);
        }

        // Create indirect draw commands using the stored offset
        std::span<RenderItem> instanceView(g_instanceData.begin() + instanceStart, g_instanceData.end());
        CreateMultiDrawIndirectCommandsSkinnedNonDeforming(commands, instanceView, viewportIndex, instanceStart);
    }

    void CreateShadowCubeMapMultiDrawIndirectCommands(std::vector<DrawIndexedIndirectCommand>& drawCommands, std::vector<RenderItem>& renderItems, uint32_t faceIndex, Light* light, BlendingMode blendingModeFilter) {
        drawCommands.clear();

        if (!light) return;

        // Get face frustum, bail if invalid
        Unloved::Frustum* frustum = light->GetFrustumByFaceIndex(faceIndex);
        if (!frustum) return;

        // Store the instance offset for this player
        int instanceStart = g_instanceData.size();

        // Preallocate an estimate
        g_instanceData.reserve(g_instanceData.size() + renderItems.size());

        // Append new render items to the global instance data if it's within light frustum
        // renderItems is already sorted by this point
        // but if anything breaks, check here! (maybe you re-ordered things)
        for (const RenderItem& renderItem : renderItems) {
            if ((renderItem.shadowFlags & SHADOW_FLAG_POINT_LIGHT) == 0u) continue;

            if ((BlendingMode)renderItem.blendingMode != blendingModeFilter) {
                continue;
            }

            if (frustum->IntersectsAABBFast(renderItem)) {
                g_instanceData.push_back(renderItem);
            }
        }

        // Create indirect draw commands using the stored offset
        std::span<RenderItem> instanceView(g_instanceData.begin() + instanceStart, g_instanceData.end());
        CreateMultiDrawIndirectCommands(drawCommands, instanceView, -1, instanceStart);
    }

    void CreateMultiDrawIndirectCommands(std::vector<DrawIndexedIndirectCommand>& commands, std::span<RenderItem> renderItems, int viewportIndex, int instanceOffset) {
        commands.reserve(commands.size() + renderItems.size());

        DrawIndexedIndirectCommand* currentCommand = nullptr;
        uint32_t currentMeshId = 0;

        for (const RenderItem& renderItem : renderItems) {
            if (!currentCommand || renderItem.meshId != currentMeshId) {
                currentMeshId = renderItem.meshId;

                currentCommand = &commands.emplace_back();
                SetDrawCommandMeshRange(*currentCommand, renderItem);
                currentCommand->baseInstance = EncodeBaseInstance(viewportIndex, instanceOffset);
                currentCommand->instanceCount = 1;
            }
            else {
                currentCommand->instanceCount++;
            }

            instanceOffset++;
        }
    }

    void CreateMultiDrawIndirectCommandsSkinned(std::vector<DrawIndexedIndirectCommand>& commands, std::span<RenderItem> renderItems, int viewportIndex, int instanceOffset) {
        commands.reserve(renderItems.size());

        for (const RenderItem& renderItem : renderItems) {
            DrawIndexedIndirectCommand command;
            SetDrawCommandMeshRange(command, renderItem);
            command.baseInstance = EncodeBaseInstance(viewportIndex, instanceOffset);
            command.instanceCount = 1;
            commands.push_back(command);
            instanceOffset++;
        }
    }

    void CreateMultiDrawIndirectCommandsSkinnedNonDeforming(std::vector<DrawIndexedIndirectCommand>& commands, std::span<RenderItem> renderItems, int viewportIndex, int instanceOffset) {
        commands.reserve(renderItems.size());

        for (const RenderItem& renderItem : renderItems) {
            DrawIndexedIndirectCommand command;
            SetDrawCommandMeshRange(command, renderItem);
            command.baseInstance = EncodeBaseInstance(viewportIndex, instanceOffset);
            command.instanceCount = 1;
            commands.push_back(command);
            instanceOffset++;
        }
    }

    void CreateSkinningDistpachGroups() {
        constexpr uint32_t SKINNING_WORKGROUP_SIZE = 128;

        g_skinningDispatchGroups.clear();

        // One dispatch group skins one chunk of one skinning job
        for (uint32_t jobIndex = 0; jobIndex < g_skinningJobs.size(); jobIndex++) {
            const SkinningJob& skinningJob = g_skinningJobs[jobIndex];

            // Split the job into fixed size vertex chunks
            for (uint32_t vertexOffset = 0; vertexOffset < skinningJob.vertexCount; vertexOffset += SKINNING_WORKGROUP_SIZE) {
                SkinningDispatchGroup& group = g_skinningDispatchGroups.emplace_back();
                group.jobIndex = jobIndex;
                group.vertexOffset = vertexOffset;
                group.padding0 = 0;
                group.padding1 = 0;
            }
        }
    }

    void CreateSkinningData() {
        ProfilerCPUZone("Skinning commands");

        auto& set = g_drawCommandsSet;

        // Sort render items by mesh index
        SortRenderItems(g_skinnedRenderItemsDefault);
        SortRenderItems(g_skinnedRenderItemsAlphaDiscard);
        SortRenderItems(g_skinnedRenderItemsBlended);
        SortRenderItems(g_skinnedRenderItemsHair);

        SortRenderItems(g_skinnedNonDeformingSkinnedMeshRenderItems);
        SortRenderItems(g_skinnedNonDeformingSkinnedMeshRenderItemsAlphaDiscard);
        SortRenderItems(g_skinnedNonDeformingSkinnedMeshRenderItemsBlended);
        SortRenderItems(g_skinnedNonDeformingSkinnedMeshRenderItemsHair);

        // Create the per viewport draw commands
        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            CreateDrawCommandsSkinned(set.skinnedStandard[i], g_skinnedRenderItemsDefault, i);
            CreateDrawCommandsSkinned(set.skinnedAlphaDiscard[i], g_skinnedRenderItemsAlphaDiscard, i);
            CreateDrawCommandsSkinned(set.skinnedBlended[i], g_skinnedRenderItemsBlended, i);
            CreateDrawCommandsSkinned(set.skinnedHair[i], g_skinnedRenderItemsHair, i);
        }

        // Combine all into a single vector for the compute skinning pass
        g_combinedSkinnedRenderItems.clear();
        g_combinedSkinnedRenderItems.insert(g_combinedSkinnedRenderItems.end(), g_skinnedRenderItemsDefault.begin(), g_skinnedRenderItemsDefault.end());
        g_combinedSkinnedRenderItems.insert(g_combinedSkinnedRenderItems.end(), g_skinnedRenderItemsAlphaDiscard.begin(), g_skinnedRenderItemsAlphaDiscard.end());
        g_combinedSkinnedRenderItems.insert(g_combinedSkinnedRenderItems.end(), g_skinnedRenderItemsBlended.begin(), g_skinnedRenderItemsBlended.end());
        g_combinedSkinnedRenderItems.insert(g_combinedSkinnedRenderItems.end(), g_skinnedRenderItemsHair.begin(), g_skinnedRenderItemsHair.end());

        // Gather all non deforming render items
        //for (AnimatedGameObject& animatedGameObject : LegacyWorld::GetAnimatedGameObjects()) {
		//	if (animatedGameObject.RenderingEnabled()) {
		//		g_skinnedNonDeformingSkinnedMeshRenderItems.insert(g_skinnedNonDeformingSkinnedMeshRenderItems.end(), animatedGameObject.GetNonDeformingRenderItems().begin(), animatedGameObject.GetNonDeformingRenderItems().end());
        //        g_nonDeformingSkinnedMeshRenderItemsDepthPeeledTransparent.insert(g_nonDeformingSkinnedMeshRenderItemsDepthPeeledTransparent.end(), animatedGameObject.GetNonDeformingRenderItemsDepthPeeledTransparent().begin(), animatedGameObject.GetNonDeformingRenderItemsDepthPeeledTransparent().end());
        //    }
        //}

        // Sort by mesh index

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            CreateDrawCommandsNonDeformingSkinned(set.skinnedNonDeformingAlphaDiscard[i], g_skinnedNonDeformingSkinnedMeshRenderItemsAlphaDiscard, i);
            CreateDrawCommandsNonDeformingSkinned(set.skinnedNonDeformingBlended[i], g_skinnedNonDeformingSkinnedMeshRenderItemsBlended, i);
            CreateDrawCommandsNonDeformingSkinned(set.skinnedNonDeformingStandard[i], g_skinnedNonDeformingSkinnedMeshRenderItems, i);
            CreateDrawCommandsNonDeformingSkinned(set.skinnedNonDeformingHair[i], g_skinnedNonDeformingSkinnedMeshRenderItemsHair, i);
        }
    }

    int EncodeBaseInstance(int playerIndex, int instanceOffset) {
        return (playerIndex << VIEWPORT_INDEX_SHIFT) | instanceOffset;
    }

    void DecodeBaseInstance(int baseInstance, int& playerIndex, int& instanceOffset) {
        playerIndex = baseInstance >> VIEWPORT_INDEX_SHIFT;
        instanceOffset = baseInstance & ((1 << VIEWPORT_INDEX_SHIFT) - 1);
    }

    const RendererData& GetRendererData() {
        return g_rendererData;
    }

    const std::vector<ViewportData>& GetViewportData() {
        return g_viewportData;
    }

    const std::vector<RenderItem>& GetRenderItems()             { return g_renderItems; }
    const std::vector<RenderItem>& GetRenderItemsAlphaDiscard() { return g_renderItemsAlphaDiscarded; }
    const std::vector<RenderItem>& GetRenderItemsBlended()      { return g_renderItemsBlended; }
    const std::vector<RenderItem>& GetRenderItemsGlass()        { return g_renderItemsGlass; }
    const std::vector<RenderItem>& GetRenderItemsHair()         { return g_renderItemsHair; }
    const std::vector<RenderItem>& GetRenderItemsMirror()       { return g_renderItemsMirror; }
    const std::vector<RenderItem>& GetRenderItemsOutline()      { return g_renderItemsOutline; }
    const std::vector<RenderItem>& GetRenderItemsPlastic()      { return g_renderItemsPlastic; }
    const std::vector<RenderItem>& GetRenderItemsProcedural()   { return g_renderItemsProcedural; }
    const std::vector<RenderItem>& GetRenderItemsStainedGlass() { return g_renderItemsStainedGlass; }
    const std::vector<RenderItem>& GetRenderItemsToiletWater()  { return g_renderItemsToiletWater; }
    const std::vector<RenderItem>& GetRenderItemsPointLightShadows() { return g_renderItemsPointLightShadows; }

    const std::vector<RenderItem>& GetSkinnedRenderItemsAlphaDiscard() { return g_skinnedRenderItemsAlphaDiscard; }
    const std::vector<RenderItem>& GetSkinnedRenderItemsBlended()      { return g_skinnedRenderItemsBlended; }
    const std::vector<RenderItem>& GetSkinnedRenderItemsDefault()      { return g_skinnedRenderItemsDefault; }
    const std::vector<RenderItem>& GetSkinnedRenderItemsHair()         { return g_skinnedRenderItemsHair; }

    const std::vector<RenderItem>& GetCombinedSkinnedRenderItems() {
        return g_combinedSkinnedRenderItems;
    }

    uint32_t GetRequiredSkinnedVertexCount() {
        return g_baseSkinnedVertex;
    }


    const std::vector<RenderItem>& GetInstanceData() {
        return g_instanceData;
    }

    const std::vector<RenderItem>& GetGlassInstanceData() {
        return g_glassInstanceData;
    }

    const std::vector<GlassLightRange>& GetGlassLightRanges() {
        return g_glassLightRanges;
    }

    const std::vector<uint32_t>& GetGlassLightIndices() {
        return g_glassLightIndices;
    }

    const std::vector<SpriteSheetRenderItem>& GetSpriteSheetInstanceData() {
        return g_spriteSheetInstanceData;
    }

    const DrawCommandsSet& GetDrawInfoSet() {
        return g_drawCommandsSet;
    }

    const FlashLightShadowMapDrawInfo& GetFlashLightShadowMapDrawInfo() {
        return g_flashLightShadowMapDrawInfo;
    }

    const std::vector<GPULight>& GetGPULights() {
        return g_gpuLights;
    }

    const std::vector<glm::mat4>& GetSkinningTransforms() {
        return g_skinningTransforms;
    }

    const std::vector<glm::mat4>& GetPreviousSkinningTransforms() {
        return g_previousSkinningTransforms;
    }

    const std::vector<DecalPaintingInfo>& GetDecalPaintingInfo() {
        return g_decalPaintingInfo;
    }

    const std::vector<BloodDecalInstanceData>& GetBloodScreenSpaceDecalInstanceData() {
        return g_bloodScreenSpaceDecalInstances;
    }

    // Submissions
    void CreateGPULights() {
        g_gpuLights.clear();

        // todo: remove me when u can
        int lightIndex = 0;

        for (Light& light : World::GetLights()) {
            GPULight& gpuLight = g_gpuLights.emplace_back();
            gpuLight.colorR = light.GetColor().r;
            gpuLight.colorG = light.GetColor().g;
            gpuLight.colorB = light.GetColor().b;
            gpuLight.posX = light.GetPosition().x;
            gpuLight.posY = light.GetPosition().y;
            gpuLight.posZ = light.GetPosition().z;
            gpuLight.radius = light.GetRadius();
            gpuLight.strength = light.GetStrength();
            gpuLight.isDirtyForRaytracing = light.IsDirtyForRaytracing();
            gpuLight.hiResShadowMapIndex = ShadowMapManager::GetHiResShadowMapIndex(light.GetObjectId());
            gpuLight.lowResShadowMapIndex = ShadowMapManager::GetLowResShadowMapIndex(light.GetObjectId());
            gpuLight.worldBoundsMin = glm::vec4(light.GetWorldBoundsMin(), 0.0f);
            gpuLight.worldBoundsMax = glm::vec4(light.GetWorldBoundsMax(), 0.0f);

            // todo: remove me when u can
            lightIndex++;

            IESProfile* iesProfile = nullptr;
            switch (light.GetIESProfileType()) {
                case IESProfileType::LAMP_0: iesProfile = Hell::ResourceManager::GetIESProfilePtr("Lamp0"); break;
                case IESProfileType::LAMP_1: iesProfile = Hell::ResourceManager::GetIESProfilePtr("Lamp1"); break;
                case IESProfileType::LAMP_2: iesProfile = Hell::ResourceManager::GetIESProfilePtr("Lamp2"); break;
                case IESProfileType::LAMP_3: iesProfile = Hell::ResourceManager::GetIESProfilePtr("Lamp3"); break;
                case IESProfileType::LAMP_4: iesProfile = Hell::ResourceManager::GetIESProfilePtr("Lamp4"); break;
                case IESProfileType::LAMP_5: iesProfile = Hell::ResourceManager::GetIESProfilePtr("Lamp5"); break;
                case IESProfileType::LAMP_6: iesProfile = Hell::ResourceManager::GetIESProfilePtr("Lamp6"); break;
                case IESProfileType::LAMP_7: iesProfile = Hell::ResourceManager::GetIESProfilePtr("Lamp7"); break;
                case IESProfileType::LAMP_8: iesProfile = Hell::ResourceManager::GetIESProfilePtr("Lamp8"); break;
                case IESProfileType::LAMP_9: iesProfile = Hell::ResourceManager::GetIESProfilePtr("Lamp9"); break;
                case IESProfileType::LAMP_10: iesProfile = Hell::ResourceManager::GetIESProfilePtr("Lamp10"); break;
                case IESProfileType::LAMP_11: iesProfile = Hell::ResourceManager::GetIESProfilePtr("Lamp11"); break;
                default: break;
            }

            if (iesProfile) {
                gpuLight.iesExposure = light.GetIESExposure();
                gpuLight.iesTextureIndex = iesProfile->GetTextureIndex();
                gpuLight.iesVScale = iesProfile->GetVScale();
                gpuLight.iesVBias = iesProfile->GetVBias();
                gpuLight.iesHScale = iesProfile->GetHScale();
                gpuLight.iesHBias = iesProfile->GetHBias();
                gpuLight.iesMaxIntensity = iesProfile->GetMaxIntensity();

                // TODO: move to light init and this WHOLE FUNCTION to Light::CreateGPULight()
                glm::vec3 forward = light.GetForward();
                if (glm::dot(forward, forward) < 0.0001f) {
                    forward = glm::vec3(0.0f, -1.0f, 0.0f); // Default to straight down
                }
                else {
                    forward = glm::normalize(forward);
                }

                float twist = glm::radians(light.GetTwist());
                glm::vec3 tempUp = (std::abs(forward.y) > 0.999f) ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
                glm::vec3 right = glm::normalize(glm::cross(tempUp, forward));
                glm::vec3 up = glm::cross(forward, right);
                glm::vec3 finalRight = right * std::cos(twist) + up * std::sin(twist);
                glm::vec3 finalUp = glm::cross(forward, finalRight);

                gpuLight.forward = forward;
                gpuLight.right = finalRight;
                gpuLight.up = finalUp;
            }
        }
    }

	void SubmitRenderItemProcedural(const RenderItem& renderItem) {
		if (!ValidateRenderItemMeshRange(renderItem, "RenderDataManager::SubmitRenderItemProcedural()")) return;

		g_renderItemsProcedural.push_back(renderItem);

        if (Hell::BackEnd::GetAPI() == API::VULKAN && (renderItem.vulkanFlags & VULKAN_FLAG_EXCLUDE_FROM_TLAS) == 0u) {
            Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");
            Mesh* mesh = meshBuffer.GetMeshById(renderItem.meshId);
            uint64_t vulkanMeshBufferId = meshBuffer.GetVulkanId();
            VulkanMeshBuffer* vulkanMeshBuffer = vulkanMeshBufferId != 0 && VulkanResourceManager::MeshBufferExists(vulkanMeshBufferId) ? VulkanResourceManager::GetMeshBuffer(vulkanMeshBufferId) : nullptr;
            uint64_t vertexBufferDeviceAddress = vulkanMeshBuffer ? vulkanMeshBuffer->GetVertexBufferAddress() : 0;
            uint64_t indexBufferDeviceAddress = vulkanMeshBuffer ? vulkanMeshBuffer->GetIndexBufferAddress() : 0;
            VulkanBuffer* vertexBuffer = vulkanMeshBuffer ? vulkanMeshBuffer->GetVertexBuffer() : nullptr;
            VulkanBuffer* indexBuffer = vulkanMeshBuffer ? vulkanMeshBuffer->GetIndexBuffer() : nullptr;

            if (mesh && vertexBufferDeviceAddress != 0 && indexBufferDeviceAddress != 0 && mesh->vertexCount != 0 && mesh->indexCount >= 3) {
                if (g_proceduralRayQueryBLASId == 0 || !VulkanResourceManager::AccelerationStructureExists(g_proceduralRayQueryBLASId)) {
                    g_proceduralRayQueryBLASId = VulkanResourceManager::CreateAccelerationStructure();
                }

                RayQueryMultiMeshBLAS& multiMeshBLAS = g_rayQueryMultiMeshBLASes.empty() ? g_rayQueryMultiMeshBLASes.emplace_back() : g_rayQueryMultiMeshBLASes[0];
                multiMeshBLAS.vulkanBlasId = g_proceduralRayQueryBLASId;
                multiMeshBLAS.vertexBufferDeviceAddress = vertexBufferDeviceAddress;
                multiMeshBLAS.indexBufferDeviceAddress = indexBufferDeviceAddress;
                multiMeshBLAS.vertexBufferByteSize = vertexBuffer ? vertexBuffer->GetSize() : 0;
                multiMeshBLAS.indexBufferByteSize = indexBuffer ? indexBuffer->GetSize() : 0;
                multiMeshBLAS.sourceGeometryVersion = meshBuffer.GetVersion();
                multiMeshBLAS.modelMatrix = glm::mat4(1.0f);

                RayQueryMeshInstance& meshInstance = multiMeshBLAS.meshInstances.emplace_back();
                meshInstance.mesh.baseVertex = renderItem.baseVertex;
                meshInstance.mesh.baseIndex = renderItem.baseIndex;
                meshInstance.mesh.vertexCount = mesh->vertexCount;
                meshInstance.mesh.indexCount = mesh->indexCount;
                meshInstance.material.blendingMode = renderItem.blendingMode;
                meshInstance.material.materialIndex = renderItem.materialIndex;
                meshInstance.material.shadowBit = renderItem.shadowFlags;
            }
        }
	}

    void SubmitRenderItemsMirror(const std::vector<RenderItem>& renderItems) {
        g_renderItemsMirror.insert(g_renderItemsMirror.begin(), renderItems.begin(), renderItems.end());
    }

    void SubmitDecalPaintingInfo(DecalPaintingInfo decalPaintingInfo) {
        g_decalPaintingInfo.push_back(decalPaintingInfo);
    }

    void SubmitSkinnedRenderItems(const std::vector<RenderItem>& renderItems) {
        g_skinnedRenderItemsDefault.insert(g_skinnedRenderItemsDefault.begin(), renderItems.begin(), renderItems.end());
    }

    void SubmitAnimatedMeshNodes(const AnimatedMeshNodes& animatedMeshNodes) {
        // Get parent
        AnimatedGameObject* animatedGameObject = Unloved::World::GetAnimatedGameObjectByObjectId(animatedMeshNodes.m_parentId);
        if (!animatedGameObject) return;

        if (!animatedMeshNodes.RenderingEnabled()) {
            animatedGameObject->CommitRenderPoseHistory();
            return;
        }

        const std::vector<glm::mat4>& currentSkinningTransforms = animatedGameObject->GetBoneSkinningMatrices();
        const bool hasPreviousSkinningTransforms = animatedGameObject->HasRenderPoseHistory() &&
            animatedGameObject->GetPreviousRenderBoneSkinningMatrices().size() == currentSkinningTransforms.size();
        const std::vector<glm::mat4>& previousSkinningTransforms = hasPreviousSkinningTransforms
            ? animatedGameObject->GetPreviousRenderBoneSkinningMatrices()
            : currentSkinningTransforms;
        const glm::mat4 currentModelMatrix = animatedGameObject->GetModelMatrix();
        const glm::mat4 previousModelMatrix = hasPreviousSkinningTransforms
            ? animatedGameObject->GetPreviousRenderModelMatrix()
            : currentModelMatrix;

        // Cache the base indices before they're mutated
        uint32_t baseSkinningTransformIndex = g_skinningTransforms.size();

        // Append current and previous render-pose matrices at matching offsets.
        g_skinningTransforms.insert(g_skinningTransforms.end(), currentSkinningTransforms.begin(), currentSkinningTransforms.end());
        g_previousSkinningTransforms.insert(g_previousSkinningTransforms.end(), previousSkinningTransforms.begin(), previousSkinningTransforms.end());

        // Ray query group (Vulkan ONLY)
        TransientRayQueryBLASInstance& group = g_transientRayQueryBLASInstances.emplace_back();
        group.modelMatrix = currentModelMatrix;

        for (const AnimatedMeshNode& node : animatedMeshNodes.GetNodes()) {
            RenderItem renderItem = node.renderItem;
            if (node.blendingMode == BlendingMode::DO_NOT_RENDER) continue;

            if (node.excludeFromVulkanTLAS) {
                renderItem.vulkanFlags |= VULKAN_FLAG_EXCLUDE_FROM_TLAS;
            }

            // Deforming
            if (node.deforming) {
                renderItem.modelMatrix = currentModelMatrix;
                renderItem.inverseModelMatrix = glm::inverse(currentModelMatrix);
                renderItem.prevModelMatrix = previousModelMatrix;

                // Vulkan raytracing instance
                if (Hell::BackEnd::GetAPI() == API::VULKAN && !Hell::Bit::Contains(renderItem.vulkanFlags, VULKAN_FLAG_EXCLUDE_FROM_TLAS)) {
                    RayQueryMeshInstance& meshInstance = group.meshInstances.emplace_back();
                    meshInstance.mesh.baseVertex = g_baseSkinnedVertex;
                    meshInstance.mesh.baseIndex = node.baseIndex;
                    meshInstance.mesh.vertexCount = node.vertexCount;
                    meshInstance.mesh.indexCount = node.indexCount;
                    meshInstance.material.blendingMode = static_cast<uint32_t>(node.blendingMode);
                    meshInstance.material.materialIndex = node.materialIndex;
                    meshInstance.material.shadowBit = renderItem.shadowFlags;
                }

                SkinningJob& skinningJob = g_skinningJobs.emplace_back();
                skinningJob.sourceBaseVertex = node.baseVertex;
                skinningJob.sourceBaseIndex = node.baseIndex;
                skinningJob.vertexCount = node.vertexCount;
                skinningJob.indexCount = node.indexCount;
                skinningJob.skinnedBaseVertex = g_baseSkinnedVertex;
                skinningJob.skinningTransformOffset = baseSkinningTransformIndex;
                skinningJob.sourceVertexWeightOffset = node.baseVertexWeight;

                // Assign a new base vertex to be used by compute skinning
                renderItem.baseSkinningTransformIndex = baseSkinningTransformIndex;
                renderItem.baseVertex = g_baseSkinnedVertex;

                g_baseSkinnedVertex += renderItem.vertexCount;

                switch (node.blendingMode) {
                case BlendingMode::DEFAULT:       g_skinnedRenderItemsDefault.push_back(renderItem);      break;
                case BlendingMode::ALPHA_DISCARD: g_skinnedRenderItemsAlphaDiscard.push_back(renderItem); break;
                case BlendingMode::BLENDED:       g_skinnedRenderItemsBlended.push_back(renderItem);      break;
                case BlendingMode::HAIR:          g_skinnedRenderItemsHair.push_back(renderItem);         break;
                default: break;
                }
            }
            // Non deforming
            else {
                Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
                Hell::SkinnedMeshMetadata* metadata = meshBuffer.GetSkinnedMeshMetadataByMeshId(renderItem.meshId);
                const int boneIndex = metadata ? metadata->nonDeformingBoneIndex : -1;

                if (boneIndex >= 0 &&
                    boneIndex < currentSkinningTransforms.size() &&
                    boneIndex < previousSkinningTransforms.size()) {
                    renderItem.modelMatrix = currentModelMatrix * currentSkinningTransforms[boneIndex];
                    renderItem.inverseModelMatrix = glm::inverse(renderItem.modelMatrix);
                    renderItem.prevModelMatrix = previousModelMatrix * previousSkinningTransforms[boneIndex];
                }
                else {
                    renderItem.prevModelMatrix = renderItem.modelMatrix;
                }

                // Vulkan raytracing instance
                if (Hell::BackEnd::GetAPI() == API::VULKAN && !Hell::Bit::Contains(renderItem.vulkanFlags, VULKAN_FLAG_EXCLUDE_FROM_TLAS)) {
                    Mesh* mesh = meshBuffer.GetMeshById(renderItem.meshId);
                    uint64_t vulkanMeshBufferId = meshBuffer.GetVulkanId();
                    VulkanMeshBuffer* vulkanMeshBuffer = vulkanMeshBufferId != 0 && VulkanResourceManager::MeshBufferExists(vulkanMeshBufferId) ? VulkanResourceManager::GetMeshBuffer(vulkanMeshBufferId) : nullptr;
                    uint64_t vertexBufferDeviceAddress = vulkanMeshBuffer ? vulkanMeshBuffer->GetVertexBufferAddress() : 0;
                    uint64_t indexBufferDeviceAddress = vulkanMeshBuffer ? vulkanMeshBuffer->GetIndexBufferAddress() : 0;

                    if (mesh && mesh->vulkanBlasId != 0 && vertexBufferDeviceAddress != 0 && indexBufferDeviceAddress != 0 && mesh->vertexCount != 0 && mesh->indexCount >= 3) {
                        RayQueryBLASInstance& blasInstance = g_rayQueryBLASInstances.emplace_back();
                        blasInstance.vulkanBlasId = mesh->vulkanBlasId;
                        blasInstance.vertexBufferDeviceAddress = vertexBufferDeviceAddress;
                        blasInstance.indexBufferDeviceAddress = indexBufferDeviceAddress;
                        blasInstance.modelMatrix = renderItem.modelMatrix;

                        RayQueryMeshInstance& meshInstance = blasInstance.meshInstance;
                        meshInstance.mesh.baseVertex = renderItem.baseVertex;
                        meshInstance.mesh.baseIndex = renderItem.baseIndex;
                        meshInstance.mesh.vertexCount = mesh->vertexCount;
                        meshInstance.mesh.indexCount = mesh->indexCount;
                        meshInstance.material.blendingMode = renderItem.blendingMode;
                        meshInstance.material.materialIndex = renderItem.materialIndex;
                        meshInstance.material.shadowBit = renderItem.shadowFlags;
                    }
                }

                switch (node.blendingMode) {
                case BlendingMode::ALPHA_DISCARD: g_skinnedNonDeformingSkinnedMeshRenderItemsAlphaDiscard.push_back(renderItem); break;
                case BlendingMode::BLENDED:       g_skinnedNonDeformingSkinnedMeshRenderItemsBlended.push_back(renderItem);      break;
                case BlendingMode::DEFAULT:       g_skinnedNonDeformingSkinnedMeshRenderItems.push_back(renderItem);             break;
                case BlendingMode::HAIR:          g_skinnedNonDeformingSkinnedMeshRenderItemsHair.push_back(renderItem);         break;
                default: break;
                }
            }
        }

        if (group.meshInstances.empty()) {
            g_transientRayQueryBLASInstances.pop_back();
        }

        animatedGameObject->CommitRenderPoseHistory();
    }

    void SubmitMeshNodes(const MeshNodes& meshNodes) {
        for (const MeshNode& node : meshNodes.GetNodes()) {
            RenderItem renderItem = node.renderItem;
            if (node.excludeFromVulkanTLAS) {
                renderItem.vulkanFlags |= VULKAN_FLAG_EXCLUDE_FROM_TLAS;
            }
            SubmitRenderItem(renderItem);
        }
    }

    void SubmitRenderItem(const RenderItem& renderItem) {
        BlendingMode blendingMode = (BlendingMode)renderItem.blendingMode;

        if (blendingMode == BlendingMode::DO_NOT_RENDER) return;
        if (!ValidateRenderItemMeshRange(renderItem, "RenderDataManager::SubmitRenderItem()")) return;

        switch (blendingMode) {
        case BlendingMode::DEFAULT:       g_renderItems.push_back(renderItem); break;
        case BlendingMode::ALPHA_DISCARD: g_renderItemsAlphaDiscarded.push_back(renderItem); break;
        case BlendingMode::BLENDED:       g_renderItemsBlended.push_back(renderItem); break;
        case BlendingMode::GLASS:         g_renderItemsGlass.push_back(renderItem); break;
        case BlendingMode::HAIR:          g_renderItemsHair.push_back(renderItem); break;
        case BlendingMode::MIRROR:        g_renderItemsMirror.push_back(renderItem); break;
        case BlendingMode::TOILET_WATER:  g_renderItemsToiletWater.push_back(renderItem); break;
        //case BlendingMode::STAINED_GLASS: g_renderItemsStainedGlass.push_back(renderItem); break;
        case BlendingMode::STAINED_GLASS: g_renderItemsGlass.push_back(renderItem); break;
        case BlendingMode::PLASTIC:       g_renderItemsPlastic.push_back(renderItem); break;
        default: break;
        }

        uint64_t objectId = 0;
        Hell::Bit::UnpackUint64(renderItem.objectIdLowerBit, renderItem.objectIdUpperBit, objectId);

        // Emissive
        if (Material* material = ResourceManager::GetMaterialByIndex(renderItem.materialIndex)) {
            bool hasEmissiveMap = material->m_emissive != g_blackTextureIndex;
            bool hasEmissiveColor = renderItem.emissiveR != 0.0f || renderItem.emissiveG != 0.0f || renderItem.emissiveB != 0.0f;

            if (hasEmissiveMap || hasEmissiveColor) {
                g_renderItemsEmissive.push_back(renderItem);
            }
        }

        // Outline
        if (objectId != 0 && objectId == Editor::GetSelectedObjectId()) g_renderItemsOutline.push_back(renderItem);

        // Shadow Casting
        if ((renderItem.shadowFlags & SHADOW_FLAG_POINT_LIGHT) != 0u) {
            g_renderItemsPointLightShadows.push_back(renderItem);

            if (Hell::Bit::Contains(renderItem.miscFlags, MISC_FLAG_DYNAMIC_OBJECT)) {
                g_renderItemsDynamicPointLightShadows.push_back(renderItem);
            }
            else {
                g_renderItemsStaticPointLightShadows.push_back(renderItem);
            }
        }
        if ((renderItem.shadowFlags & SHADOW_FLAG_CSM)  != 0u) g_renderItemsMoonLightShadows.push_back(renderItem);

        // Vulkan raytracing instance
        if (Hell::BackEnd::GetAPI() == API::VULKAN && (renderItem.vulkanFlags & VULKAN_FLAG_EXCLUDE_FROM_TLAS) == 0u) {
            Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
            Mesh* mesh = meshBuffer.GetMeshById(renderItem.meshId);
            uint64_t vulkanMeshBufferId = meshBuffer.GetVulkanId();
            VulkanMeshBuffer* vulkanMeshBuffer = vulkanMeshBufferId != 0 && VulkanResourceManager::MeshBufferExists(vulkanMeshBufferId) ? VulkanResourceManager::GetMeshBuffer(vulkanMeshBufferId) : nullptr;
            uint64_t vertexBufferDeviceAddress = vulkanMeshBuffer ? vulkanMeshBuffer->GetVertexBufferAddress() : 0;
            uint64_t indexBufferDeviceAddress = vulkanMeshBuffer ? vulkanMeshBuffer->GetIndexBufferAddress() : 0;

            if (mesh && mesh->vulkanBlasId != 0 && vertexBufferDeviceAddress != 0 && indexBufferDeviceAddress != 0 && mesh->vertexCount != 0 && mesh->indexCount >= 3) {
                RayQueryBLASInstance& blasInstance = g_rayQueryBLASInstances.emplace_back();
                blasInstance.vulkanBlasId = mesh->vulkanBlasId;
                blasInstance.vertexBufferDeviceAddress = vertexBufferDeviceAddress;
                blasInstance.indexBufferDeviceAddress = indexBufferDeviceAddress;
                blasInstance.modelMatrix = renderItem.modelMatrix;

                RayQueryMeshInstance& meshInstance = blasInstance.meshInstance;
                meshInstance.mesh.baseVertex = renderItem.baseVertex;
                meshInstance.mesh.baseIndex = renderItem.baseIndex;
                meshInstance.mesh.vertexCount = mesh->vertexCount;
                meshInstance.mesh.indexCount = mesh->indexCount;
                meshInstance.material.blendingMode = renderItem.blendingMode;
                meshInstance.material.materialIndex = renderItem.materialIndex;
                meshInstance.material.shadowBit = renderItem.shadowFlags;
            }
        }
    }

    void SubmitRenderItems(const std::vector<RenderItem>& renderItems) {
        for (const RenderItem& renderItem : renderItems) {
            SubmitRenderItem(renderItem);
        }
    }

    void SubmitSpriteSheetRenderItem(const SpriteSheetRenderItem& renderItem) {
        g_spriteSheetRenderItems.push_back(renderItem);
    }
}
