#include "GL_renderer.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Hell/Render/API/OpenGL/GL_rasterizer_state_manager.h"
#include "Hell/Render/API/OpenGL/GL_resource_manager.h"
#include "Hell/Render/API/OpenGL/GL_util.h"
#include "Hell/Render/API/OpenGL/Types/GL_indirectBuffer.hpp"
#include "Hell/Render/API/OpenGL/Types/GL_pbo.hpp"
#include "Hell/Render/API/OpenGL/Types/GL_shader.h"
#include "Hell/Render/API/OpenGL/Types/GL_ssbo.h"
#include "Hell/Backend/BackEnd.h"
#include "Hell/Audio.h"
namespace Audio = Hell::Audio;
#include "Unloved/Session/Session.h"
#include "Unloved/Config/Config.h"
#include "Unloved/Systems/Ocean/Ocean.h"
#include "Unloved/Player/Player.h"
#include "Renderer/RenderDataManager.h"
#include "Util/Util.h"
#include "Hell/UI/UIBackEnd.h"
#include "Hell/UI/TextBlitter.h"
#include "Unloved/Objects/Props/GameObject.h"
#include "../Timer.hpp"

#include "Unloved/Editor/Editor.h"
#include "Unloved/Editor/Gizmo.h"
#include "Unloved/Viewport/ViewportManager.h"

#include "Hell/Render/API/OpenGL/Types/GL_texture_readback.h"

#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "World/LegacyWorld.h"
#include "Renderer/Renderer.h"
#include <unordered_map>
#include "Hell/Input.h"
namespace Input = Hell::Input;


#define NONE_BIT 0

namespace OpenGLRenderer {
    using namespace Unloved;


    OpenGLMeshPatch g_tesselationPatch;

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

        uint64_t perlinNoiseId = OpenGL::ResourceManager::CreateTexture3D("PerlinNoise");
        OpenGLTexture3D& perlinNoise = OpenGL::ResourceManager::GetTexture3DById(perlinNoiseId);
        perlinNoise.Create(128, GL_R32F, true);

        uint64_t flashlightShadowMapsId = OpenGL::ResourceManager::CreateShadowMap("FlashlightShadowMaps");
        OpenGL::ResourceManager::GetShadowMapById(flashlightShadowMapsId) = OpenGLShadowMap("FlashlightShadowMaps", FLASHLIGHT_SHADOWMAP_SIZE, FLASHLIGHT_SHADOWMAP_SIZE, 4);

        g_tesselationPatch.Resize2(Ocean::GetTesslationMeshSize().x, Ocean::GetTesslationMeshSize().y);

        CreateFrameBuffers();
        CreateSSBOs();
        InitSSBOs();
        LoadShaders();

        OpenGLRasterizerState* decalPass = OpenGLRasterizerStateManager::CreateRasterizerState("DecalPass");
        decalPass->depthTestEnabled = true;
        decalPass->blendEnable = true;
        decalPass->cullfaceEnable = true;
        decalPass->depthMask = false;
        decalPass->depthFunc = GL_GREATER;
        decalPass->blendFuncSrcfactor = GL_SRC_ALPHA;
        decalPass->blendFuncDstfactor = GL_ONE_MINUS_SRC_ALPHA;

        OpenGLRasterizerState* emissivePass = OpenGLRasterizerStateManager::CreateRasterizerState("EmissivePass");
        emissivePass->depthTestEnabled = true;
        emissivePass->blendEnable = false;
        emissivePass->cullfaceEnable = true;
        emissivePass->depthMask = false;
        emissivePass->depthFunc = GL_GREATER;

        OpenGLRasterizerState* geometryPassDefault = OpenGLRasterizerStateManager::CreateRasterizerState("GeometryPass_Default");
        geometryPassDefault->depthTestEnabled = true;
        geometryPassDefault->blendEnable = false;
        geometryPassDefault->cullfaceEnable = true;
        geometryPassDefault->depthMask = true;
        geometryPassDefault->depthFunc = GL_GREATER;

        OpenGLRasterizerState* geometryPassAlphaDiscard = OpenGLRasterizerStateManager::CreateRasterizerState("GeometryPass_AlphaDiscard");
        geometryPassAlphaDiscard->depthTestEnabled = true;
        geometryPassAlphaDiscard->blendEnable = false;
        geometryPassAlphaDiscard->cullfaceEnable = true;
        geometryPassAlphaDiscard->depthMask = true;
        geometryPassAlphaDiscard->depthFunc = GL_GEQUAL;

        OpenGLRasterizerState* geometryPassBlended = OpenGLRasterizerStateManager::CreateRasterizerState("GeometryPass_Blended");
        geometryPassBlended->depthTestEnabled = true;
        geometryPassBlended->blendEnable = true;
        geometryPassBlended->cullfaceEnable = false;
        geometryPassBlended->depthMask = false;
        geometryPassBlended->depthFunc = GL_GEQUAL;
        geometryPassBlended->blendFuncSrcfactor = GL_SRC_ALPHA;
        geometryPassBlended->blendFuncDstfactor = GL_ONE_MINUS_SRC_ALPHA;

        OpenGLRasterizerState* glassPass = OpenGLRasterizerStateManager::CreateRasterizerState("GlassPass");
        glassPass->depthTestEnabled = true;
        glassPass->blendEnable = false;
        glassPass->cullfaceEnable = true;
        glassPass->depthMask = false;
        glassPass->depthFunc = GL_GREATER;

        OpenGLRasterizerState* hairPassViewspaceDepth = OpenGLRasterizerStateManager::CreateRasterizerState("HairViewspaceDepth");
        hairPassViewspaceDepth->depthTestEnabled = true;
        hairPassViewspaceDepth->blendEnable = false;
        hairPassViewspaceDepth->cullfaceEnable = true;
        hairPassViewspaceDepth->depthMask = true;
        hairPassViewspaceDepth->depthFunc = GL_GREATER;
        hairPassViewspaceDepth->blendFuncSrcfactor = GL_SRC_ALPHA;
        hairPassViewspaceDepth->blendFuncDstfactor = GL_ONE_MINUS_SRC_ALPHA;
        hairPassViewspaceDepth->pointSize = 8;

        OpenGLRasterizerState* hairPassLighting = OpenGLRasterizerStateManager::CreateRasterizerState("HairLighting");
        hairPassLighting->depthTestEnabled = true;
        hairPassLighting->blendEnable = false;
        hairPassLighting->cullfaceEnable = true;
        hairPassLighting->depthMask = true;
        hairPassLighting->depthFunc = GL_EQUAL;
        hairPassLighting->blendFuncSrcfactor = GL_SRC_ALPHA;
        hairPassLighting->blendFuncDstfactor = GL_ONE_MINUS_SRC_ALPHA;
        hairPassLighting->pointSize = 8;

        OpenGLRasterizerState* spriteSheet = OpenGLRasterizerStateManager::CreateRasterizerState("SpriteSheetPass");
        spriteSheet->depthTestEnabled = true;
        spriteSheet->blendEnable = true;
        spriteSheet->cullfaceEnable = false;
        spriteSheet->depthMask = false;
        spriteSheet->depthFunc = GL_GREATER;
        spriteSheet->blendFuncSrcfactor = GL_SRC_ALPHA;
        spriteSheet->blendFuncDstfactor = GL_ONE; // was GL_ONE_MINUS_SRC_ALPHA

