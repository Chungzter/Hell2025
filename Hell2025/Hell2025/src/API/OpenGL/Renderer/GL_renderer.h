#pragma once
#include "Math/AABB.h"
#include "Math/OBB.h"
#include <string>
#include "API/OpenGL/Types/GL_cubemapView.h"
#include "API/OpenGL/Types/GL_cubemap_frame_buffer.h"
#include "API/OpenGL/Types/GL_mesh_buffer.h"
#include "API/OpenGL/Types/GL_mesh_patch.h"
#include "API/OpenGL/Types/GL_frameBuffer.h"
#include "API/OpenGL/Types/GL_shader.h"
#include "API/OpenGL/Types/GL_shadow_map.h"
#include "API/OpenGL/Types/GL_shadow_cube_map_array.h"
#include "API/OpenGL/Types/GL_shadow_map_array.h"
#include "API/OpenGL/Types/GL_texture_array.h"
#include "API/OpenGL/Types/GL_texture_3d.h"
#include "API/OpenGL/Types/GL_timer.h"
#include "API/OpenGL/Types/GL_ssbo.h"
#include "GlobalIllumination/DDGIVolume.h"
#include "Types/Map/Map.h"
#include "Viewport/Viewport.h"

struct OpenGLRasterizerState {
    // Blending
    GLboolean blendEnable = false;
    GLenum blendFuncSrcfactor = GL_SRC_ALPHA;
    GLenum blendFuncDstfactor = GL_ONE_MINUS_SRC_ALPHA;

    // Color
    GLboolean colorMask = true;

    // Depth
    GLboolean depthTestEnabled = true;
    GLboolean depthMask = true;
    GLenum depthFunc = GL_LESS;

    // Misc
    GLboolean cullfaceEnable = true;
    GLenum cullfaceMode = GL_BACK;
    GLfloat pointSize = 1.0f;

    // Stencil
    GLboolean stencilTestEnabled = false;
    GLenum stencilFunc = GL_ALWAYS;
    GLint stencilRef = 0;
    GLuint stencilReadMask = 0xFF;
    GLuint stencilWriteMask = 0xFF;
    GLenum stencilFailOp = GL_KEEP;
    GLenum stencilDepthFailOp = GL_KEEP;
    GLenum stencilPassOp = GL_KEEP;
};

namespace OpenGLRenderer {
    void Init();
    void InitMain();
    void CleanUp();

	void RenderLoadingScreen();

    void PreGameLogicComputePasses();
    void RenderGame();

    void InitMSAA();
    void RenderGameMSAA();

	void InitREStyle();
	void RenderGameREStyle();

    void CreateSSBOs();
    void InitSSBOs();
    void UpdateSSBOS();

    void ParticlePass();

    // Compute passes
    void BlitRoads();
    //void ComputeLightVolumeMask();
    //void ComputeProbeLighting();
    void ComputeOceanFFTPass();
    void ComputeSkinningPass();
    void ComputeTileWorldBounds();
    void OceanHeightReadback();
    void PaintHeightMap();
    void ComputeViewspaceDepth();

    // Init passes
    void InitGrass();
    void InitOceanHeightReadback();

    // Render passes
    void ChristmasLightCullingPass();
    void DebugPass();
    void DebugViewPass();
    void DecalPaintingPass();
    void DownSampleFinalImage();
    void EditorPass();
    void EmissivePass();
    void FurPass();
    void GeometryPass();
    void MetaBallsPass();
    void MirrorGeometryPass();
    void GlassPass();
    void GrassPass();
    void HairPass();
    void HeightMapPass();
    void HouseGeometryPass();
    void ImGuiPass();
    void InventoryGaussianPass();
    void LightCullingPass();
    void LightingPass();
    void OceanGeometryPass();
    void OceanUnderWaterFlags();
    void OceanSurfaceCompositePass();
    void OceanUnderwaterCompositePass();
    void OutlinePass();
    void PostProcessingPass();
    void PlasticPass();
    void WinstonPass();
    void BloodDecalsPass();
    void SkyBoxPass();
    void SpriteSheetPass();
    void ScreenspaceReflectionsPass();
    void StainedGlassPass();
    void UIPass();
    void VatBloodPass();
    void WeatherBoardsPass();
    void ChristmasLightsPass();
    void ExamineItemPass();
    void DepthPeeledTransparencyPass();

    void ComputeLightAABBs();
    void ReserveLightAABBSSBOStorage();
    void DebugDrawLightAABBs();

    void GaussianBlur();



