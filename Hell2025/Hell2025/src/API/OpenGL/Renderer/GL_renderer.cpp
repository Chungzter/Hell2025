#include "GL_renderer.h"
#include "API/OpenGL/GL_backend.h"
#include "API/OpenGL/GL_Util.h"
#include "API/OpenGL/Types/GL_indirectBuffer.hpp"
#include "API/OpenGL/Types/GL_pbo.hpp"
#include "API/OpenGL/Types/GL_shader.h"
#include "API/OpenGL/Types/GL_ssbo.h"
#include "AssetManagement/AssetManager.h"
#include "BackEnd/BackEnd.h"
#include "Audio/Audio.h"
#include "Core/Game.h"
#include "Config/Config.h"
#include "Input/Input.h"
#include "Ocean/Ocean.h"
#include "Player/Player.h"
#include "Renderer/RenderDataManager.h"
#include "Util/Util.h"
#include "UI/UIBackEnd.h"
#include "UI/TextBlitter.h"
#include "Types/Game/GameObject.h"
#include "../Timer.hpp"
#include <glm/gtx/matrix_decompose.hpp>

#include "Editor/Editor.h"
#include "Editor/Gizmo.h"
#include "Viewport/ViewportManager.h"

#include "API/OpenGL/Types/GL_texture_readback.h"

#include "Hell/Logging.h"
#include <Game/GPUTypes.h>
#include "World/World.h"
#include "Renderer/Renderer.h"
#include <unordered_map>

#define NONE_BIT 0

namespace OpenGLRenderer {

    std::unordered_map<std::string, OpenGLCubemapFrameBuffer> g_cubemapFrameBuffers;
    std::unordered_map<std::string, OpenGLCubemapView> g_cubemapViews;
    std::unordered_map<std::string, OpenGLFrameBuffer> g_frameBuffers;
    std::unordered_map<std::string, OpenGLRasterizerState> g_rasterizerStates;
    std::unordered_map<std::string, OpenGLShader> g_shaders;
    std::unordered_map<std::string, OpenGLShadowCubeMapArray> g_shadowCubeMapArrays;
    std::unordered_map<std::string, OpenGLShadowMap> g_shadowMaps;
    std::unordered_map<std::string, OpenGLShadowMapArray> g_shadowMapArrays;
    std::unordered_map<std::string, OpenGLSSBO> g_ssbos;
    std::unordered_map<std::string, OpenGLTextureArray> g_textureArrays;
    std::unordered_map<std::string, OpenGLTexture3D> g_3dTextures;

    OpenGLShader* g_boundShader = nullptr;

    OpenGLMeshPatch g_tesselationPatch;
    OpenGLFrameBuffer g_blurBuffers[4][4] = {};

    std::vector<float> g_shadowCascadeLevels{ 5.0f, 10.0f, 20.0f, 40.0f };
    const glm::vec3 g_lightDir = glm::normalize(glm::vec3(20.0f, 50, 20.0f));
    unsigned int g_lightFBO;
    unsigned int g_lightDepthMaps;
    constexpr unsigned int g_depthMapResolution = 4096;

    int g_fftDisplayMode = 0;
    int g_fftEditBand = 0;

    GLuint g_emptyVao = 0;
    std::unordered_map<std::string, GLuint> g_cachedTextureHandles;

    void LoadShaders();
    void CreateFrameBuffers();

    IndirectBuffer g_indirectBuffer;

    struct Cubemaps {
        OpenGLCubemapView g_skyboxView;
    } g_cubemaps;

    void ClearRenderTargets();

    int GetFftDisplayMode() {
        return g_fftDisplayMode;
    }

    void Init() {

        Ocean::Init();

        g_3dTextures["PerlinNoise"] = OpenGLTexture3D();
        g_3dTextures["PerlinNoise"].Create(128, GL_R32F, true);

        g_shadowMaps["FlashlightShadowMaps"] = OpenGLShadowMap("FlashlightShadowMaps", FLASHLIGHT_SHADOWMAP_SIZE, FLASHLIGHT_SHADOWMAP_SIZE, 4);

        g_tesselationPatch.Resize2(Ocean::GetTesslationMeshSize().x, Ocean::GetTesslationMeshSize().y);

        CreateFrameBuffers();
        CreateSSBOs();
        InitSSBOs();
        LoadShaders();

        // Allocate shadow map array memory
		g_shadowCubeMapArrays["HiRes"] = OpenGLShadowCubeMapArray();
		g_shadowCubeMapArrays["HiRes"].Init(SHADOWMAP_HI_RES_COUNT, 1024);

        // Moon light shadow maps
        float depthMapResolution = SHADOW_MAP_CSM_SIZE;
        int cascadeCount = int(g_shadowCascadeLevels.size()) + 1;
        int playerCount = 2;
        int layerCount = playerCount * cascadeCount;
        g_shadowMapArrays["MoonlightCSM"] = OpenGLShadowMapArray();
        g_shadowMapArrays["MoonlightCSM"].Init(layerCount, depthMapResolution, GL_DEPTH_COMPONENT32F);

        InitFog();
        InitGrass();
        InitOceanHeightReadback();

		//InitMSAA();
		InitREStyle();
    }

    void InitMain() {
        InitRasterizerStates();

        // Attempt to load skybox
        std::vector<Texture*> textures = {
            AssetManager::GetTextureByName("px"),
            AssetManager::GetTextureByName("nx"),
            AssetManager::GetTextureByName("py"),
            AssetManager::GetTextureByName("ny"),
            AssetManager::GetTextureByName("pz"),
            AssetManager::GetTextureByName("nz"),
        };
        std::vector<GLuint> texturesHandles;
        for (Texture* texture : textures) {
            if (!texture) continue;
            texturesHandles.push_back(texture->GetGLTexture().GetHandle());
        }
        if (texturesHandles.size() == 6) {
            g_cubemapViews["SkyboxNightSky"] = OpenGLCubemapView(texturesHandles);
        }

        CreateBlurBuffers();

        // Upload materials
        std::vector<Material>& materials = AssetManager::GetMaterials();
        std::vector<GPUMaterial> gpuMaterials(materials.size());

        for (int i = 0; i < materials.size(); i++) {
            gpuMaterials[i].basecolor = materials[i].m_basecolor;
            gpuMaterials[i].normal = materials[i].m_normal;
            gpuMaterials[i].rma = materials[i].m_rma;
            gpuMaterials[i].emissive = materials[i].m_emissive;
            gpuMaterials[i].opacity = materials[i].m_opacity;
            gpuMaterials[i].hairMaps = materials[i].m_hairMaps;
        }

        UploadSSBOStatic("Materials", gpuMaterials.size() * sizeof(GPUMaterial), gpuMaterials.data());
    }