        OpenGLRasterizerState* skybox = OpenGLRasterizerStateManager::CreateRasterizerState("SkyBox");
        skybox->depthTestEnabled = false;
        skybox->blendEnable = false;
        skybox->cullfaceEnable = false;
        skybox->depthMask = false;
        skybox->depthFunc = GL_GREATER;

        // Allocate shadow map array memory
		uint64_t hiResShadowMapsId = OpenGL::ResourceManager::CreateShadowCubeMapArray("HiRes");
		OpenGLShadowCubeMapArray& hiResShadowMaps = OpenGL::ResourceManager::GetShadowCubeMapArrayById(hiResShadowMapsId);
        if (hiResShadowMaps.GetHandle() != 0) {
            hiResShadowMaps.CleanUp();
        }
		hiResShadowMaps.Init(SHADOWMAP_HI_RES_COUNT, 1024);

        // Moon light shadow maps
        float depthMapResolution = SHADOW_MAP_CSM_SIZE;
        int cascadeCount = int(g_shadowCascadeLevels.size()) + 1;
        int playerCount = 2;
        int layerCount = playerCount * cascadeCount;
        uint64_t moonlightCSMId = OpenGL::ResourceManager::CreateShadowMapArray("MoonlightCSM");
        OpenGLShadowMapArray& moonlightCSM = OpenGL::ResourceManager::GetShadowMapArrayById(moonlightCSMId);
        if (moonlightCSM.GetHandle() != 0) {
            moonlightCSM.CleanUp();
        }
        moonlightCSM.Init(layerCount, depthMapResolution, GL_DEPTH_COMPONENT32F);

        InitFog();
        InitGrass();
        InitOceanHeightReadback();