    // Requiem functions
    void VisibilityPass();
    void VisibilityAlphaDiscardPass();
    void VisibilitySkinnedPass();
    void VisibilitySkinnedHairPass();

    void MaterialResolvePass();
    void MaterialResolveSkinnedPass();
    void MaterialResolveProceduralPass();

    void HairPassRE();

    void PostProcessingPassRE();

    void BindShadowMapsRE();
    void RenderFullscreenTriangle();



    // Debug passes
    void RaytracedSceneDebug();
    void DrawPointCloud(DDGIVolume& ddgiVolume);
    void DrawPointCloudGrid(DDGIVolume& ddgiVolume);
    void DrawProbes(DDGIVolume& ddgiVolume);
    void DrawGPUBvhSceneNodes(DDGIVolume& volume, const glm::vec4& color);
    void DrawGPUBvhSceneLeafNodes(DDGIVolume& volume, const glm::vec4& color);
    void DrawRaytracingBvh(DDGIVolume& volume);

    // Global illumination
    void UpdateGlobalIllumintation();
    OpenGLTextureArray& GetProbeDistanceTextureArray();

    // Utility passes
    void RecalculateAllHeightMapData(bool blitWorldMap);
    void ReadBackHeightMapData(Map* map);
    void ClearAllWoundMasks();

    // Render tasks
    void RenderShadowMaps();

    void InitFog();
    void BlitFog();
    void RayMarchFog();

    // Debug
    void UpdateDebugMesh();
    void DrawLine(const glm::vec3& begin, const glm::vec3& end, const glm::vec4& color, bool depthEnabled = false, int exclusiveViewportIndex = -1, int ignoredViewportIndex = -1);
    void DrawLine2D(const glm::ivec2& begin, const glm::ivec2& end, const glm::vec4& color);
    void DrawPoint(const glm::vec3& position, const glm::vec4& color, bool depthEnabled = false, int exclusiveViewportIndex = -1);
    void DrawPoint2D(const glm::ivec2& position, const glm::vec4& color);
    void DrawAABB(const AABB& aabb, const glm::vec4& color);
    void DrawAABB(const AABB& aabb, const glm::vec4& color, const glm::mat4& worldTransform);
    void DrawOBB(const OBB& obb, const glm::vec4& color);
    void DrawFrustum(const Frustum& frustum, const glm::vec4& color);
    void DrawSphere(const glm::vec3& position, float radius, const glm::vec4& color);
    void DebugBlitFrameBufferTexture(const std::string& frameBufferName, const std::string& attachmentName, GLint dstX, GLint dstY, GLint width, GLint height);
    void DebugBlitOpenGLTexture(GLuint textureHandle, float scale);
    void BlitDebugTextures();

    void DrawItemExamineLine(const glm::vec3& begin, const glm::vec3& end, const glm::vec4& color);
    void DrawItemExamineAABB(const AABB& aabb, const glm::vec4& color);

    inline std::vector<DebugVertex2D> g_points2D;
    inline std::vector<DebugVertex3D> g_points3D;
    inline std::vector<DebugVertex2D> g_lines2D;
    inline std::vector<DebugVertex3D> g_lines3D;
    inline std::vector<DebugVertex3D> g_itemExaminelines;

    void CreateBlurBuffers();
    void DrawQuad();

    void BindEmptyVAO();
    GLuint GetTextureHandleByName(const std::string& name);

    OpenGLCubemapView& GetCubemapView(const std::string& name);
    OpenGLCubemapView* GetCubemapViewOLD(const std::string& name);
    OpenGLFrameBuffer* GetBlurBuffer(int viewportIndex, int bufferIndex);
    OpenGLShader& GetShader(const std::string& name);
    OpenGLShader* GetShaderOLD(const std::string& name);
    OpenGLShadowMap& GetShadowMap(const std::string& name);
    OpenGLShadowMap* GetShadowMapOLD(const std::string& name);
	OpenGLShadowCubeMapArray* GetShadowCubeMapArrayOLD(const std::string& name);
    OpenGLShadowCubeMapArray& GetShadowCubeMapArray(const std::string& name);
    OpenGLShadowCubeMapArray* GetLightAABBCubeMapArray(const std::string& name);
    OpenGLShadowMapArray* GetShadowMapArrayOLD(const std::string& name);
    OpenGLShadowMapArray& GetShadowMapArray(const std::string& name);
    OpenGLTextureArray* GetTextureArray(const std::string& name);
    OpenGLTexture3D* GetTexture3D(const std::string& name);
    OpenGLMeshPatch* GetOceanMeshPatch();