    void CreateFrameBuffers() {
        const Resolutions& resolutions = Config::GetResolutions();

        OpenGLCubemapFrameBuffer& lightAABBfbo = CreateCubemapFrameBuffer("LightAABB", 512);
        lightAABBfbo.CreateAttachment(GL_RGBA32F, GL_NEAREST);
        lightAABBfbo.CreateDepthAttachment(GL_DEPTH_COMPONENT32F);

        OpenGLFrameBuffer& gBuffer = CreateFrameBuffer("GBuffer", resolutions.gBuffer);
        gBuffer.CreateAttachment("BaseColorMetallic", GL_RGBA8);
        gBuffer.CreateAttachment("NormalXYRoughnessMisc", GL_RGB10_A2);
        //gBuffer.CreateAttachment("Normal", GL_RGBA16F);
        //gBuffer.CreateAttachment("BaseColor", GL_RGBA8);
        gBuffer.CreateAttachment("RMA", GL_RGBA8); // In alpha is screenspace blood decal mask
        gBuffer.CreateAttachment("Lighting", GL_RGBA16F, GL_LINEAR, GL_LINEAR);
        gBuffer.CreateAttachment("Emissive", GL_RGBA8, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
        gBuffer.CreateAttachment("Glass", GL_RGBA16F);
        gBuffer.CreateAttachment("VelocityXYOcclusionSubSurface", GL_RGBA16F);
        gBuffer.CreateDepthAttachment(GL_DEPTH32F_STENCIL8);

        OpenGLFrameBuffer& scratchFbo = CreateFrameBuffer("Scratch", resolutions.gBuffer);
        scratchFbo.CreateAttachment("RGBA16F", GL_RGBA16F);

        OpenGLFrameBuffer& waterFbo = CreateFrameBuffer("Water", resolutions.gBuffer);
        waterFbo.CreateAttachment("Lighting", GL_RGBA16F);
        waterFbo.CreateAttachment("OceanFlags", GL_R8UI);
        waterFbo.CreateAttachment("OceanMask", GL_R8UI);
        waterFbo.CreateDepthAttachment(GL_DEPTH32F_STENCIL8);

        OpenGLFrameBuffer& emissiveBlurFbo = CreateFrameBuffer("EmissiveBlur", resolutions.gBuffer.x, resolutions.gBuffer.y);
        emissiveBlurFbo.CreateAttachment("ColorA", GL_RGBA8, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, true);
        emissiveBlurFbo.CreateAttachment("ColorB", GL_RGBA8, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, true);

        OpenGLFrameBuffer& indirectDiffuseFbo = CreateFrameBuffer("IndirectDiffuse", resolutions.gBuffer);
        indirectDiffuseFbo.CreateAttachment("Color", GL_R11F_G11F_B10F);

        g_frameBuffers["DepthPeeledTransparency"] = OpenGLFrameBuffer("DepthPeeledTransparency", resolutions.gBuffer);
        g_frameBuffers["DepthPeeledTransparency"].CreateAttachment("Color", GL_RGBA16F);
        g_frameBuffers["DepthPeeledTransparency"].CreateAttachment("ViewspaceDepth", GL_R32F);
        g_frameBuffers["DepthPeeledTransparency"].CreateAttachment("ViewspaceDepthPrevious", GL_R32F);
        g_frameBuffers["DepthPeeledTransparency"].CreateAttachment("Composite", GL_RGBA16F);
        g_frameBuffers["DepthPeeledTransparency"].CreateDepthAttachment(GL_DEPTH32F_STENCIL8);

        //g_frameBuffers["BloodFluid"] = OpenGLFrameBuffer("BloodFluid", resolutions.gBuffer);
        //g_frameBuffers["BloodFluid"].CreateAttachment("Depth", GL_R32F);
        //g_frameBuffers["BloodFluid"].CreateAttachment("Thickness", GL_R32F);
        //g_frameBuffers["BloodFluid"].CreateAttachment("BlurIntermediate", GL_R32F);

        g_frameBuffers["GaussianBlur"] = OpenGLFrameBuffer("GaussianBlur", resolutions.gBuffer.x / 2, resolutions.gBuffer.y / 2);
        g_frameBuffers["GaussianBlur"].CreateAttachment("ColorA", GL_RGBA16F, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
        g_frameBuffers["GaussianBlur"].CreateAttachment("ColorB", GL_RGBA16F, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);

        g_frameBuffers["DecalPainting"] = OpenGLFrameBuffer("DecalPainting", 512, 512);
        g_frameBuffers["DecalPainting"].CreateAttachment("UVMap", GL_RGBA8, GL_LINEAR, GL_LINEAR);
        g_frameBuffers["DecalPainting"].CreateDepthAttachment(GL_DEPTH_COMPONENT24);

        g_textureArrays["WoundMasks"] = OpenGLTextureArray();
        g_textureArrays["WoundMasks"].AllocateMemory(WOUND_MASK_TEXTURE_SIZE, WOUND_MASK_TEXTURE_SIZE, GL_R8, 1, WOUND_MASK_TEXTURE_ARRAY_SIZE); // consider adding mipmaps

        g_frameBuffers["DecalMasks"] = OpenGLFrameBuffer("DecalMasks", WOUND_MASK_TEXTURE_SIZE, WOUND_MASK_TEXTURE_SIZE);

        g_frameBuffers["GBufferBackup"] = OpenGLFrameBuffer("GBufferBackup", resolutions.gBuffer);
        g_frameBuffers["GBufferBackup"].CreateDepthAttachment(GL_DEPTH32F_STENCIL8); // do you really need this? you have WIP below

        g_frameBuffers["WIP"] = OpenGLFrameBuffer("WIP", resolutions.gBuffer);
        g_frameBuffers["WIP"].CreateDepthAttachment(GL_DEPTH32F_STENCIL8);

        g_frameBuffers["Fog"] = OpenGLFrameBuffer("Fog", resolutions.gBuffer / 2);
        g_frameBuffers["Fog"].CreateAttachment("Color", GL_RGBA16F, GL_LINEAR, GL_LINEAR);

        g_frameBuffers["QuarterSize"].Create("QuarterSize", resolutions.gBuffer.x / 4, resolutions.gBuffer.y / 4);
        g_frameBuffers["QuarterSize"].CreateAttachment("DownsampledFinalLighting", GL_RGBA16F);

        g_frameBuffers["HalfSize"].Create("QuarterSize", resolutions.gBuffer.x / 2, resolutions.gBuffer.y / 2);
        g_frameBuffers["HalfSize"].CreateAttachment("DownsampledFinalLighting", GL_RGBA16F, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, true);
        g_frameBuffers["HalfSize"].CreateAttachment("SSRHistoryA", GL_RGBA16F);
        g_frameBuffers["HalfSize"].CreateAttachment("SSRHistoryB", GL_RGBA16F);
        g_frameBuffers["HalfSize"].CreateAttachment("SSRCurrent", GL_RGBA16F);

        g_frameBuffers["MiscFullSize"].Create("MiscFullSize", resolutions.gBuffer);
        g_frameBuffers["MiscFullSize"].CreateAttachment("GaussianFinalLightingIntermediate", GL_RGBA16F);
        g_frameBuffers["MiscFullSize"].CreateAttachment("GaussianFinalLighting", GL_RGBA16F);
        g_frameBuffers["MiscFullSize"].CreateAttachment("ScreenSpaceBloodDecalMask", GL_R8);
        g_frameBuffers["MiscFullSize"].CreateAttachment("ViewspaceDepth", GL_R32F, GL_NEAREST, GL_NEAREST);
        g_frameBuffers["MiscFullSize"].CreateAttachment("FinalLightingCopy", GL_RGBA16F, GL_LINEAR, GL_LINEAR);

        g_frameBuffers["Outline"] = OpenGLFrameBuffer("Outline", resolutions.gBuffer);
        g_frameBuffers["Outline"].CreateAttachment("Mask", GL_R8);
        g_frameBuffers["Outline"].CreateAttachment("Result", GL_R8);

        g_frameBuffers["Hair"] = OpenGLFrameBuffer("Hair", resolutions.hair);
        g_frameBuffers["Hair"].CreateDepthAttachment(GL_DEPTH32F_STENCIL8);
        g_frameBuffers["Hair"].CreateAttachment("Lighting", GL_RGBA16F);
        g_frameBuffers["Hair"].CreateAttachment("ViewspaceDepth", GL_R32F);
        g_frameBuffers["Hair"].CreateAttachment("ViewspaceDepthPrevious", GL_R32F);
        g_frameBuffers["Hair"].CreateAttachment("Composite", GL_RGBA16F);

        g_frameBuffers["FinalImage"] = OpenGLFrameBuffer("FinalImage", resolutions.finalImage);
        g_frameBuffers["FinalImage"].CreateAttachment("Color", GL_RGBA16F);

        g_frameBuffers["Present"] = OpenGLFrameBuffer("Present", resolutions.ui);
        g_frameBuffers["Present"].CreateAttachment("Color", GL_RGBA8, GL_NEAREST, GL_NEAREST);

        g_frameBuffers["World"] = OpenGLFrameBuffer("World", 1, 1);
        g_frameBuffers["World"].CreateAttachment("HeightMap", GL_R16F);

        g_frameBuffers["Road"] = OpenGLFrameBuffer("Road", 1, 1);
        g_frameBuffers["Road"].CreateAttachment("RoadMask", GL_R16F);

        g_frameBuffers["HeightMapBlitBuffer"] = OpenGLFrameBuffer("HeightMapBlitBuffer", HEIGHT_MAP_SIZE, HEIGHT_MAP_SIZE);

        g_frameBuffers["HeightMap"] = OpenGLFrameBuffer("HeightMap", HEIGHT_MAP_SIZE, HEIGHT_MAP_SIZE);
        g_frameBuffers["HeightMap"].CreateAttachment("Color", GL_R16F);

        g_frameBuffers["FFT_band0"].Create("FFT_band0", Ocean::GetFFTResolution(0).x, Ocean::GetFFTResolution(0).y);
        g_frameBuffers["FFT_band0"].CreateAttachment("Displacement", GL_RGBA32F, GL_LINEAR, GL_LINEAR, GL_REPEAT);
        g_frameBuffers["FFT_band0"].CreateAttachment("Normals", GL_RGBA32F, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT, true);

        g_frameBuffers["FFT_band1"].Create("FFT_band1", Ocean::GetFFTResolution(1).x, Ocean::GetFFTResolution(1).y);
        g_frameBuffers["FFT_band1"].CreateAttachment("Displacement", GL_RGBA32F, GL_LINEAR, GL_LINEAR, GL_REPEAT, true);
        g_frameBuffers["FFT_band1"].CreateAttachment("Normals", GL_RGBA32F, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT, true);
    }

    void LoadShaders() {
        LoadShader("ChristmasLightCulling", { "GL_christmas_light_culling.comp" });
        LoadShader("ChristmasLightsWire", { "GL_christmas_light_wire.vert", "GL_christmas_light_wire.frag" });
        LoadShader("BlitRoad", { "GL_blit_road.comp" });
        LoadShader("BlurHorizontal", { "GL_blur_horizontal.vert", "GL_blur.frag" });
        LoadShader("BlurVertical", { "GL_blur_vertical.vert", "GL_blur.frag" });
        LoadShader("ComputeSkinning", { "GL_compute_skinning.comp" });
        LoadShader("TileWorldBounds", { "GL_tile_world_bounds.comp" });

        LoadShader("DownSample2xBox", { "GL_down_sample_2x_box.comp" });
        LoadShader("EditorMesh", { "GL_editor_mesh.vert", "GL_editor_mesh.frag" });
        LoadShader("EmissiveComposite", { "GL_emissive_composite.comp" });
        LoadShader("EmissiveCompositeNew", { "GL_emissive_composite_new.comp" });
        LoadShader("ExamineItem", { "GL_examine_item.vert", "GL_examine_item.frag" });
        LoadShader("FogRayMarch", { "GL_fog_ray_march.comp" });
        LoadShader("FogComposite", { "GL_fog_composite.comp" });
        LoadShader("Fur", { "GL_fur.vert", "GL_fur.frag" });
        LoadShader("FurComposite", { "GL_fur_composite.comp" });
        LoadShader("GBuffer", { "GL_GBuffer.vert", "GL_gBuffer.frag" });
        LoadShader("Gizmo", { "GL_gizmo.vert", "GL_gizmo.frag" });
        LoadShader("Glass", { "GL_glass.vert", "GL_glass.frag" });
        LoadShader("GlassComposite", { "GL_glass_composite.comp" });
        LoadShader("Grass", { "GL_grass.vert", "GL_grass.frag" });
        LoadShader("GrassGeometryGeneration", { "GL_grass_geometry_generation.comp" });
        LoadShader("GrassPositionGeneration", { "GL_grass_position_generation.comp" });
        LoadShader("GaussianBlurUtil", { "GL_gaussian_blur_util.comp" });
        LoadShader("HairDepthPeel", { "GL_hair_depth_peel.vert", "GL_hair_depth_peel.frag" });
        LoadShader("HairFinalComposite", { "GL_hair_final_composite.comp" });
        LoadShader("HairLighting", { "GL_hair_lighting.vert", "GL_hair_lighting.frag" });
        LoadShader("HeightMapColor", { "GL_heightmap_color.vert", "GL_heightmap_color.frag" });
        LoadShader("HeightMapImageGeneration", { "GL_heightmap_image_generation.comp" });
        LoadShader("HeightMapPhysxTextureGeneration", { "GL_heightmap_physx_texture_generation.comp" });
        LoadShader("HeightMapToWorldBlit", { "GL_heightmap_to_world_blit.comp" });
        LoadShader("HeightMapVertexGeneration", { "GL_heightmap_vertex_generation.comp" });
        LoadShader("HeightMapPaint", { "GL_heightmap_paint.comp" });
        LoadShader("LightCulling", { "GL_light_culling.comp" });
        LoadShader("Lighting", { "GL_lighting.comp" });
        LoadShader("CSMLighting", { "GL_lighting.vert", "GL_lighting.frag" });
        LoadShader("GaussianBlur", { "GL_gaussian_blur.comp" }); // am I needed????
        LoadShader("Outline", { "GL_outline.vert", "GL_outline.frag" });
        LoadShader("OutlineComposite", { "GL_outline_composite.comp" });
        LoadShader("OutlineMask", { "GL_outline_mask.vert", "GL_outline_mask.frag" });
        LoadShader("PerlinNoise3D", { "GL_perlin_noise_3d.comp" });
        LoadShader("ShadowMap", { "GL_shadow_map.vert", "GL_shadow_map.frag" });
        LoadShader("ShadowCubeMap", { "GL_shadow_cube_map.vert", "GL_shadow_cube_map.frag" });
        LoadShader("SolidColor", { "GL_solid_color.vert", "GL_solid_color.frag" });
        LoadShader("Skybox", { "GL_skybox.vert", "GL_skybox.frag" });
        LoadShader("SpriteSheet", { "GL_sprite_sheet.vert", "GL_sprite_sheet.frag" });
        LoadShader("ScreenspaceReflections", { "GL_screenspace_reflections.comp" });
        LoadShader("StainedGlass", { "GL_stained_glass.vert", "GL_stained_glass.frag" });
        LoadShader("UI", { "GL_ui.vert", "GL_ui.frag" });
        LoadShader("Winston", { "GL_winston.vert", "GL_winston.frag" });
        LoadShader("CSMDepth", { "GL_csm_depth.vert", "GL_csm_depth.frag", "GL_csm_depth.geom" });
        LoadShader("ZeroOut", { "GL_zero_out.comp" });

        LoadShader("MetaBalls", { "GL_meta_balls.vert", "GL_meta_balls.frag" });
        LoadShader("ViewspaceDepth", { "GL_viewspace_depth.comp" });
        LoadShader("DepthPeeledTransparencyColor", { "GL_depth_peeled_transparency_color.vert", "GL_depth_peeled_transparency_color.frag" });
        LoadShader("DepthPeeledTransparencyDepth", { "GL_depth_peeled_transparency_depth.vert", "GL_depth_peeled_transparency_depth.frag" });
        LoadShader("DepthPeeledTransparencyComposite", { "GL_depth_peeled_transparency_composite.comp" });
        LoadShader("RaytraceScene", { "GL_raytrace_scene.comp" });
        LoadShader("Plastic", { "GL_plastic.vert", "GL_plastic.frag" });

        LoadShader("LightAABBPosition", { "GL_light_aabb_position.vert", "GL_light_aabb_position.frag" });
        LoadShader("LightAABBMinMax", { "GL_light_aabb_min_max.comp" });

        // Blood
        LoadShader("Blood", "BloodDecalsCulling", { "GL_blood_decals_culling.comp" });
        LoadShader("Blood", "BloodDecalsDraw", { "GL_blood_decals_draw.vert", "GL_blood_decals_draw.frag" });
        LoadShader("Blood", "BloodDecalsComposite", { "GL_blood_decals_composite.comp" });
        LoadShader("Blood", "BloodFluidDepth", { "GL_blood_fluid.vert", "GL_blood_fluid_depth.frag" });
        LoadShader("Blood", "BloodFluidThickness", { "GL_blood_fluid.vert", "GL_blood_fluid_thickness.frag" });
        LoadShader("Blood", "BloodFluidBlur", { "GL_blood_fluid_blur.comp" });
        LoadShader("Blood", "VatBlood", { "GL_vat_blood.vert", "GL_vat_blood.frag" });

        // Debug
        LoadShader("Debug", "DebugHackAABB", { "GL_debug_hack_aabb.vert", "GL_debug_hack_aabb.frag" });
        LoadShader("Debug", "DebugLightAABB", { "GL_debug_light_aabb.vert", "GL_debug_light_aabb.frag" });
        LoadShader("Debug", "DebugPointCloud", { "GL_debug_point_cloud.vert", "GL_debug_point_cloud.frag" });
        LoadShader("Debug", "DebugProbes", { "GL_debug_probes.vert", "GL_debug_probes.frag" });
        LoadShader("Debug", "DebugRagdoll", { "GL_debug_ragdoll.vert", "GL_debug_ragdoll.frag" });
        LoadShader("Debug", "DebugSolidColor", { "GL_debug_solid_color.vert", "GL_debug_solid_color.frag" });
        LoadShader("Debug", "DebugTextureBlit", { "GL_debug_texture_blit.vert", "GL_debug_texture_blit.frag" });
        LoadShader("Debug", "DebugTextured", { "GL_debug_textured.vert", "GL_debug_textured.frag" });
        LoadShader("Debug", "DebugTileView", { "GL_debug_tile_view.comp" });
        LoadShader("Debug", "DebugVertex2D", { "GL_debug_vertex_2D.vert", "GL_debug_vertex_2D.frag" });
        LoadShader("Debug", "DebugVertex3D", { "GL_debug_vertex_3D.vert", "GL_debug_vertex_3D.frag" });
		LoadShader("Debug", "DebugView", { "GL_debug_view.comp" });
		LoadShader("Debug", "DebugViewMSAA", { "GL_debug_view.comp" }, { "MSAA_ENABLED" });
		LoadShader("Debug", "DebugViewRE", { "GL_debug_view.comp" }, { "RE_ENABLED" });

        // DDGI
		LoadShader("DDGI", "PointCloudBaseColor", { "GL_point_cloud_basecolor.comp" });
        LoadShader("DDGI", "PointCloudLighting", { "GL_point_cloud_lighting.comp" });
        LoadShader("DDGI", "ProbeDistance", { "GL_probe_distance.comp" });
        LoadShader("DDGI", "ProbeDistanceBorder", { "GL_probe_distance_border.comp" });
        LoadShader("DDGI", "ProbeDistanceDispatchArgs", { "GL_probe_distance_dispatch_args.comp" });
        LoadShader("DDGI", "ProbeDistanceList", { "GL_probe_distance_list.comp" });
        LoadShader("DDGI", "ProbeIrradiance", { "GL_probe_irradiance.comp" });
        LoadShader("DDGI", "ProbeIrradianceBorder", { "GL_probe_irradiance_border.comp" });
        LoadShader("DDGI", "ProbeIrradianceDirtyPointCheck", { "GL_probe_irradiance_dirty_point_check.comp" });
		LoadShader("DDGI", "ProbeIrradianceList", { "GL_probe_irradiance_list.comp" });
        LoadShader("DDGI", "ProbeIrradianceTexture", { "GL_probe_irradiance_texture.comp" });
        LoadShader("DDGI", "ProbeLightingDispatchArgs", { "GL_probe_lighting_dispatch_args.comp" });
        LoadShader("DDGI", "ProbePointIndices", { "GL_probe_point_indices.comp" });
        LoadShader("DDGI", "ProbeRelevance", { "GL_probe_relevance.comp" });
        LoadShader("DDGI", "ProbeRelocation", { "GL_probe_state_update.comp" });
        LoadShader("DDGI", "ProbeStateUpdate", { "GL_probe_state_update.comp" });

        // Decals
        LoadShader("Decals", "DecalPaintUVs", { "gl_decal_paint_uvs.vert", "gl_decal_paint_uvs.frag" });
        LoadShader("Decals", "DecalPaintMask", { "gl_decal_paint_mask.comp" });
        LoadShader("Decals", "Decals", { "GL_decals.vert", "GL_decals.frag" });

        // Ocean
        LoadShader("Water", "FttRadix64Vertical", { "GL_ftt_radix_64_vertical.comp" });
        LoadShader("Water", "FttRadix8Vertical", { "GL_ftt_radix_8_vertical.comp" });
        LoadShader("Water", "FttRadix64Horizontal", { "GL_ftt_radix_64_horizontal.comp" });
        LoadShader("Water", "FttRadix8Horizontal", { "GL_ftt_radix_8_horizontal.comp" });
        LoadShader("Water", "OceanFlags", { "GL_ocean_flags.comp" });
        LoadShader("Water", "OceanSurfaceComposite", { "GL_ocean_surface_composite.comp" });
        LoadShader("Water", "OceanGeometry", { "GL_ocean_geometry.vert", "GL_ocean_geometry.frag", "GL_ocean_geometry.tesc", "GL_ocean_geometry.tese" });
        LoadShader("Water", "OceanCalculateSpectrum", { "GL_ocean_calculate_spectrum.comp" });
        LoadShader("Water", "OceanUpdateTextures", { "GL_ocean_update_textures.comp" });
        LoadShader("Water", "OceanUnderwaterComposite", { "GL_ocean_underwater_composite.comp" });
        LoadShader("Water", "OceanTesseleationEdgeTransitionCleanUp", { "GL_ocean_tessellation_edge_transition_cleanup.comp" });
        LoadShader("Water", "OceanPositionReadback", { "GL_ocean_position_readback.comp" });

        // Post processing
        LoadShader("PostProcessing", "FXAA", { "GL_fxaa.comp" });
        LoadShader("PostProcessing", "TAA", { "GL_taa.comp" });
		LoadShader("PostProcessing", "PostProcessing", { "GL_post_processing.comp" });
    }

    void CreateSSBOs() {
		GLbitfield staticFlags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
		GLbitfield dynamicFlags = GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;

		// Create ssbos

        // Ocean
        const glm::uvec2 oceanSize = Ocean::GetBaseFFTResolution(); // WARNING!!! This size must bit your largest FFT dimensions
		g_ssbos["ffth0Band0"] = OpenGLSSBO(Ocean::GetFFTResolution(0).x * Ocean::GetFFTResolution(0).y * sizeof(std::complex<float>), staticFlags);
		g_ssbos["ffth0Band1"] = OpenGLSSBO(Ocean::GetFFTResolution(1).x * Ocean::GetFFTResolution(1).y * sizeof(std::complex<float>), staticFlags);
		g_ssbos["fftSpectrumInSSBO"] = OpenGLSSBO(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
		g_ssbos["fftSpectrumOutSSBO"] = OpenGLSSBO(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
		g_ssbos["fftDispInXSSBO"] = OpenGLSSBO(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
		g_ssbos["fftDispZInSSBO"] = OpenGLSSBO(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
		g_ssbos["fftGradXInSSBO"] = OpenGLSSBO(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
		g_ssbos["fftGradZInSSBO"] = OpenGLSSBO(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
		g_ssbos["fftDispXOutSSBO"] = OpenGLSSBO(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
		g_ssbos["fftDispZOutSSBO"] = OpenGLSSBO(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
		g_ssbos["fftGradXOutSSBO"] = OpenGLSSBO(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
		g_ssbos["fftGradZOutSSBO"] = OpenGLSSBO(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);

		int dummySize = 64;

        // Core
        g_ssbos["Samplers"] = OpenGLSSBO(sizeof(glm::uvec2), GL_DYNAMIC_STORAGE_BIT);
        g_ssbos["ViewportData"] = OpenGLSSBO(sizeof(ViewportData) * 4, GL_DYNAMIC_STORAGE_BIT);
        g_ssbos["RendererData"] = OpenGLSSBO(sizeof(RendererData), GL_DYNAMIC_STORAGE_BIT);
        g_ssbos["InstanceData"] = OpenGLSSBO(sizeof(RenderItem) * MAX_INSTANCE_DATA_COUNT, GL_DYNAMIC_STORAGE_BIT);
        g_ssbos["SkinningTransforms"] = OpenGLSSBO(sizeof(glm::mat4) * MAX_ANIMATED_TRANSFORMS, GL_DYNAMIC_STORAGE_BIT);
        g_ssbos["Lights"] = OpenGLSSBO(sizeof(GPULight) * MAX_GPU_LIGHTS, GL_DYNAMIC_STORAGE_BIT);

        CreateSSBOStatic("Materials");

        CreateSSBO("RenderItemsUI", dummySize, GL_DYNAMIC_STORAGE_BIT);

        // Vertices
        CreateSSBOStatic("Indices2");
        CreateSSBOStatic("Vertices2");
        CreateSSBOStatic("VertexWeights");

        // Raytracing
		CreateSSBO("TriangleData", dummySize, GL_DYNAMIC_STORAGE_BIT);
		CreateSSBO("SceneBvh", dummySize, GL_DYNAMIC_STORAGE_BIT);
		CreateSSBO("MeshesBvh", dummySize, GL_DYNAMIC_STORAGE_BIT);
		CreateSSBO("EntityInstances", dummySize, GL_DYNAMIC_STORAGE_BIT);
		CreateSSBO("PointGridBuffer", dummySize, GL_DYNAMIC_STORAGE_BIT);
		CreateSSBO("PointIndicesBuffer", dummySize, GL_DYNAMIC_STORAGE_BIT);

		// DDGI
		CreateSSBO("DDGIVolume", sizeof(DDGIVolumeGPU), GL_DYNAMIC_STORAGE_BIT);
		CreateSSBO("DirtyDoorAABBs", sizeof(GPUAABB), GL_DYNAMIC_STORAGE_BIT);
		CreateSSBO("PointCloudGridCounts", dummySize, GL_DYNAMIC_STORAGE_BIT);
		CreateSSBO("PointCloudDirtyFlags", dummySize, GL_DYNAMIC_STORAGE_BIT);
		CreateSSBO("PointCloudGridOffsets", dummySize, GL_DYNAMIC_STORAGE_BIT);
		CreateSSBO("PointCloudTextureInfo", dummySize, GL_DYNAMIC_STORAGE_BIT);
		CreateSSBO("ProbeDistanceCounter", sizeof(uint32_t), GL_DYNAMIC_STORAGE_BIT);
		CreateSSBO("ProbeDistanceDispatchArgs", sizeof(DispatchIndirectCommand), GL_DYNAMIC_STORAGE_BIT);
		CreateSSBO("ProbeDistanceIndices", dummySize, GL_DYNAMIC_STORAGE_BIT);
		CreateSSBO("ProbeIndexCounter", sizeof(uint32_t), GL_DYNAMIC_STORAGE_BIT);
		CreateSSBO("ProbeIrradianceCounter", sizeof(uint32_t), GL_DYNAMIC_STORAGE_BIT);
		CreateSSBO("ProbeIrradianceDispatchArgs", sizeof(DispatchIndirectCommand), GL_DYNAMIC_STORAGE_BIT);
		CreateSSBO("ProbeIrradianceIndices", dummySize, GL_DYNAMIC_STORAGE_BIT);
		CreateSSBO("ProbePointIndices", dummySize, GL_DYNAMIC_STORAGE_BIT);
		CreateSSBO("ProbePointOffsets", dummySize, GL_DYNAMIC_STORAGE_BIT);
		CreateSSBO("ProbePointCounts", dummySize, GL_DYNAMIC_STORAGE_BIT);
        CreateSSBO("ProbeSHColor", dummySize, GL_DYNAMIC_STORAGE_BIT);
		CreateSSBO("ProbeStates", dummySize, GL_DYNAMIC_STORAGE_BIT);

		CreateSSBO("LightAABBs", dummySize, GL_DYNAMIC_STORAGE_BIT);

        // Tile data
		CreateSSBO("TileChristmasLights", GetTileCount() * sizeof(TileInstanceData), NONE_BIT);
		CreateSSBO("TileBloodDecals", GetTileCount() * sizeof(TileInstanceData), NONE_BIT);
		CreateSSBO("TileLights", GetTileCount() * sizeof(TileLights), NONE_BIT);
		CreateSSBO("TileWorldBounds", GetTileCount() * sizeof(TileWorldBounds), NONE_BIT);

        // Instance data
        CreateSSBO("BloodDecalCounter", sizeof(uint32_t), GL_DYNAMIC_STORAGE_BIT);
        CreateSSBO("BloodDecalIndices", sizeof(uint32_t) * GetTileCount() * 256, NONE_BIT);
        g_ssbos["BloodDecalInstances"] = OpenGLSSBO(sizeof(BloodDecalInstanceData) * MAX_SCREEN_SPACE_BLOOD_DECAL_COUNT, GL_DYNAMIC_STORAGE_BIT);
        CreateSSBO("ChristmasLightCounter", sizeof(uint32_t), GL_DYNAMIC_STORAGE_BIT);
        CreateSSBO("ChristmasLightIndices", sizeof(uint32_t) * GetTileCount() * 256, NONE_BIT);
        CreateSSBO("ChristmasLightInstances", MAX_CHRISTMAS_LIGHTS * sizeof(GPUChristmasLight), GL_DYNAMIC_STORAGE_BIT);

        // Remove me at some point
		CreateSSBO("MetaBalls", sizeof(glm::vec4) * 1000, GL_DYNAMIC_STORAGE_BIT);

		int MAX_OCEAN_PATCHES = 500;
		CreateSSBO("OceanPatchTransforms", sizeof(glm::mat4) * MAX_OCEAN_PATCHES, GL_DYNAMIC_STORAGE_BIT);

		// Preallocate the indirect command buffer
		g_indirectBuffer.PreAllocate(sizeof(DrawIndexedIndirectCommand) * MAX_INDIRECT_DRAW_COMMAND_COUNT);
    }

    void InitSSBOs() {
        //DispatchIndirectCommand command = { 1, 1, 1 };
        //UpdateSSBO("ProbeDispatchArgs", sizeof(DispatchIndirectCommand), &command);

        // HO
        const std::vector<std::complex<float>>& h0Band0 = Ocean::GetH0(0);
        const std::vector<std::complex<float>>& h0Band1 = Ocean::GetH0(1);
        g_ssbos["ffth0Band0"].CopyFrom(h0Band0.data(), sizeof(std::complex<float>) * h0Band0.size());
        g_ssbos["ffth0Band1"].CopyFrom(h0Band1.data(), sizeof(std::complex<float>) * h0Band1.size());

    }

    void UpdateSSBOS() {
        UpdateSSBO("Samplers", sizeof(GLuint64) * OpenGLBackEnd::GetBindlessTextureIDs().size(), OpenGLBackEnd::GetBindlessTextureIDs().data());

        const RendererData& rendererData = RenderDataManager::GetRendererData();
        const std::vector<BloodDecalInstanceData>& screenSpaceBloodDecalInstances = RenderDataManager::GetScreenSpaceBloodDecalInstanceData();
        const std::vector<GPULight>& gpuLightsHighRes = RenderDataManager::GetGPULightsHighRes();
        const std::vector<RenderItem>& instanceData = RenderDataManager::GetInstanceData();
        const std::vector<ViewportData>& playerData = RenderDataManager::GetViewportData();
        const std::vector<glm::mat4>&oceanPatchTransforms = RenderDataManager::GetOceanPatchTransforms();

        GLuint zero = 0;

        UpdateSSBO("BloodDecalCounter", sizeof(uint32_t), &zero);
        UpdateSSBO("BloodDecalInstances", screenSpaceBloodDecalInstances.size() * sizeof(BloodDecalInstanceData), screenSpaceBloodDecalInstances.data());
        UpdateSSBO("ChristmasLightCounter", sizeof(uint32_t), &zero);
        UpdateSSBO("InstanceData", instanceData.size() * sizeof(RenderItem), instanceData.data());
        UpdateSSBO("Lights", gpuLightsHighRes.size() * sizeof(GPULight), gpuLightsHighRes.data());
        UpdateSSBO("RendererData", sizeof(RendererData), (void*)&rendererData);
        UpdateSSBO("ViewportData", playerData.size() * sizeof(ViewportData), playerData.data());
        UpdateSSBO("OceanPatchTransforms", oceanPatchTransforms.size() * sizeof(glm::mat4), oceanPatchTransforms.data());

        const std::vector<RenderItemUI>& renderItemsUI = UIBackEnd::GetRenderItems();
        UpdateSSBO("RenderItemsUI", renderItemsUI.size() * sizeof(RenderItemUI), renderItemsUI.data());

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        BindSSBO(0, "Samplers");
        BindSSBO(1, "RendererData");
        BindSSBO(2, "ViewportData");
        BindSSBO(3, "InstanceData");
        BindSSBO(4, "Lights");
    }

    void PreGameLogicComputePasses() {
        PaintHeightMap();
    }


    void RenderDebugHackAABB() {
        static GLuint vao = 0;
        if (vao == 0) {
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);
        }

        OpenGLFrameBuffer* gBuffer = GetFrameBufferOLD("GBuffer");
        gBuffer->Bind();
        gBuffer->DrawBuffer("Lighting");

        BindShader("DebugHackAABB");
        glBindVertexArray(vao);

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGLRenderer::SetViewport(gBuffer, viewport);
                SetUniformMat4("u_projectionView", RenderDataManager::GetViewportData()[i].projectionView);
                glDrawArrays(GL_LINE_STRIP, 0, 16);
            }
        }
    }


    void RenderGame() {
        ProfilerOpenGLFrame();

		if (Renderer::GetRendererMode() == RendererMode::RE_STYLE) {
			RenderGameREStyle();
			return;
		}


        ComputeOceanFFTPass();
        OceanHeightReadback();

        OpenGLFrameBuffer& hairFrameBuffer = g_frameBuffers["Hair"];
        DDGIVolume& ddgiVolume = World::GetTestDDGIVolume();

        glDisable(GL_DITHER);

        if (Input::KeyPressed(HELL_KEY_N)) {
            Audio::PlayAudio(AUDIO_SELECT, 1.00f);
            FlipNormalMapY();
        }

        //BlitRoads();

        ComputeSkinningPass();
        ClearRenderTargets();

        UpdateSSBOS();
        RenderShadowMaps();
        SkyBoxPass();
        HeightMapPass();


        DecalPaintingPass();
        HouseGeometryPass();
        GeometryPass();
        GrassPass();
        MirrorGeometryPass();
        WeatherBoardsPass();
        VatBloodPass();

        ComputeTileWorldBounds();
        ChristmasLightCullingPass();
        LightCullingPass();

        BloodDecalsPass();
        ComputeViewspaceDepth();

        // GI
        UpdateGlobalIllumintation();

        BindSSBO(0, "Samplers");
        BindSSBO(1, "RendererData");
        BindSSBO(2, "ViewportData");
        BindSSBO(3, "InstanceData");
        BindSSBO(4, "Lights");
        BindSSBO(5, "TileLights");
        BindSSBO(6, "TileWorldBounds");

        BindSSBO(10, "ProbeSHColor");
        BindSSBO(11, "ProbeStates");

        LightingPass();

        //FurPass();
        OceanGeometryPass();
        OceanUnderWaterFlags();
        OceanSurfaceCompositePass();

        GlassPass();
        EmissivePass();
        ScreenspaceReflectionsPass();
        HairPass();
        //DepthPeeledTransparencyPass();
        PlasticPass();
        RayMarchFog();
        GaussianBlur();
        OceanUnderwaterCompositePass();
        StainedGlassPass();
        WinstonPass();
        SpriteSheetPass(); // Muzzle flash, etc
        InventoryGaussianPass();

        // Disabling lighting actually just clears it, that way you don't have fog and shit everywhere
        if (!Renderer::GetCurrentRendererSettings().enableLighting) {
            OpenGLFrameBuffer& gBuffer = GetFrameBuffer("GBuffer");
            gBuffer.Bind();
            gBuffer.ClearAttachment("Lighting", 0, 0, 0, 0);
        }

        if (Renderer::GetCurrentRendererSettings().debugDrawPointCloud)       DrawPointCloud(ddgiVolume);
        if (Renderer::GetCurrentRendererSettings().debugDrawPointCloudGrid)   DrawPointCloudGrid(ddgiVolume);
        if (Renderer::GetCurrentRendererSettings().debugDrawIrradianceProbes) DrawProbes(ddgiVolume);

        PostProcessingPass();

        DebugViewPass();
        DebugPass();

        ExamineItemPass();
        EditorPass();
        OutlinePass();

        //DownSampleFinalImage();

        //if (Input::KeyDown(HELL_KEY_U)) {
        //    OpenGLFrameBuffer* bloodFluidFbo = GetFrameBuffer("BloodFluid");
        //    OpenGLRenderer::BlitFrameBuffer(bloodFluidFbo, &finalImageBuffer, "Depth", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);
        //}
        //if (Input::KeyDown(HELL_KEY_Y)) {
        //    OpenGLFrameBuffer* bloodFluidFbo = GetFrameBuffer("BloodFluid");
        //    OpenGLRenderer::BlitFrameBuffer(bloodFluidFbo, &finalImageBuffer, "Thickness", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);
        //}
        //if (Input::KeyDown(HELL_KEY_T)) {
        //    OpenGLFrameBuffer* bloodFluidFbo = GetFrameBuffer("BloodFluid");
        //    OpenGLRenderer::BlitFrameBuffer(bloodFluidFbo, &finalImageBuffer, "BlurIntermediate", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);
        //}

        //BlitFog();

        OpenGLFrameBuffer& finalImageFbo = GetFrameBuffer("FinalImage");
        OpenGLFrameBuffer& gBufferRE = GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer& presentFbo = GetFrameBuffer("Present");

        // Downscale with linear filtering
        OpenGLRenderer::BlitFrameBuffer(&gBufferRE, &finalImageFbo, "Lighting", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);

        // Upscale with nearest filtering
        OpenGLRenderer::BlitFrameBuffer(&finalImageFbo, &presentFbo, "Color", "Color", GL_COLOR_BUFFER_BIT, GL_NEAREST);

        UIPass();

        // Blit to swap chain
        OpenGLRenderer::BlitToDefaultFrameBuffer(&presentFbo, "Color", GL_COLOR_BUFFER_BIT, GL_NEAREST);

        ImGuiPass();
        BlitDebugTextures();

        // DEBUG RENDER FFT TEXTURES TO THE SCREEN
        if (Input::KeyPressed(HELL_KEY_5)) {
            g_fftDisplayMode = 1;
            g_fftEditBand = 0;
        }
        if (Input::KeyPressed(HELL_KEY_6)) {
            g_fftDisplayMode = 2;
            g_fftEditBand = 1;
        }
        if (Input::KeyPressed(HELL_KEY_7)) {
            g_fftDisplayMode = 0;
        }
    }

    void ClearRenderTargets() {
        glDepthMask(GL_TRUE);

        // Water
        OpenGLFrameBuffer& waterFrameBuffer = GetFrameBuffer("Water");
        waterFrameBuffer.Bind();
        waterFrameBuffer.ClearAttachment("Lighting", 0, 0, 0, 0);
        waterFrameBuffer.ClearAttachmentUI("OceanFlags", 0, 0, 0, 0);
        waterFrameBuffer.ClearAttachmentUI("OceanMask", 0, 0, 0, 0);

        // GBuffer
        OpenGLFrameBuffer& gBuffer = GetFrameBuffer("GBuffer");
        gBuffer.Bind();
        gBuffer.ClearAttachment("BaseColor", 0, 0, 0, 0);
        gBuffer.ClearAttachment("Normal", 0, 0, 0, 0);
        gBuffer.ClearAttachment("RMA", 0, 0, 0, 0);
        gBuffer.ClearAttachment("Emissive", 0, 0, 0, 0);
        gBuffer.ClearAttachment("Glass", 0, 0, 0, 0);
        gBuffer.ClearAttachment("VelocityXYOcclusionSubSurface", 0, 0, 0, 1);
        gBuffer.ClearDepthAttachment(0.0f);

        // Decal mask
        OpenGLFrameBuffer& miscFullSizeFBO = GetFrameBuffer("MiscFullSize");
        miscFullSizeFBO.Bind();
        miscFullSizeFBO.ClearTexImage("ScreenSpaceBloodDecalMask", 0, 0, 0, 0);
    }

    void MultiDrawIndirect(const std::vector<DrawIndexedIndirectCommand>& commands) {
        if (commands.size()) {
            // Feed the draw command data to the gpu
            g_indirectBuffer.Bind();
            g_indirectBuffer.Update(sizeof(DrawIndexedIndirectCommand) * commands.size(), commands.data());

            // Fire of the commands
            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (GLvoid*)0, (GLsizei)commands.size(), 0);
        }
    }

    void SplitMultiDrawIndirect(OpenGLShader* shader, const std::vector<DrawIndexedIndirectCommand>& commands, bool bindMaterial, bool bindWoundMaterial) {
        if (!shader) {
            Logging::Fatal() << "SplitMultiDrawIndirect(..) was called with nullptr shader\n";
            return;
        }

        const std::vector<RenderItem>& instanceData = RenderDataManager::GetInstanceData();

        for (const DrawIndexedIndirectCommand& command : commands) {
            int viewportIndex = command.baseInstance >> VIEWPORT_INDEX_SHIFT;
            int instanceOffset = command.baseInstance & ((1 << VIEWPORT_INDEX_SHIFT) - 1);

            for (GLuint i = 0; i < command.instanceCount; ++i) {
                const RenderItem& renderItem = instanceData[instanceOffset + i];

                shader->SetInt("u_viewportIndex", viewportIndex);
                shader->SetInt("u_globalInstanceIndex", instanceOffset + i);

                if (bindMaterial) {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByBindlessIndex(renderItem.baseColorTextureIndex)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByBindlessIndex(renderItem.normalMapTextureIndex)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByBindlessIndex(renderItem.rmaTextureIndex)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE3);

                    // Try bind emissive texture
                    if (renderItem.emissiveTextureIndex != -1) {
                        if (Texture* texture = AssetManager::GetTextureByBindlessIndex(renderItem.emissiveTextureIndex)) {
                            glBindTexture(GL_TEXTURE_2D, texture->GetGLTexture().GetHandle());
                        }
                    }
                    // Fall back to black
                    else {
                        glBindTexture(GL_TEXTURE_2D, GetTextureHandleByName("Black"));
                    }
                }
                if (bindWoundMaterial) {
                    glActiveTexture(GL_TEXTURE4);
                    glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByBindlessIndex(renderItem.additionalTextureIndex0)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE5);
                    glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByBindlessIndex(renderItem.additionalTextureIndex1)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE6);
                    glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByBindlessIndex(renderItem.additionalTextureIndex2)->GetGLTexture().GetHandle());
                }

                glDrawElementsBaseVertex(GL_TRIANGLES, command.indexCount, GL_UNSIGNED_INT, (GLvoid*)(command.firstIndex * sizeof(GLuint)), command.baseVertex);
            }
        }
    }

    void HotloadShaders() {
        // Reset the currently bound shader
        g_boundShader = nullptr;

        std::string failedShaders = "FAILED TO HOTLOAD";

        bool allSucceeded = true;
        for (auto& [_, shader] : g_shaders) {
            if (!shader.Hotload()) {
				allSucceeded = false;
				failedShaders += "\n- ";
				for (const std::string& path : shader.GetPaths()) {
                    failedShaders += path + " ";
                }
            }
        }
        if (allSucceeded) {
			std::cout << "Hotloaded shaders\n";
            Debug::BlitQuickDebugMessage("HOTLOADED SHADERS\n");
		}
		else {
			Debug::BlitQuickDebugMessage(failedShaders);
        }
    }

    void DrawQuad() {
        static Mesh* mesh = AssetManager::GetMeshByModelNameMeshName("Primitives", "Quad");
        glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
    }

    void DebugHack(const std::string& message) {

    }

    void CreateBlurBuffers() {
        const Resolutions& resolutions = Config::GetResolutions();

        // Iterate each viewport
        for (int x = 0; x < 4; x++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(x);

            // Start the first blur buffer at the full viewport dimensions
            SpaceCoords spaceCoords = viewport->GetGBufferSpaceCoords();
            float width = spaceCoords.width;
            float height = spaceCoords.height;

            // Create framebuffers, downscale by 50% each time
            for (int y = 0; y < 4; y++) {

                // Clean up existing framebuffer
                g_blurBuffers[x][y].Create("BlurBuffer", (int)width, (int)height);
                g_blurBuffers[x][y].CreateAttachment("ColorA", GL_RGBA8, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
                g_blurBuffers[x][y].CreateAttachment("ColorB", GL_RGBA8, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
                width *= 0.5f;
                height *= 0.5f;
            }
        }
    }

    void BindEmptyVAO() {
        if (g_emptyVao == 0) glGenVertexArrays(1, &g_emptyVao);
        glBindVertexArray(g_emptyVao);
    }

	void MultiDrawPerViewport(OpenGLFrameBuffer* fbo, OpenGLShader* shader, const std::vector<DrawIndexedIndirectCommand> drawCommands[4], OpenGLRasterizerState& rasterizerState) {
		SetRasterizerState(rasterizerState);

		for (int i = 0; i < 4; i++) {
			Viewport* viewport = ViewportManager::GetViewportByIndex(i);
			if (viewport->IsVisible()) {
				OpenGLRenderer::SetViewport(fbo, viewport);
				if (BackEnd::RenderDocFound()) {
					SplitMultiDrawIndirect(shader, drawCommands[i], true, false);
				}
				else {
					MultiDrawIndirect(drawCommands[i]);
				}
			}
		}
	}

    void MultiDrawPerViewportRE(OpenGLFrameBuffer& fbo, const std::vector<DrawIndexedIndirectCommand> drawCommands[4], OpenGLRasterizerState& rasterizerState) {
        SetRasterizerState(rasterizerState);

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGLRenderer::SetViewport(&fbo, viewport);
                MultiDrawIndirect(drawCommands[i]);
            }
        }
    }

	void MultiDrawPerViewport(OpenGLFrameBuffer& fbo, OpenGLShader& shader, const std::vector<DrawIndexedIndirectCommand> drawCommands[4], OpenGLRasterizerState& rasterizerState) {
		SetRasterizerState(rasterizerState);

		for (int i = 0; i < 4; i++) {
			Viewport* viewport = ViewportManager::GetViewportByIndex(i);
			if (viewport->IsVisible()) {
				OpenGLRenderer::SetViewport(&fbo, viewport);
				if (BackEnd::RenderDocFound()) {
					SplitMultiDrawIndirect(&shader, drawCommands[i], true, false);
				}
				else {
					MultiDrawIndirect(drawCommands[i]);
				}
			}
		}
	}

    void DispatchCompute(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ) {
        glDispatchCompute(groupsX, groupsY, groupsZ);
    }

    void DispatchComputeIndirect() {
        glDispatchComputeIndirect(0);
    }

    void LoadShader(const std::string& name, const std::vector<std::string>& shaderPaths, const std::vector<std::string>& defines) {
        const auto [it, inserted] = g_shaders.try_emplace(name, shaderPaths, "", defines);
        if (!inserted) {
            Logging::Error() << "Renderer::LoadShader() failed: '" << name << "' already exists\n";
        }
    }

	void LoadShader(const std::string& subDirectory, const std::string& name, const std::vector<std::string>& shaderPaths, const std::vector<std::string>& defines) {
		const auto [it, inserted] = g_shaders.try_emplace(name, shaderPaths, subDirectory, defines);
		if (!inserted) {
			Logging::Error() << "Renderer::LoadShader() failed: '" << name << "' already exists\n";
		}
	}

    void BindShader(const std::string& name) {
        OpenGLShader* shader = GetShaderOLD(name);

        if (!shader) return;

        // You commented this out because if you do any shader->bind() elsewhere it breaks this tracker
        //if (g_boundShader && shader == g_boundShader) return;

        g_boundShader = shader;
        g_boundShader->Bind();
    }

    void SetUniformBool(const std::string& name, bool value) {
		if (g_boundShader) {
			g_boundShader->SetBool(name, value);
		}
    }

    void SetUniformInt(const std::string& name, int value) {
        if (g_boundShader) {
            g_boundShader->SetInt(name, value);
        }
    }

    void SetUniformUInt(const std::string& name, uint32_t value) {
        if (g_boundShader) {
            g_boundShader->SetUInt(name, value);
        }
    }

    void SetUniformFloat(const std::string& name, float value) {
        if (g_boundShader) {
            g_boundShader->SetFloat(name, value);
        }
    }

    void SetUniformVec2(const std::string& name, const glm::vec2& value) {
        if (g_boundShader) {
            g_boundShader->SetVec2(name, value);
        }
    }

    void SetUniformVec3(const std::string& name, const glm::vec3& value) {
        if (g_boundShader) {
            g_boundShader->SetVec3(name, value);
        }
    }

    void SetUniformIVec3(const std::string& name, const glm::ivec3& value) {
        if (g_boundShader) {
            g_boundShader->SetIVec3(name, value);
        }
    }

    void SetUniformUVec3(const std::string& name, const glm::uvec3& value) {
        if (g_boundShader) {
            g_boundShader->SetUVec3(name, value);
        }
    }

    void SetUniformVec4(const std::string& name, const glm::vec4& value) {
        if (g_boundShader) {
            g_boundShader->SetVec4(name, value);
        }
    }

    void SetUniformMat4(const std::string& name, const glm::mat4& value) {
        if (g_boundShader) {
            g_boundShader->SetMat4(name, value);
        }
    }

    void CreateSSBO(const std::string& name, size_t size, GLbitfield flags) {
        const auto [it, inserted] = g_ssbos.try_emplace(name, size, flags);
        if (!inserted) {
            Logging::Error() << "Renderer::CreateSSBO() failed: '" << name << "' already exists\n";
        }
    }


    void CreateSSBOStatic(const std::string& name) {
        const auto [it, inserted] = g_ssbos.try_emplace(name);
        if (!inserted) {
            Logging::Error() << "Renderer::CreateSSBO() failed: '" << name << "' already exists\n";
        }
    }

    void BindSSBO(unsigned int bindingIndex, const std::string& name) {
        if (OpenGLSSBO* ssbo = GetSSBO(name)) {
            ssbo->Bind(bindingIndex);
        }
    }

    void BindSSBO(unsigned int bindingIndex, uint32_t vboHandle) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingIndex, vboHandle);
    }

    void ReserveSSBO(const std::string& name, size_t size) {
        if (OpenGLSSBO* ssbo = GetSSBO(name)) {
            ssbo->Reserve(size);
        }
    }

    void ClearSSBO(const std::string& name) {
        if (OpenGLSSBO* ssbo = GetSSBO(name)) {
            ssbo->Clear();
        }
    }

    void ClearSSBORange(const std::string& name, size_t offset, size_t size) {
        if (OpenGLSSBO* ssbo = GetSSBO(name)) {
            ssbo->ClearRange(offset, size);
        }
    }

    void BindDispatchBuffer(const std::string& name) {
        if (OpenGLSSBO* ssbo = GetSSBO(name)) {
            glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, ssbo->GetHandle());
        }
    }