		//InitMSAA();
		InitREStyle();
    }

    void InitMain() {
        // Attempt to load skybox
        std::vector<Texture*> textures = {
            Hell::ResourceManager::GetTextureByName("px"),
            Hell::ResourceManager::GetTextureByName("nx"),
            Hell::ResourceManager::GetTextureByName("py"),
            Hell::ResourceManager::GetTextureByName("ny"),
            Hell::ResourceManager::GetTextureByName("pz"),
            Hell::ResourceManager::GetTextureByName("nz"),
        };
        std::vector<GLuint> texturesHandles;
        for (Texture* texture : textures) {
            if (!texture) continue;
            texturesHandles.push_back(texture->GetGLTexture().GetHandle());
        }
        if (texturesHandles.size() == 6) {
            uint64_t skyboxNightSkyId = OpenGL::ResourceManager::CreateCubemapView("SkyboxNightSky");
            OpenGL::ResourceManager::GetCubemapViewById(skyboxNightSkyId).CreateCubemap(texturesHandles);
        }

        CreateBlurBuffers();

        // Upload materials
        std::vector<Material>& materials = Hell::ResourceManager::GetMaterials();
        OpenGL::UploadSSBOStatic("Materials", materials.size() * sizeof(Material), materials.data());
    }

    void CreateFrameBuffers() {
        const Resolutions& resolutions = Config::GetResolutions();

        OpenGLCubemapFrameBuffer& lightAABBfbo = OpenGL::ResourceManager::CreateCubemapFrameBuffer("LightAABB");
        lightAABBfbo.Create(512);
        lightAABBfbo.CreateAttachment(GL_RGBA32F, GL_NEAREST);
        lightAABBfbo.CreateDepthAttachment(GL_DEPTH_COMPONENT32F);

        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::CreateFrameBuffer("GBuffer");
        gBuffer.Create(resolutions.gBuffer);
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

        OpenGLFrameBuffer& scratchFbo = OpenGL::ResourceManager::CreateFrameBuffer("Scratch");
        scratchFbo.Create(resolutions.gBuffer);
        scratchFbo.CreateAttachment("RGBA16F", GL_RGBA16F);

        OpenGLFrameBuffer& waterFbo = OpenGL::ResourceManager::CreateFrameBuffer("Water");
        waterFbo.Create(resolutions.gBuffer);
        waterFbo.CreateAttachment("Lighting", GL_RGBA16F);
        waterFbo.CreateAttachment("OceanFlags", GL_R8UI);
        waterFbo.CreateAttachment("OceanMask", GL_R8UI);
        waterFbo.CreateDepthAttachment(GL_DEPTH32F_STENCIL8);

        OpenGLFrameBuffer& emissiveBlurFbo = OpenGL::ResourceManager::CreateFrameBuffer("EmissiveBlur");
        emissiveBlurFbo.Create(resolutions.gBuffer.x, resolutions.gBuffer.y);
        emissiveBlurFbo.CreateAttachment("ColorA", GL_RGBA8, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, true);
        emissiveBlurFbo.CreateAttachment("ColorB", GL_RGBA8, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, true);

        OpenGLFrameBuffer& indirectDiffuseFbo = OpenGL::ResourceManager::CreateFrameBuffer("IndirectDiffuse");
        indirectDiffuseFbo.Create(resolutions.gBuffer);
        indirectDiffuseFbo.CreateAttachment("Color", GL_R11F_G11F_B10F);

        OpenGLFrameBuffer& depthPeeledTransparencyFbo = OpenGL::ResourceManager::CreateFrameBuffer("DepthPeeledTransparency");
        depthPeeledTransparencyFbo.Create(resolutions.gBuffer);
        depthPeeledTransparencyFbo.CreateAttachment("Color", GL_RGBA16F);
        depthPeeledTransparencyFbo.CreateAttachment("ViewspaceDepth", GL_R32F);
        depthPeeledTransparencyFbo.CreateAttachment("ViewspaceDepthPrevious", GL_R32F);
        depthPeeledTransparencyFbo.CreateAttachment("Composite", GL_RGBA16F);
        depthPeeledTransparencyFbo.CreateDepthAttachment(GL_DEPTH32F_STENCIL8);

        //OpenGLFrameBuffer& bloodFluidFbo = CreateFrameBuffer("BloodFluid", resolutions.gBuffer);
        //bloodFluidFbo.CreateAttachment("Depth", GL_R32F);
        //bloodFluidFbo.CreateAttachment("Thickness", GL_R32F);
        //bloodFluidFbo.CreateAttachment("BlurIntermediate", GL_R32F);

        OpenGLFrameBuffer& gaussianBlurFbo = OpenGL::ResourceManager::CreateFrameBuffer("GaussianBlur");
        gaussianBlurFbo.Create(resolutions.gBuffer.x / 2, resolutions.gBuffer.y / 2);
        gaussianBlurFbo.CreateAttachment("ColorA", GL_RGBA16F, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
        gaussianBlurFbo.CreateAttachment("ColorB", GL_RGBA16F, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);

        OpenGLFrameBuffer& decalPaintingFbo = OpenGL::ResourceManager::CreateFrameBuffer("DecalPainting");
        decalPaintingFbo.Create(512, 512);
        decalPaintingFbo.CreateAttachment("UVMap", GL_RGBA8, GL_LINEAR, GL_LINEAR);
        decalPaintingFbo.CreateDepthAttachment(GL_DEPTH_COMPONENT24);

        Hell::TextureArray& woundMasks = Hell::ResourceManager::CreateTextureArray("WoundMasks");
        woundMasks.CleanUp();
        woundMasks.AllocateMemory(WOUND_MASK_TEXTURE_SIZE, WOUND_MASK_TEXTURE_SIZE, GL_R8, 1, WOUND_MASK_TEXTURE_ARRAY_SIZE); // consider adding mipmaps

        OpenGL::ResourceManager::CreateFrameBuffer("DecalMasks").Create(WOUND_MASK_TEXTURE_SIZE, WOUND_MASK_TEXTURE_SIZE);

        OpenGLFrameBuffer& gBufferBackupFbo = OpenGL::ResourceManager::CreateFrameBuffer("GBufferBackup");
        gBufferBackupFbo.Create(resolutions.gBuffer);
        gBufferBackupFbo.CreateDepthAttachment(GL_DEPTH32F_STENCIL8); // do you really need this? you have WIP below

        OpenGLFrameBuffer& wipFbo = OpenGL::ResourceManager::CreateFrameBuffer("WIP");
        wipFbo.Create(resolutions.gBuffer);
        wipFbo.CreateDepthAttachment(GL_DEPTH32F_STENCIL8);

        OpenGLFrameBuffer& fogFbo = OpenGL::ResourceManager::CreateFrameBuffer("Fog");
        fogFbo.Create(resolutions.gBuffer / 2);
        fogFbo.CreateAttachment("Color", GL_RGBA16F, GL_LINEAR, GL_LINEAR);

        OpenGLFrameBuffer& quarterSizeFbo = OpenGL::ResourceManager::CreateFrameBuffer("QuarterSize");
        quarterSizeFbo.Create(resolutions.gBuffer.x / 4, resolutions.gBuffer.y / 4);
        quarterSizeFbo.CreateAttachment("DownsampledFinalLighting", GL_RGBA16F);

        OpenGLFrameBuffer& halfSizeFbo = OpenGL::ResourceManager::CreateFrameBuffer("HalfSize");
        halfSizeFbo.Create(resolutions.gBuffer.x / 2, resolutions.gBuffer.y / 2);
        halfSizeFbo.CreateAttachment("DownsampledFinalLighting", GL_RGBA16F, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, true);
        halfSizeFbo.CreateAttachment("SSRHistoryA", GL_RGBA16F);
        halfSizeFbo.CreateAttachment("SSRHistoryB", GL_RGBA16F);
        halfSizeFbo.CreateAttachment("SSRCurrent", GL_RGBA16F);

        OpenGLFrameBuffer& miscFullSizeFbo = OpenGL::ResourceManager::CreateFrameBuffer("MiscFullSize");
        miscFullSizeFbo.Create(resolutions.gBuffer);
        miscFullSizeFbo.CreateAttachment("GaussianFinalLightingIntermediate", GL_RGBA16F);
        miscFullSizeFbo.CreateAttachment("GaussianFinalLighting", GL_RGBA16F);
        miscFullSizeFbo.CreateAttachment("BloodScreenSpaceDecalMask", GL_R8);
        miscFullSizeFbo.CreateAttachment("ViewspaceDepth", GL_R32F, GL_NEAREST, GL_NEAREST);
        miscFullSizeFbo.CreateAttachment("FinalLightingCopy", GL_RGBA16F, GL_LINEAR, GL_LINEAR);

        OpenGLFrameBuffer& outlineFbo = OpenGL::ResourceManager::CreateFrameBuffer("Outline");
        outlineFbo.Create(resolutions.gBuffer);
        outlineFbo.CreateAttachment("Mask", GL_R8);
        outlineFbo.CreateAttachment("Result", GL_R8);

        OpenGLFrameBuffer& hairFbo = OpenGL::ResourceManager::CreateFrameBuffer("Hair");
        hairFbo.Create(resolutions.hair);
        hairFbo.CreateDepthAttachment(GL_DEPTH32F_STENCIL8);
        hairFbo.CreateAttachment("Lighting", GL_RGBA16F);
        hairFbo.CreateAttachment("ViewspaceDepth", GL_R32F);
        hairFbo.CreateAttachment("ViewspaceDepthPrevious", GL_R32F);
        hairFbo.CreateAttachment("Composite", GL_RGBA16F);

        OpenGLFrameBuffer& finalImageFbo = OpenGL::ResourceManager::CreateFrameBuffer("FinalImage");
        finalImageFbo.Create(resolutions.finalImage);
        finalImageFbo.CreateAttachment("Color", GL_RGBA16F);

        OpenGLFrameBuffer& presentFbo = OpenGL::ResourceManager::CreateFrameBuffer("Present");
        presentFbo.Create(resolutions.ui);
        presentFbo.CreateAttachment("Color", GL_RGBA8, GL_NEAREST, GL_NEAREST);

        OpenGLFrameBuffer& worldFbo = OpenGL::ResourceManager::CreateFrameBuffer("World");
        worldFbo.Create(1, 1);
        worldFbo.CreateAttachment("HeightMap", GL_R16F);

        OpenGLFrameBuffer& roadFbo = OpenGL::ResourceManager::CreateFrameBuffer("Road");
        roadFbo.Create(1, 1);
        roadFbo.CreateAttachment("RoadMask", GL_R16F);

        OpenGL::ResourceManager::CreateFrameBuffer("HeightMapBlitBuffer").Create(HEIGHT_MAP_SIZE, HEIGHT_MAP_SIZE);

        OpenGLFrameBuffer& heightMapFbo = OpenGL::ResourceManager::CreateFrameBuffer("HeightMap");
        heightMapFbo.Create(HEIGHT_MAP_SIZE, HEIGHT_MAP_SIZE);
        heightMapFbo.CreateAttachment("Color", GL_R16F);

        OpenGLFrameBuffer& fftFrameBufferBand0 = OpenGL::ResourceManager::CreateFrameBuffer("FFT_band0");
        fftFrameBufferBand0.Create(Ocean::GetFFTResolution(0).x, Ocean::GetFFTResolution(0).y);
        fftFrameBufferBand0.CreateAttachment("Displacement", GL_RGBA32F, GL_LINEAR, GL_LINEAR, GL_REPEAT);
        fftFrameBufferBand0.CreateAttachment("Normals", GL_RGBA32F, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT, true);

        OpenGLFrameBuffer& fftFrameBufferBand1 = OpenGL::ResourceManager::CreateFrameBuffer("FFT_band1");
        fftFrameBufferBand1.Create(Ocean::GetFFTResolution(1).x, Ocean::GetFFTResolution(1).y);
        fftFrameBufferBand1.CreateAttachment("Displacement", GL_RGBA32F, GL_LINEAR, GL_LINEAR, GL_REPEAT, true);
        fftFrameBufferBand1.CreateAttachment("Normals", GL_RGBA32F, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT, true);
    }

    void LoadShaders() {
        OpenGL::ResourceManager::LoadShader("ChristmasLightCulling", { "GL_christmas_light_culling.comp" });
        OpenGL::ResourceManager::LoadShader("ChristmasLightsWire", { "GL_christmas_light_wire.vert", "GL_christmas_light_wire.frag" });
        OpenGL::ResourceManager::LoadShader("BlitRoad", { "GL_blit_road.comp" });
        OpenGL::ResourceManager::LoadShader("BlurHorizontal", { "GL_blur_horizontal.vert", "GL_blur.frag" });
        OpenGL::ResourceManager::LoadShader("BlurVertical", { "GL_blur_vertical.vert", "GL_blur.frag" });
        OpenGL::ResourceManager::LoadShader("ComputeSkinning", { "GL_compute_skinning.comp" });
        OpenGL::ResourceManager::LoadShader("TileWorldBounds", { "GL_tile_world_bounds.comp" });

        OpenGL::ResourceManager::LoadShader("DownSample2xBox", { "GL_down_sample_2x_box.comp" });
        OpenGL::ResourceManager::LoadShader("EditorMesh", { "GL_editor_mesh.vert", "GL_editor_mesh.frag" });
        OpenGL::ResourceManager::LoadShader("EmissiveComposite", { "GL_emissive_composite.comp" });
        OpenGL::ResourceManager::LoadShader("EmissiveCompositeNew", { "GL_emissive_composite_new.comp" });
        OpenGL::ResourceManager::LoadShader("ExamineItem", { "GL_examine_item.vert", "GL_examine_item.frag" });
        OpenGL::ResourceManager::LoadShader("FogRayMarch", { "GL_fog_ray_march.comp" });
        OpenGL::ResourceManager::LoadShader("FogComposite", { "GL_fog_composite.comp" });
        OpenGL::ResourceManager::LoadShader("Fur", { "GL_fur.vert", "GL_fur.frag" });
        OpenGL::ResourceManager::LoadShader("FurComposite", { "GL_fur_composite.comp" });
        OpenGL::ResourceManager::LoadShader("GBuffer", { "GL_GBuffer.vert", "GL_gBuffer.frag" });
        OpenGL::ResourceManager::LoadShader("Gizmo", { "GL_gizmo.vert", "GL_gizmo.frag" });
        OpenGL::ResourceManager::LoadShader("Glass", { "GL_glass.vert", "GL_glass.frag" });
        OpenGL::ResourceManager::LoadShader("GlassComposite", { "GL_glass_composite.comp" });
        OpenGL::ResourceManager::LoadShader("Grass", { "GL_grass.vert", "GL_grass.frag" });
        OpenGL::ResourceManager::LoadShader("GrassGeometryGeneration", { "GL_grass_geometry_generation.comp" });
        OpenGL::ResourceManager::LoadShader("GrassPositionGeneration", { "GL_grass_position_generation.comp" });
        OpenGL::ResourceManager::LoadShader("GaussianBlurUtil", { "GL_gaussian_blur_util.comp" });
        OpenGL::ResourceManager::LoadShader("HairDepthPeel", { "GL_hair_depth_peel.vert", "GL_hair_depth_peel.frag" });
        OpenGL::ResourceManager::LoadShader("HairFinalComposite", { "GL_hair_final_composite.comp" });
        OpenGL::ResourceManager::LoadShader("HairLighting", { "GL_hair_lighting.vert", "GL_hair_lighting.frag" });
        OpenGL::ResourceManager::LoadShader("HeightMapColor", { "GL_heightmap_color.vert", "GL_heightmap_color.frag" });
        OpenGL::ResourceManager::LoadShader("HeightMapImageGeneration", { "GL_heightmap_image_generation.comp" });
        OpenGL::ResourceManager::LoadShader("HeightMapPhysxTextureGeneration", { "GL_heightmap_physx_texture_generation.comp" });
        OpenGL::ResourceManager::LoadShader("HeightMapToWorldBlit", { "GL_heightmap_to_world_blit.comp" });
        OpenGL::ResourceManager::LoadShader("HeightMapVertexGeneration", { "GL_heightmap_vertex_generation.comp" });
        OpenGL::ResourceManager::LoadShader("HeightMapPaint", { "GL_heightmap_paint.comp" });
        OpenGL::ResourceManager::LoadShader("LightCulling", { "GL_light_culling.comp" });
        OpenGL::ResourceManager::LoadShader("Lighting", { "GL_lighting.comp" });
        OpenGL::ResourceManager::LoadShader("CSMLighting", { "GL_lighting.vert", "GL_lighting.frag" });
        OpenGL::ResourceManager::LoadShader("GaussianBlur", { "GL_gaussian_blur.comp" }); // am I needed????
        OpenGL::ResourceManager::LoadShader("Outline", { "GL_outline.vert", "GL_outline.frag" });
        OpenGL::ResourceManager::LoadShader("OutlineComposite", { "GL_outline_composite.comp" });
        OpenGL::ResourceManager::LoadShader("OutlineMask", { "GL_outline_mask.vert", "GL_outline_mask.frag" });
        OpenGL::ResourceManager::LoadShader("PerlinNoise3D", { "GL_perlin_noise_3d.comp" });
        OpenGL::ResourceManager::LoadShader("ShadowMap", { "GL_shadow_map.vert", "GL_shadow_map.frag" });
        OpenGL::ResourceManager::LoadShader("ShadowCubeMap", { "GL_shadow_cube_map.vert", "GL_shadow_cube_map.frag" });
        OpenGL::ResourceManager::LoadShader("SolidColor", { "GL_solid_color.vert", "GL_solid_color.frag" });
        OpenGL::ResourceManager::LoadShader("Skybox", { "GL_skybox.vert", "GL_skybox.frag" });
        OpenGL::ResourceManager::LoadShader("SpriteSheet", { "GL_sprite_sheet.vert", "GL_sprite_sheet.frag" });
        OpenGL::ResourceManager::LoadShader("ScreenspaceReflections", { "GL_screenspace_reflections.comp" });
        OpenGL::ResourceManager::LoadShader("StainedGlass", { "GL_stained_glass.vert", "GL_stained_glass.frag" });
        OpenGL::ResourceManager::LoadShader("UI", { "GL_ui.vert", "GL_ui.frag" });
        OpenGL::ResourceManager::LoadShader("Winston", { "GL_winston.vert", "GL_winston.frag" });
        OpenGL::ResourceManager::LoadShader("CSMDepth", { "GL_csm_depth.vert", "GL_csm_depth.frag", "GL_csm_depth.geom" });
        OpenGL::ResourceManager::LoadShader("ZeroOut", { "GL_zero_out.comp" });

        OpenGL::ResourceManager::LoadShader("MetaBalls", { "GL_meta_balls.vert", "GL_meta_balls.frag" });
        OpenGL::ResourceManager::LoadShader("ViewspaceDepth", { "GL_viewspace_depth.comp" });
        OpenGL::ResourceManager::LoadShader("DepthPeeledTransparencyColor", { "GL_depth_peeled_transparency_color.vert", "GL_depth_peeled_transparency_color.frag" });
        OpenGL::ResourceManager::LoadShader("DepthPeeledTransparencyDepth", { "GL_depth_peeled_transparency_depth.vert", "GL_depth_peeled_transparency_depth.frag" });
        OpenGL::ResourceManager::LoadShader("DepthPeeledTransparencyComposite", { "GL_depth_peeled_transparency_composite.comp" });
        OpenGL::ResourceManager::LoadShader("RaytraceScene", { "GL_raytrace_scene.comp" });
        OpenGL::ResourceManager::LoadShader("Plastic", { "GL_plastic.vert", "GL_plastic.frag" });

        OpenGL::ResourceManager::LoadShader("LightAABBPosition", { "GL_light_aabb_position.vert", "GL_light_aabb_position.frag" });
        OpenGL::ResourceManager::LoadShader("LightAABBMinMax", { "GL_light_aabb_min_max.comp" });

        // Blood
        OpenGL::ResourceManager::LoadShader("Blood", "BloodDecalsCulling", { "GL_blood_decals_culling.comp" });
        OpenGL::ResourceManager::LoadShader("Blood", "BloodDecalsDraw", { "GL_blood_decals_draw.vert", "GL_blood_decals_draw.frag" });
        OpenGL::ResourceManager::LoadShader("Blood", "BloodDecalsComposite", { "GL_blood_decals_composite.comp" });
        OpenGL::ResourceManager::LoadShader("Blood", "BloodFluidDepth", { "GL_blood_fluid.vert", "GL_blood_fluid_depth.frag" });
        OpenGL::ResourceManager::LoadShader("Blood", "BloodFluidThickness", { "GL_blood_fluid.vert", "GL_blood_fluid_thickness.frag" });
        OpenGL::ResourceManager::LoadShader("Blood", "BloodFluidBlur", { "GL_blood_fluid_blur.comp" });
        OpenGL::ResourceManager::LoadShader("Blood", "VatBlood", { "GL_vat_blood.vert", "GL_vat_blood.frag" });

        // Debug
        OpenGL::ResourceManager::LoadShader("Debug", "DebugHackAABB", { "GL_debug_hack_aabb.vert", "GL_debug_hack_aabb.frag" });
        OpenGL::ResourceManager::LoadShader("Debug", "DebugLightAABB", { "GL_debug_light_aabb.vert", "GL_debug_light_aabb.frag" });
        OpenGL::ResourceManager::LoadShader("Debug", "DebugPointCloud", { "GL_debug_point_cloud.vert", "GL_debug_point_cloud.frag" });
        OpenGL::ResourceManager::LoadShader("Debug", "DebugProbes", { "GL_debug_probes.vert", "GL_debug_probes.frag" });
        OpenGL::ResourceManager::LoadShader("Debug", "DebugRagdoll", { "GL_debug_ragdoll.vert", "GL_debug_ragdoll.frag" });
        OpenGL::ResourceManager::LoadShader("Debug", "DebugSolidColor", { "GL_debug_solid_color.vert", "GL_debug_solid_color.frag" });
        OpenGL::ResourceManager::LoadShader("Debug", "DebugTextureBlit", { "GL_debug_texture_blit.vert", "GL_debug_texture_blit.frag" });
        OpenGL::ResourceManager::LoadShader("Debug", "DebugTextured", { "GL_debug_textured.vert", "GL_debug_textured.frag" });
        OpenGL::ResourceManager::LoadShader("Debug", "DebugTileView", { "GL_debug_tile_view.comp" });
        OpenGL::ResourceManager::LoadShader("Debug", "DebugVertex2D", { "GL_debug_vertex_2D.vert", "GL_debug_vertex_2D.frag" });
        OpenGL::ResourceManager::LoadShader("Debug", "DebugVertex3D", { "GL_debug_vertex_3D.vert", "GL_debug_vertex_3D.frag" });
		OpenGL::ResourceManager::LoadShader("Debug", "DebugView", { "GL_debug_view.comp" });
		OpenGL::ResourceManager::LoadShader("Debug", "DebugViewMSAA", { "GL_debug_view.comp" }, { "MSAA_ENABLED" });
		OpenGL::ResourceManager::LoadShader("Debug", "DebugViewRE", { "GL_debug_view.comp" }, { "RE_ENABLED" });

        // DDGI
		OpenGL::ResourceManager::LoadShader("DDGI", "PointCloudBaseColor", { "GL_point_cloud_basecolor.comp" });
        OpenGL::ResourceManager::LoadShader("DDGI", "PointCloudLighting", { "GL_point_cloud_lighting.comp" });
        OpenGL::ResourceManager::LoadShader("DDGI", "ProbeDistance", { "GL_probe_distance.comp" });
        OpenGL::ResourceManager::LoadShader("DDGI", "ProbeDistanceBorder", { "GL_probe_distance_border.comp" });
        OpenGL::ResourceManager::LoadShader("DDGI", "ProbeDistanceDispatchArgs", { "GL_probe_distance_dispatch_args.comp" });
        OpenGL::ResourceManager::LoadShader("DDGI", "ProbeDistanceList", { "GL_probe_distance_list.comp" });
        OpenGL::ResourceManager::LoadShader("DDGI", "ProbeIrradiance", { "GL_probe_irradiance.comp" });
        OpenGL::ResourceManager::LoadShader("DDGI", "ProbeIrradianceBorder", { "GL_probe_irradiance_border.comp" });
        OpenGL::ResourceManager::LoadShader("DDGI", "ProbeIrradianceDirtyPointCheck", { "GL_probe_irradiance_dirty_point_check.comp" });
		OpenGL::ResourceManager::LoadShader("DDGI", "ProbeIrradianceList", { "GL_probe_irradiance_list.comp" });
        OpenGL::ResourceManager::LoadShader("DDGI", "ProbeIrradianceTexture", { "GL_probe_irradiance_texture.comp" });
        OpenGL::ResourceManager::LoadShader("DDGI", "ProbeLightingDispatchArgs", { "GL_probe_lighting_dispatch_args.comp" });
        OpenGL::ResourceManager::LoadShader("DDGI", "ProbePointIndices", { "GL_probe_point_indices.comp" });
        OpenGL::ResourceManager::LoadShader("DDGI", "ProbeRelevance", { "GL_probe_relevance.comp" });
        OpenGL::ResourceManager::LoadShader("DDGI", "ProbeRelocation", { "GL_probe_state_update.comp" });
        OpenGL::ResourceManager::LoadShader("DDGI", "ProbeStateUpdate", { "GL_probe_state_update.comp" });

        // Decals
        OpenGL::ResourceManager::LoadShader("Decals", "DecalPaintUVs", { "gl_decal_paint_uvs.vert", "gl_decal_paint_uvs.frag" });
        OpenGL::ResourceManager::LoadShader("Decals", "DecalPaintMask", { "gl_decal_paint_mask.comp" });
        OpenGL::ResourceManager::LoadShader("Decals", "Decals", { "GL_decals.vert", "GL_decals.frag" });

        // Ocean
        OpenGL::ResourceManager::LoadShader("Water", "FttRadix64Vertical", { "GL_ftt_radix_64_vertical.comp" });
        OpenGL::ResourceManager::LoadShader("Water", "FttRadix8Vertical", { "GL_ftt_radix_8_vertical.comp" });
        OpenGL::ResourceManager::LoadShader("Water", "FttRadix64Horizontal", { "GL_ftt_radix_64_horizontal.comp" });
        OpenGL::ResourceManager::LoadShader("Water", "FttRadix8Horizontal", { "GL_ftt_radix_8_horizontal.comp" });
        OpenGL::ResourceManager::LoadShader("Water", "OceanFlags", { "GL_ocean_flags.comp" });
        OpenGL::ResourceManager::LoadShader("Water", "OceanSurfaceComposite", { "GL_ocean_surface_composite.comp" });
        OpenGL::ResourceManager::LoadShader("Water", "OceanGeometry", { "GL_ocean_geometry.vert", "GL_ocean_geometry.frag", "GL_ocean_geometry.tesc", "GL_ocean_geometry.tese" });
        OpenGL::ResourceManager::LoadShader("Water", "OceanCalculateSpectrum", { "GL_ocean_calculate_spectrum.comp" });
        OpenGL::ResourceManager::LoadShader("Water", "OceanUpdateTextures", { "GL_ocean_update_textures.comp" });
        OpenGL::ResourceManager::LoadShader("Water", "OceanUnderwaterComposite", { "GL_ocean_underwater_composite.comp" });
        OpenGL::ResourceManager::LoadShader("Water", "OceanTesseleationEdgeTransitionCleanUp", { "GL_ocean_tessellation_edge_transition_cleanup.comp" });
        OpenGL::ResourceManager::LoadShader("Water", "OceanPositionReadback", { "GL_ocean_position_readback.comp" });

        // Post processing
        OpenGL::ResourceManager::LoadShader("PostProcessing", "FXAA", { "GL_fxaa.comp" });
        OpenGL::ResourceManager::LoadShader("PostProcessing", "TAA", { "GL_taa.comp" });
		OpenGL::ResourceManager::LoadShader("PostProcessing", "PostProcessing", { "GL_post_processing.comp" });
    }

    void CreateSSBOs() {
		GLbitfield staticFlags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
		GLbitfield dynamicFlags = GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;

		// Create ssbos

        // Ocean
        const glm::uvec2 oceanSize = Ocean::GetBaseFFTResolution(); // WARNING!!! This size must bit your largest FFT dimensions
		OpenGL::ResourceManager::CreateSSBO("ffth0Band0").Create(Ocean::GetFFTResolution(0).x * Ocean::GetFFTResolution(0).y * sizeof(std::complex<float>), staticFlags);
		OpenGL::ResourceManager::CreateSSBO("ffth0Band1").Create(Ocean::GetFFTResolution(1).x * Ocean::GetFFTResolution(1).y * sizeof(std::complex<float>), staticFlags);
		OpenGL::ResourceManager::CreateSSBO("fftSpectrumInSSBO").Create(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
		OpenGL::ResourceManager::CreateSSBO("fftSpectrumOutSSBO").Create(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
		OpenGL::ResourceManager::CreateSSBO("fftDispInXSSBO").Create(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
		OpenGL::ResourceManager::CreateSSBO("fftDispZInSSBO").Create(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
		OpenGL::ResourceManager::CreateSSBO("fftGradXInSSBO").Create(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
		OpenGL::ResourceManager::CreateSSBO("fftGradZInSSBO").Create(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
		OpenGL::ResourceManager::CreateSSBO("fftDispXOutSSBO").Create(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
		OpenGL::ResourceManager::CreateSSBO("fftDispZOutSSBO").Create(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
		OpenGL::ResourceManager::CreateSSBO("fftGradXOutSSBO").Create(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
		OpenGL::ResourceManager::CreateSSBO("fftGradZOutSSBO").Create(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);

        int dummySize = 64;

        // Core
        OpenGL::ResourceManager::CreateSSBO("Samplers").Create(sizeof(glm::uvec2), GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ViewportData").Create(sizeof(ViewportData) * 4, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("RendererData").Create(sizeof(RendererData), GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("InstanceData").Create(sizeof(RenderItem) * MAX_INSTANCE_DATA_COUNT, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("SkinningTransforms").Create(sizeof(glm::mat4) * MAX_ANIMATED_TRANSFORMS, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("Lights").Create(sizeof(GPULight) * MAX_GPU_LIGHTS, GL_DYNAMIC_STORAGE_BIT);

        OpenGL::ResourceManager::CreateSSBO("Materials");

        OpenGL::ResourceManager::CreateSSBO("RenderItemsUI").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);

        // Vertices
        OpenGL::ResourceManager::CreateSSBO("Indices2");
        OpenGL::ResourceManager::CreateSSBO("Vertices2");
        OpenGL::ResourceManager::CreateSSBO("VertexWeights");

        // Raytracing
		OpenGL::ResourceManager::CreateSSBO("TriangleData").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
		OpenGL::ResourceManager::CreateSSBO("SceneBvh").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
		OpenGL::ResourceManager::CreateSSBO("MeshesBvh").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
		OpenGL::ResourceManager::CreateSSBO("EntityInstances").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
		OpenGL::ResourceManager::CreateSSBO("PointGridBuffer").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
		OpenGL::ResourceManager::CreateSSBO("PointIndicesBuffer").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);

		// DDGI
		OpenGL::ResourceManager::CreateSSBO("DDGIVolume").Create(sizeof(DDGIVolumeGPU), GL_DYNAMIC_STORAGE_BIT);
		OpenGL::ResourceManager::CreateSSBO("DirtyDoorAABBs").Create(sizeof(GPUAABB), GL_DYNAMIC_STORAGE_BIT);
		OpenGL::ResourceManager::CreateSSBO("PointCloudGridCounts").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
		OpenGL::ResourceManager::CreateSSBO("PointCloudDirtyFlags").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
		OpenGL::ResourceManager::CreateSSBO("PointCloudGridOffsets").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
		OpenGL::ResourceManager::CreateSSBO("PointCloudTextureInfo").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
		OpenGL::ResourceManager::CreateSSBO("ProbeDistanceCounter").Create(sizeof(uint32_t), GL_DYNAMIC_STORAGE_BIT);
		OpenGL::ResourceManager::CreateSSBO("ProbeDistanceDispatchArgs").Create(sizeof(DispatchIndirectCommand), GL_DYNAMIC_STORAGE_BIT);
		OpenGL::ResourceManager::CreateSSBO("ProbeDistanceIndices").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
		OpenGL::ResourceManager::CreateSSBO("ProbeIndexCounter").Create(sizeof(uint32_t), GL_DYNAMIC_STORAGE_BIT);
		OpenGL::ResourceManager::CreateSSBO("ProbeIrradianceCounter").Create(sizeof(uint32_t), GL_DYNAMIC_STORAGE_BIT);
		OpenGL::ResourceManager::CreateSSBO("ProbeIrradianceDispatchArgs").Create(sizeof(DispatchIndirectCommand), GL_DYNAMIC_STORAGE_BIT);
		OpenGL::ResourceManager::CreateSSBO("ProbeIrradianceIndices").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
		OpenGL::ResourceManager::CreateSSBO("ProbePointIndices").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
		OpenGL::ResourceManager::CreateSSBO("ProbePointOffsets").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
		OpenGL::ResourceManager::CreateSSBO("ProbePointCounts").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ProbeSHColor").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);
		OpenGL::ResourceManager::CreateSSBO("ProbeStates").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);

		OpenGL::ResourceManager::CreateSSBO("LightAABBs").Create(dummySize, GL_DYNAMIC_STORAGE_BIT);

        // Tile data
		OpenGL::ResourceManager::CreateSSBO("TileChristmasLights").Create(GetTileCount() * sizeof(TileInstanceData), NONE_BIT);
		OpenGL::ResourceManager::CreateSSBO("TileBloodDecals").Create(GetTileCount() * sizeof(TileInstanceData), NONE_BIT);
		OpenGL::ResourceManager::CreateSSBO("TileLights").Create(GetTileCount() * sizeof(TileLights), NONE_BIT);
		OpenGL::ResourceManager::CreateSSBO("TileWorldBounds").Create(GetTileCount() * sizeof(TileWorldBounds), NONE_BIT);

        // Instance data
        OpenGL::ResourceManager::CreateSSBO("BloodDecalCounter").Create(sizeof(uint32_t), GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("BloodDecalIndices").Create(sizeof(uint32_t) * GetTileCount() * 256, NONE_BIT);
        OpenGL::ResourceManager::CreateSSBO("BloodDecalInstances").Create(sizeof(BloodDecalInstanceData) * MAX_SCREEN_SPACE_BLOOD_DECAL_COUNT, GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ChristmasLightCounter").Create(sizeof(uint32_t), GL_DYNAMIC_STORAGE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ChristmasLightIndices").Create(sizeof(uint32_t) * GetTileCount() * 256, NONE_BIT);
        OpenGL::ResourceManager::CreateSSBO("ChristmasLightInstances").Create(MAX_CHRISTMAS_LIGHTS * sizeof(GPUChristmasLight), GL_DYNAMIC_STORAGE_BIT);

        // Remove me at some point
		OpenGL::ResourceManager::CreateSSBO("MetaBalls").Create(sizeof(glm::vec4) * 1000, GL_DYNAMIC_STORAGE_BIT);

		int MAX_OCEAN_PATCHES = 500;
		OpenGL::ResourceManager::CreateSSBO("OceanPatchTransforms").Create(sizeof(glm::mat4) * MAX_OCEAN_PATCHES, GL_DYNAMIC_STORAGE_BIT);

		// Preallocate the indirect command buffer
		g_indirectBuffer.PreAllocate(sizeof(DrawIndexedIndirectCommand) * MAX_INDIRECT_DRAW_COMMAND_COUNT);
    }

    void InitSSBOs() {
        //DispatchIndirectCommand command = { 1, 1, 1 };
        //OpenGL::UpdateSSBO("ProbeDispatchArgs", sizeof(DispatchIndirectCommand), &command);

        // HO
        const std::vector<std::complex<float>>& h0Band0 = Ocean::GetH0(0);
        const std::vector<std::complex<float>>& h0Band1 = Ocean::GetH0(1);
        if (OpenGLSSBO* ssbo = OpenGL::ResourceManager::GetSSBOPtr("ffth0Band0")) {
            ssbo->CopyFrom(h0Band0.data(), sizeof(std::complex<float>) * h0Band0.size());
        }
        if (OpenGLSSBO* ssbo = OpenGL::ResourceManager::GetSSBOPtr("ffth0Band1")) {
            ssbo->CopyFrom(h0Band1.data(), sizeof(std::complex<float>) * h0Band1.size());
        }

    }

    void UpdateSSBOS() {
        OpenGL::UpdateSSBO("Samplers", sizeof(GLuint64) * OpenGL::BackEnd::GetBindlessTextureIDs().size(), OpenGL::BackEnd::GetBindlessTextureIDs().data());

        const RendererData& rendererData = RenderDataManager::GetRendererData();
        const std::vector<BloodDecalInstanceData>& bloodScreenSpaceDecalInstances = RenderDataManager::GetBloodScreenSpaceDecalInstanceData();
        const std::vector<GPULight>& gpuLightsHighRes = RenderDataManager::GetGPULightsHighRes();
        const std::vector<RenderItem>& instanceData = RenderDataManager::GetInstanceData();
        const std::vector<ViewportData>& playerData = RenderDataManager::GetViewportData();
        const std::vector<glm::mat4>&oceanPatchTransforms = RenderDataManager::GetOceanPatchTransforms();

        GLuint zero = 0;

        OpenGL::UpdateSSBO("BloodDecalCounter", sizeof(uint32_t), &zero);
        OpenGL::UpdateSSBO("BloodDecalInstances", bloodScreenSpaceDecalInstances.size() * sizeof(BloodDecalInstanceData), bloodScreenSpaceDecalInstances.data());
        OpenGL::UpdateSSBO("ChristmasLightCounter", sizeof(uint32_t), &zero);
        OpenGL::UpdateSSBO("InstanceData", instanceData.size() * sizeof(RenderItem), instanceData.data());
        OpenGL::UpdateSSBO("Lights", gpuLightsHighRes.size() * sizeof(GPULight), gpuLightsHighRes.data());
        OpenGL::UpdateSSBO("RendererData", sizeof(RendererData), (void*)&rendererData);
        OpenGL::UpdateSSBO("ViewportData", playerData.size() * sizeof(ViewportData), playerData.data());
        OpenGL::UpdateSSBO("OceanPatchTransforms", oceanPatchTransforms.size() * sizeof(glm::mat4), oceanPatchTransforms.data());

        const std::vector<RenderItemUI>& renderItemsUI = UIBackEnd::GetRenderItems();
        OpenGL::UpdateSSBO("RenderItemsUI", renderItemsUI.size() * sizeof(RenderItemUI), renderItemsUI.data());

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        OpenGL::BindSSBO(0, "Samplers");
        OpenGL::BindSSBO(1, "RendererData");
        OpenGL::BindSSBO(2, "ViewportData");
        OpenGL::BindSSBO(3, "InstanceData");
        OpenGL::BindSSBO(4, "Lights");
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

        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        gBuffer->Bind();
        gBuffer->DrawBuffer("Lighting");

        OpenGL::BindShader("DebugHackAABB");
        glBindVertexArray(vao);

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGLRenderer::SetViewport(gBuffer, viewport);
                OpenGL::SetUniformMat4("u_projectionView", RenderDataManager::GetViewportData()[i].projectionView);
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

        OpenGLFrameBuffer& hairFrameBuffer = OpenGL::ResourceManager::GetFrameBuffer("Hair");
        Unloved::DDGIVolume& ddgiVolume = LegacyWorld::GetTestDDGIVolume();

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

        OpenGL::BindSSBO(0, "Samplers");
        OpenGL::BindSSBO(1, "RendererData");
        OpenGL::BindSSBO(2, "ViewportData");
        OpenGL::BindSSBO(3, "InstanceData");
        OpenGL::BindSSBO(4, "Lights");
        OpenGL::BindSSBO(5, "TileLights");
        OpenGL::BindSSBO(6, "TileWorldBounds");

        OpenGL::BindSSBO(10, "ProbeSHColor");
        OpenGL::BindSSBO(11, "ProbeStates");

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
            OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
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
        //    OpenGLFrameBuffer* bloodFluidFbo = OpenGL::ResourceManager::GetFrameBufferPtr("BloodFluid");
        //    OpenGL::BlitFrameBuffer(bloodFluidFbo, &finalImageBuffer, "Depth", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);
        //}
        //if (Input::KeyDown(HELL_KEY_Y)) {
        //    OpenGLFrameBuffer* bloodFluidFbo = OpenGL::ResourceManager::GetFrameBufferPtr("BloodFluid");
        //    OpenGL::BlitFrameBuffer(bloodFluidFbo, &finalImageBuffer, "Thickness", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);
        //}
        //if (Input::KeyDown(HELL_KEY_T)) {
        //    OpenGLFrameBuffer* bloodFluidFbo = OpenGL::ResourceManager::GetFrameBufferPtr("BloodFluid");
        //    OpenGL::BlitFrameBuffer(bloodFluidFbo, &finalImageBuffer, "BlurIntermediate", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);
        //}

        //BlitFog();

        OpenGLFrameBuffer& finalImageFbo = OpenGL::ResourceManager::GetFrameBuffer("FinalImage");
        OpenGLFrameBuffer& gBufferRE = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer& presentFbo = OpenGL::ResourceManager::GetFrameBuffer("Present");

        // Downscale with linear filtering
        OpenGL::BlitFrameBuffer(&gBufferRE, &finalImageFbo, "Lighting", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);

        // Upscale with nearest filtering
        OpenGL::BlitFrameBuffer(&finalImageFbo, &presentFbo, "Color", "Color", GL_COLOR_BUFFER_BIT, GL_NEAREST);

        UIPass();

        // Blit to swap chain
        OpenGL::BlitToDefaultFrameBuffer(&presentFbo, "Color", GL_COLOR_BUFFER_BIT, GL_NEAREST);

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
        OpenGLFrameBuffer& waterFrameBuffer = OpenGL::ResourceManager::GetFrameBuffer("Water");
        waterFrameBuffer.Bind();
        waterFrameBuffer.ClearAttachment("Lighting", 0, 0, 0, 0);
        waterFrameBuffer.ClearAttachmentUI("OceanFlags", 0, 0, 0, 0);
        waterFrameBuffer.ClearAttachmentUI("OceanMask", 0, 0, 0, 0);

        // GBuffer
        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        gBuffer.Bind();
        gBuffer.ClearAttachment("Lighting", 0, 0, 0, 1);
        gBuffer.ClearAttachment("BaseColorMetallic", 0, 0, 0, 1);
        gBuffer.ClearAttachment("NormalXYRoughnessMisc", 0, 0, 0, 1);
        gBuffer.ClearAttachment("VelocityXYOcclusionSubSurface", 0, 0, 0, 1);
        gBuffer.ClearAttachment("Emissive", 0.0f, 0.0f, 0.0f, 0.0f);
        gBuffer.ClearAttachmentUI("Visibility", 0, 0, 0, 0);
        gBuffer.ClearDepthAttachment(0.0f);
        gBuffer.ClearStencilBits(0);

        gBuffer.ClearAttachment("Glass", 0, 0, 0, 0); // TODO: remove me when/if u can

        // Decal mask
        OpenGLFrameBuffer& miscFullSizeFBO = OpenGL::ResourceManager::GetFrameBuffer("MiscFullSize");
        miscFullSizeFBO.Bind();
        miscFullSizeFBO.ClearTexImage("BloodScreenSpaceDecalMask", 0, 0, 0, 0);
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

                OpenGL::SetUniformInt("u_viewportIndex", viewportIndex);
                OpenGL::SetUniformInt("u_globalInstanceIndex", instanceOffset + i);

                if (bindMaterial) {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.baseColorTextureIndex)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.normalMapTextureIndex)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.rmaTextureIndex)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE3);

                    // Try bind emissive texture
                    if (renderItem.emissiveTextureIndex != -1) {
                        if (Texture* texture = Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.emissiveTextureIndex)) {
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
                    glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.additionalTextureIndex0)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE5);
                    glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.additionalTextureIndex1)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE6);
                    glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.additionalTextureIndex2)->GetGLTexture().GetHandle());
                }

                glDrawElementsBaseVertex(GL_TRIANGLES, command.indexCount, GL_UNSIGNED_INT, (GLvoid*)(command.firstIndex * sizeof(GLuint)), command.baseVertex);
            }
        }
    }

    void DrawFullscreenTriangle() {
        BindEmptyVAO();
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    void DebugHack(const std::string& message) {

    }

    void CreateBlurBuffers() {
        const Resolutions& resolutions = Config::GetResolutions();

        // Iterate each viewport
        for (int x = 0; x < 4; x++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(x);

            // Start the first blur buffer at the full viewport dimensions
            Unloved::SpaceCoords spaceCoords = viewport->GetGBufferSpaceCoords();
            float width = spaceCoords.width;
            float height = spaceCoords.height;

            // Create framebuffers, downscale by 50% each time
            for (int y = 0; y < 4; y++) {

                std::string blurBufferName = "BlurBuffer_" + std::to_string(x) + "_" + std::to_string(y);
                OpenGLFrameBuffer& blurBuffer = OpenGL::ResourceManager::CreateFrameBuffer(blurBufferName);
                blurBuffer.Create(blurBufferName, (int)width, (int)height);
                blurBuffer.CreateAttachment("ColorA", GL_RGBA8, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
                blurBuffer.CreateAttachment("ColorB", GL_RGBA8, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
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
		OpenGLRasterizerStateManager::SetRasterizerState(rasterizerState);

		for (int i = 0; i < 4; i++) {
			Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
			if (viewport->IsVisible()) {
				OpenGLRenderer::SetViewport(fbo, viewport);
				if (Hell::BackEnd::RenderDocFound()) {
					SplitMultiDrawIndirect(shader, drawCommands[i], true, false);
				}
				else {
					MultiDrawIndirect(drawCommands[i]);
				}
			}
		}
	}

    void MultiDrawPerViewportRE(OpenGLFrameBuffer& fbo, const std::vector<DrawIndexedIndirectCommand> drawCommands[4], OpenGLRasterizerState& rasterizerState) {
        OpenGLRasterizerStateManager::SetRasterizerState(rasterizerState);

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGLRenderer::SetViewport(&fbo, viewport);
                MultiDrawIndirect(drawCommands[i]);
            }
        }
    }

	void MultiDrawPerViewport(OpenGLFrameBuffer& fbo, OpenGLShader& shader, const std::vector<DrawIndexedIndirectCommand> drawCommands[4], OpenGLRasterizerState& rasterizerState) {
		OpenGLRasterizerStateManager::SetRasterizerState(rasterizerState);

		for (int i = 0; i < 4; i++) {
			Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
			if (viewport->IsVisible()) {
				OpenGLRenderer::SetViewport(&fbo, viewport);
				if (Hell::BackEnd::RenderDocFound()) {
					SplitMultiDrawIndirect(&shader, drawCommands[i], true, false);
				}
				else {
					MultiDrawIndirect(drawCommands[i]);
				}
			}
		}
	}

    GLuint GetTextureHandleByName(const std::string& name) {
        if (auto it = g_cachedTextureHandles.find(name); it != g_cachedTextureHandles.end()) {
            return it->second;
        }

        Texture* texture = Hell::ResourceManager::GetTextureByName(name);
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



    void CleanUp() {
        if (g_emptyVao != 0) {
            glDeleteVertexArrays(1, &g_emptyVao);
            g_emptyVao = 0;
        }
    }

    std::vector<float>& GetShadowCascadeLevels() {
        return g_shadowCascadeLevels;
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