    std::vector<float>& GetShadowCascadeLevels();
    void MultiDrawPerViewport(OpenGLFrameBuffer* fbo, OpenGLShader* shader, const std::vector<DrawIndexedIndirectCommand> drawCommands[4], OpenGLRasterizerState& rasterizerState);
    void MultiDrawPerViewport(OpenGLFrameBuffer& fbo, OpenGLShader& shader, const std::vector<DrawIndexedIndirectCommand> drawCommands[4], OpenGLRasterizerState& rasterizerState);
    void MultiDrawPerViewportRE(OpenGLFrameBuffer& fbo, const std::vector<DrawIndexedIndirectCommand> drawCommands[4], OpenGLRasterizerState& rasterizerState);

    // Frame Buffers
    OpenGLFrameBuffer& CreateFrameBuffer(const std::string& name, glm::ivec2 resolution);
    OpenGLFrameBuffer& CreateFrameBuffer(const std::string& name, int32_t width, int32_t height);
    OpenGLFrameBuffer& CreateMultisampledFrameBuffer(const std::string& name, glm::ivec2 resolution, uint32_t sampleCount);
    OpenGLFrameBuffer& CreateMultisampledFrameBuffer(const std::string& name, int32_t width, int32_t height, uint32_t sampleCount);
    OpenGLFrameBuffer& GetFrameBuffer(const std::string& name);
    OpenGLFrameBuffer* GetFrameBufferOLD(const std::string& name);

    OpenGLCubemapFrameBuffer& CreateCubemapFrameBuffer(const std::string& name, int32_t size);
    OpenGLCubemapFrameBuffer& GetCubemapFrameBuffer(const std::string& name);

    // Textures
    void BindImageTexture(uint32_t bindingIndex, uint32_t textureHandle, uint32_t access, uint32_t format, bool layered = false);
    void BindImageTextureArray(uint32_t bindingIndex, uint32_t textureHandle, uint32_t access, uint32_t format);
    void BindTextureUnit(uint32_t bindingIndex, uint32_t textureHandle);

	// Shaders
	void LoadShader(const std::string& name, const std::vector<std::string>& shaderPaths, const std::vector<std::string>& defines = std::vector<std::string>());
	void LoadShader(const std::string& subDirectory, const std::string& name, const std::vector<std::string>& shaderPaths, const std::vector<std::string>& defines = std::vector<std::string>());
    void BindShader(const std::string& name);
	void HotloadShaders();
    void SetUniformBool(const std::string& name, bool value);
    void SetUniformInt(const std::string& name, int value);
    void SetUniformUInt(const std::string& name, uint32_t value);
    void SetUniformFloat(const std::string& name, float value);
    void SetUniformVec2(const std::string& name, const glm::vec2& value);
    void SetUniformVec3(const std::string& name, const glm::vec3& value);
    void SetUniformIVec3(const std::string& name, const glm::ivec3& value);
    void SetUniformUVec3(const std::string& name, const glm::uvec3& value);
	void SetUniformVec4(const std::string& name, const glm::vec4& value);
	void SetUniformMat4(const std::string& name, const glm::mat4& value);

    // SSBOs
    void CreateSSBO(const std::string& name, size_t size, GLbitfield flags);
    void CreateSSBOStatic(const std::string& name);
    void UpdateSSBO(const std::string& name, size_t size, const void* data);
    void UploadSSBOStatic(const std::string& name, size_t size, const void* data);
    void BindSSBO(unsigned int bindingIndex, const std::string& name);
    void BindSSBO(unsigned int bindingIndex, uint32_t vboHandle);
    void ClearSSBO(const std::string& name);
    void ClearSSBORange(const std::string& name, size_t offset, size_t size);
    void ReserveSSBO(const std::string& name, size_t size);
    void BindDispatchBuffer(const std::string& name);
    void BindDrawIndirectBuffer(const std::string& name);
    OpenGLSSBO* GetSSBO(const std::string& name);

    // Misc
    void CreateGrassGeometry();
    void EditorRasterizerStateOverride();
    void DispatchCompute(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ);
    void DispatchComputeIndirect();

    void DebugHack(const std::string& message);

    // Rasterizer State
    void InitRasterizerStates();
    OpenGLRasterizerState* CreateRasterizerState(const std::string& name);
    OpenGLRasterizerState* GetRasterizerState(const std::string& name);
    void ForceRasterizerState(const std::string& name);
    void ForceRasterizerState(const OpenGLRasterizerState& rasterizerState);
    void SetRasterizerState(const OpenGLRasterizerState& rasterizerState);