    void BindDrawIndirectBuffer(const std::string& name) {
        if (OpenGLSSBO* ssbo = GetSSBO(name)) {
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, ssbo->GetHandle());
        }
    }

    void BindImageTexture(uint32_t bindingIndex, uint32_t textureHandle, uint32_t access, uint32_t format, bool layered) {
        glBindImageTexture(static_cast<GLuint>(bindingIndex), static_cast<GLuint>(textureHandle), 0, layered, 0, static_cast<GLenum>(access), static_cast<GLenum>(format));
    }

    void BindImageTextureArray(uint32_t bindingIndex, uint32_t textureHandle, uint32_t access, uint32_t format) {
        glBindImageTexture(static_cast<GLuint>(bindingIndex), static_cast<GLuint>(textureHandle), 0, GL_TRUE, 0, static_cast<GLenum>(access), static_cast<GLenum>(format));
    }

    void BindTextureUnit(uint32_t bindingIndex, uint32_t textureHandle) {
        glBindTextureUnit(static_cast<GLuint>(bindingIndex), static_cast<GLuint>(textureHandle));
    }

    GLuint GetTextureHandleByName(const std::string& name) {
        if (auto it = g_cachedTextureHandles.find(name); it != g_cachedTextureHandles.end()) {
            return it->second;
        }

        Texture* texture = AssetManager::GetTextureByName(name);
        if (!texture) {
            Logging::Fatal() << "OpenGLRenderer::GetTextureHandleByName() failed because '" << name << "' does not exist\n";
            return 0;
        }

        const GLuint textureHandle = texture->GetGLTexture().GetHandle();
        g_cachedTextureHandles.emplace(name, textureHandle);
        return textureHandle;
    }

    OpenGLMeshPatch* GetOceanMeshPatch() {
        return &g_tesselationPatch;
    }

    OpenGLShader& GetShader(const std::string& name) {
        static OpenGLShader invald;

        auto it = g_shaders.find(name);
        if (it == g_shaders.end()) {
            Logging::Error() << "Renderer::GetShader() failed to get '" << name << "'\n";
            return invald;
        }

        return it->second;
    }

    OpenGLShader* GetShaderOLD(const std::string& name) {
        auto it = g_shaders.find(name);
        if (it == g_shaders.end()) {
            Logging::Error() << "Renderer::GetShader() failed to get '" << name << "'\n";
            return nullptr;
        }
        return &it->second;
    }

    OpenGLFrameBuffer& CreateFrameBuffer(const std::string& name, glm::ivec2 resolution) {
        return CreateFrameBuffer(name, resolution.x, resolution.y);
    }

    OpenGLFrameBuffer& CreateFrameBuffer(const std::string& name, int32_t width, int32_t height) {
        auto it = g_frameBuffers.find(name);

        if (it != g_frameBuffers.end()) {
            it->second.CleanUp();
            it->second = OpenGLFrameBuffer(name, width, height);
            return it->second;
        }

        auto result = g_frameBuffers.emplace(name, OpenGLFrameBuffer(name, width, height));
        return result.first->second;
    }

    OpenGLFrameBuffer& CreateMultisampledFrameBuffer(const std::string& name, glm::ivec2 resolution, uint32_t sampleCount) {
        return CreateMultisampledFrameBuffer(name, resolution.x, resolution.y, sampleCount);
    }

    OpenGLFrameBuffer& CreateMultisampledFrameBuffer(const std::string& name, int32_t width, int32_t height, uint32_t sampleCount) {
        auto it = g_frameBuffers.find(name);

        if (it != g_frameBuffers.end()) {
            it->second.CleanUp();
            it->second = OpenGLFrameBuffer(name, width, height, sampleCount);
            return it->second;
        }

        auto result = g_frameBuffers.emplace(name, OpenGLFrameBuffer(name, width, height, sampleCount));
        return result.first->second;
    }

    OpenGLFrameBuffer* GetFrameBufferOLD(const std::string& name) {
        auto it = g_frameBuffers.find(name);
        if (it == g_frameBuffers.end()) {
            Logging::Error() << "Renderer::GetFrameBuffer() failed to get '" << name << "'\n";
            return nullptr;
        }
        return &it->second;
    }

    OpenGLFrameBuffer& GetFrameBuffer(const std::string& name) {
        static OpenGLFrameBuffer invalid;

        auto it = g_frameBuffers.find(name);
        if (it == g_frameBuffers.end()) {
            Logging::Error() << "Renderer::GetFrameBuffer() failed to get '" << name << "'\n";
            return invalid;
        }
        return it->second;
    }

    OpenGLCubemapFrameBuffer& CreateCubemapFrameBuffer(const std::string& name, int32_t size) {
        auto it = g_cubemapFrameBuffers.find(name);

        if (it != g_cubemapFrameBuffers.end()) {
            Logging::Warning() << "Renderer::CreateCubemapFrameBuffer(): '" << name << "' overwritten.\n";
            it->second.CleanUp();
            it->second = OpenGLCubemapFrameBuffer(name, size);
            return it->second;
        }

        auto result = g_cubemapFrameBuffers.emplace(name, OpenGLCubemapFrameBuffer(name, size));
        return result.first->second;
    }

    OpenGLCubemapFrameBuffer& GetCubemapFrameBuffer(const std::string& name) {
        static OpenGLCubemapFrameBuffer invalid;

        auto it = g_cubemapFrameBuffers.find(name);
        if (it == g_cubemapFrameBuffers.end()) {
            Logging::Error() << "Renderer::GetCubemapFrameBuffer() failed: '" << name << "' not found. Returning null buffer.\n";
            return invalid;
        }
        return it->second;
    }

    OpenGLShadowMap* GetShadowMapOLD(const std::string& name) {
        auto it = g_shadowMaps.find(name);
        if (it == g_shadowMaps.end()) {
            Logging::Error() << "Renderer::GetShadowMap() failed to get '" << name << "'\n";
            return nullptr;
        }
        return &it->second;
    }

	OpenGLShadowMap& GetShadowMap(const std::string& name) {
        static OpenGLShadowMap invalid;

		auto it = g_shadowMaps.find(name);
		if (it == g_shadowMaps.end()) {
			Logging::Error() << "Renderer::GetShadowMap() failed to get '" << name << "'\n";
			return invalid;
		}
		return it->second;
	}

	OpenGLShadowCubeMapArray* GetShadowCubeMapArrayOLD(const std::string& name) {
		auto it = g_shadowCubeMapArrays.find(name);
		if (it == g_shadowCubeMapArrays.end()) {
			Logging::Error() << "Renderer::GetShadowCubeMapArray() failed to get '" << name << "'\n";
			return nullptr;
		}
		return &it->second;
	}

	OpenGLShadowCubeMapArray& GetShadowCubeMapArray(const std::string& name) {
        static OpenGLShadowCubeMapArray invalid;

        auto it = g_shadowCubeMapArrays.find(name);
		if (it == g_shadowCubeMapArrays.end()) {
			Logging::Error() << "Renderer::GetShadowCubeMapArray() failed to get '" << name << "'\n";
			return invalid;
		}
		return it->second;
	}

	OpenGLShadowMapArray* GetShadowMapArrayOLD(const std::string& name) {
		auto it = g_shadowMapArrays.find(name);
		if (it == g_shadowMapArrays.end()) {
			Logging::Error() << "Renderer::GetShadowMapArray() failed to get '" << name << "'\n";
			return nullptr;
		}
		return &it->second;
	}

	OpenGLShadowMapArray& GetShadowMapArray(const std::string& name) {
        static OpenGLShadowMapArray invalid;

		auto it = g_shadowMapArrays.find(name);
		if (it == g_shadowMapArrays.end()) {
			Logging::Error() << "Renderer::GetShadowMapArray() failed to get '" << name << "'\n";
			return invalid;
		}
		return it->second;
	}

    OpenGLTextureArray* GetTextureArray(const std::string& name) {
        auto it = g_textureArrays.find(name);
        if (it == g_textureArrays.end()) {
            Logging::Error() << "Renderer::GetTextureArray() failed to get '" << name << "'\n";
            return nullptr;
        }
        return &it->second;
    }

    OpenGLTexture3D* GetTexture3D(const std::string& name) {
        auto it = g_3dTextures.find(name);
        if (it == g_3dTextures.end()) {
            Logging::Error() << "Renderer::GetTexture3D() failed to get '" << name << "'\n";
            return nullptr;
        }
        return &it->second;
    }

    OpenGLFrameBuffer* GetBlurBuffer(int viewportIndex, int bufferIndex) {
        if (viewportIndex < 0 || viewportIndex >= 4 || bufferIndex < 0 || bufferIndex >= 4) {
            Logging::Error() << "Renderer::GetBlurBuffer() failed to get indices [" << viewportIndex << "][" << bufferIndex << "]\n";
            return nullptr;
        }
        return &g_blurBuffers[viewportIndex][bufferIndex];
    }

    OpenGLSSBO* GetSSBO(const std::string& name) {
        auto it = g_ssbos.find(name);
        if (it == g_ssbos.end()) {
            Logging::Error() << "Renderer::GetSSBO() failed to get '" << name << "'\n";
            return nullptr;
        }
        return &it->second;
    }

    OpenGLCubemapView& GetCubemapView(const std::string& name) {
        static OpenGLCubemapView invalid;

        auto it = g_cubemapViews.find(name);
        if (it == g_cubemapViews.end()) {
            Logging::Error() << "Renderer::GetCubemapView() failed to get '" << name << "'\n";
            return invalid;
        }
        return it->second;
    }

    OpenGLCubemapView* GetCubemapViewOLD(const std::string& name) {
        auto it = g_cubemapViews.find(name);
        if (it == g_cubemapViews.end()) {
            Logging::Error() << "Renderer::GetCubemapViewOLD() failed to get '" << name << "'\n";
            return nullptr;
        }
        return &it->second;
    }

    OpenGLRasterizerState* GetRasterizerState(const std::string& name) {
        auto it = g_rasterizerStates.find(name);
        if (it == g_rasterizerStates.end()) {
            Logging::Error() << "Renderer::GetRasterizerState() failed to get '" << name << "'\n";
            return nullptr;
        }
        return &it->second;
    }

    OpenGLRasterizerState* CreateRasterizerState(const std::string& name) {
        g_rasterizerStates[name] = OpenGLRasterizerState();
        return &g_rasterizerStates[name];
    }

    void CleanUp() {
        if (g_emptyVao != 0) {
            glDeleteVertexArrays(1, &g_emptyVao);
            g_emptyVao = 0;
        }
    }

    std::vector<float>& GetShadowCascadeLevels() {
        return g_shadowCascadeLevels;
    }

    void UpdateSSBO(const std::string& name, size_t size, const void* data) {
        OpenGLSSBO* ssbo = GetSSBO(name);
        if (ssbo && size > 0) {
            ssbo->Update(size, data);
        }
    }

    void UploadSSBOStatic(const std::string& name, size_t size, const void* data) {
        OpenGLSSBO* ssbo = GetSSBO(name);
        if (ssbo && size > 0) {
            ssbo->UploadStatic(size, data);
        }
    }

    void EditorRasterizerStateOverride() {
        if (Editor::IsOpen() && Editor::BackfaceCullingDisabled()) {
            glDisable(GL_CULL_FACE);
        }
    }

    const std::string& GetZoneNames() {
        return ProfilerOpenGLZoneNames();
    }

    const std::string& GetZoneGPUTimings() {
        return ProfilerOpenGLGpuTimings();
    }

    const std::string& GetZoneCPUTimings() {
        return ProfilerOpenGLCpuTimings();
    }

    const std::string& GetTotalGPUTime() {
        return ProfilerOpenGLTotalGPU();
    }

    const std::string& GetTotalCPUTime() {
        return ProfilerOpenGLTotalCPU();
    }

    uint32_t GetTileCount() { return Renderer::GetTileCount(); }
	uint32_t GetTileCountX() { return Renderer::GetTileCountX(); }
	uint32_t GetTileCountY() { return Renderer::GetTileCountY(); }
}