    // Vertex Data
    void UploadVertexWeights();

    // Drawing
    void MultiDrawIndirect(const std::vector<DrawIndexedIndirectCommand>& commands);
    void SplitMultiDrawIndirect(OpenGLShader* shader, const std::vector<DrawIndexedIndirectCommand>& commands, bool bindMaterial, bool bindWoundMaterial);

    // Util
    void SetViewport(OpenGLFrameBuffer* framebuffer, Viewport* viewport);
    void ClearFrameBufferByViewport(OpenGLFrameBuffer* framebuffer, const char* attachmentName, Viewport* viewport, GLfloat r, GLfloat g = 0.0f, GLfloat b = 0.0f, GLfloat a = 0.0f);
    void ClearFrameBufferByViewportInt(OpenGLFrameBuffer* framebuffer, const char* attachmentName, Viewport* viewport, GLint r, GLint g = 0.0f, GLint b = 0.0f, GLint a = 0.0f);
    void ClearFrameBufferByViewportUInt(OpenGLFrameBuffer* framebuffer, const char* attachmentName, Viewport* viewport, GLuint r, GLuint g = 0.0f, GLuint b = 0.0f, GLuint a = 0.0f);
    void BlitFrameBuffer(OpenGLFrameBuffer* srcFrameBuffer, OpenGLFrameBuffer* dstFrameBuffer, const char* srcName, const char* dstName, GLbitfield mask, GLenum filter);
    void BlitFrameBuffer(OpenGLFrameBuffer* srcFrameBuffer, OpenGLFrameBuffer* dstFrameBuffer, const char* srcName, const char* dstName, BlitRect srcRect, BlitRect dstRect, GLbitfield mask, GLenum filter);
    void BlitFrameBufferDepth(OpenGLFrameBuffer* srcFrameBuffer, OpenGLFrameBuffer* dstFrameBuffer);
    void BlitFrameBufferDepth(OpenGLFrameBuffer* srcFrameBuffer, OpenGLFrameBuffer* dstFrameBuffer, const Viewport* viewport);
    void BlitFrameBufferDepth(OpenGLFrameBuffer* srcFrameBuffer, OpenGLFrameBuffer* dstFrameBuffer, BlitRect srcRect, BlitRect dstRect);
    void BlitToDefaultFrameBuffer(OpenGLFrameBuffer* srcFrameBuffer, const char* srcName, GLbitfield mask, GLenum filter);
    void BlitToDefaultFrameBuffer(OpenGLFrameBuffer* srcFrameBuffer, const char* srcName, BlitRect srcRect, BlitRect dstRect, GLbitfield mask, GLenum filter);
    void BlitShadowCubeMapArray(OpenGLShadowCubeMapArray& src, OpenGLShadowCubeMapArray& dst, int srcLayerIndex, int dstLayerIndex);

    RenderItem2D CreateRenderItem2D(const std::string& textureName, glm::ivec2 location, glm::ivec2 viewportSize, Alignment alignment, glm::vec3 colorTint = WHITE, glm::ivec2 size = glm::ivec2(-1, -1));
    BlitRect BlitRectFromFrameBufferViewport(OpenGLFrameBuffer* framebuffer, Viewport* viewport);
    GLint CreateQuadVAO();
    void CopyDepthBuffer(OpenGLFrameBuffer* srcFrameBuffer, OpenGLFrameBuffer* dstFrameBuffer);
    void GaussianBlur(OpenGLFrameBuffer& srcFrameBuffer, OpenGLFrameBuffer& dstFrameBuffer, const std::string& srcAttachmentName, const std::string& dstAttachmentName, BlitRect srcRect, BlitRect dstRect, int blurRadius, int passCount);
    int GetFftDisplayMode();

	uint32_t GetTileCount();
	uint32_t GetTileCountX();
	uint32_t GetTileCountY();

    // TIDY ME
    inline bool g_flipNormalMapY = false;
    inline void FlipNormalMapY() {
        g_flipNormalMapY = !g_flipNormalMapY;
    }
    inline bool ShouldFlipNormalMapY() {
        return g_flipNormalMapY;
    }
    // TIDY ME

    // Profiling
    const std::string& GetZoneNames();
    const std::string& GetZoneGPUTimings();
    const std::string& GetZoneCPUTimings();
    const std::string& GetTotalGPUTime();
    const std::string& GetTotalCPUTime();
}