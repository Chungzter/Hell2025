#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Unloved/Editor/Editor.h"
#include "Unloved/Render/Renderer.h"

#include "Unloved/Render/RendererConstants.h"

namespace OpenGL::Renderer {

    void MaterialResolvePass() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gbufferFbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        gbufferFbo.Bind();
        gbufferFbo.SetViewport();
        gbufferFbo.DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "VelocityXYOcclusionSubSurface" });

        OpenGL::BindShader("MaterialResolve");
        OpenGL::SetUniformBool("u_hasPreviousSkinnedPositions", false);
        OpenGL::SetUniformBool("u_woundMaskEnabled", false);
        OpenGL::SetUniformBool("u_heightMapResolve", false);

        OpenGL::BindImageTexture(0, gbufferFbo.GetColorAttachmentHandleByName("Visibility"), GL_READ_ONLY, GL_RG32UI);
        OpenGL::BindTextureUnit(1, gbufferFbo.GetDepthAttachmentHandle());

        OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry");
        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
        OpenGL::BindSSBO(SSBO_IDX_MATERIAL_RESOLVE_VERTICES, meshBuffer.GetVBO());
        OpenGL::BindSSBO(SSBO_IDX_MATERIAL_RESOLVE_INDICES, meshBuffer.GetEBO());

        OpenGLRasterizerState state;
        state.depthTestEnabled = false;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.colorMask = true;

        state.stencilTestEnabled = true;
        state.stencilFunc = GL_EQUAL;
        state.stencilRef = STENCIL_BIT_ASSET;
        state.stencilReadMask = 0xFF;
        state.stencilWriteMask = 0x00;
        state.stencilFailOp = GL_KEEP;
        state.stencilDepthFailOp = GL_KEEP;
        state.stencilPassOp = GL_KEEP;

        OpenGL::RasterizerStateManager::SetRasterizerState(state);
        RenderFullscreenTriangle();
    }

    void MaterialResolveHeightMapPass() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer* roadFramebuffer = OpenGL::ResourceManager::GetFrameBufferPtr("Road");
        if (!roadFramebuffer) return;

        OpenGLFrameBuffer& gbufferFbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        gbufferFbo.Bind();
        gbufferFbo.SetViewport();
        gbufferFbo.DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "VelocityXYOcclusionSubSurface" });

        OpenGL::BindShader("MaterialResolve");
        OpenGL::SetUniformBool("u_hasPreviousSkinnedPositions", false);
        OpenGL::SetUniformBool("u_woundMaskEnabled", false);
        OpenGL::SetUniformBool("u_heightMapResolve", true);

        OpenGL::BindImageTexture(0, gbufferFbo.GetColorAttachmentHandleByName("Visibility"), GL_READ_ONLY, GL_RG32UI);
        OpenGL::BindTextureUnit(1, gbufferFbo.GetDepthAttachmentHandle());
        OpenGL::BindTextureUnit(3, roadFramebuffer->GetColorAttachmentHandleByName("RoadMask"));

        OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer("HeightMapGeometry");
        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
        OpenGL::BindSSBO(SSBO_IDX_MATERIAL_RESOLVE_VERTICES, meshBuffer.GetVBO());
        OpenGL::BindSSBO(SSBO_IDX_MATERIAL_RESOLVE_INDICES, meshBuffer.GetEBO());

        int32_t dirtRoadMaterialIndex = Hell::ResourceManager::GetMaterialIndexByName("DirtRoad");
        int32_t groundMaterialIndex = Hell::ResourceManager::GetMaterialIndexByName("Ground_MudVeg");
        if (groundMaterialIndex == -1) {
            groundMaterialIndex = Hell::ResourceManager::GetMaterialIndexByName("CheckerBoard");
        }
        OpenGL::SetUniformInt("u_dirtRoadMaterialIndex", dirtRoadMaterialIndex == -1 ? groundMaterialIndex : dirtRoadMaterialIndex);

        float textureScaling = 1.0f;
        if (Unloved::Editor::IsOpen() && Unloved::Editor::GetEditorMode() == EditorMode::MAP_HEIGHT_EDITOR) {
            textureScaling = 0.1f;
        }
        OpenGL::SetUniformFloat("u_textureScaling", textureScaling);

        OpenGLRasterizerState state;
        state.depthTestEnabled = false;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.colorMask = true;

        state.stencilTestEnabled = true;
        state.stencilFunc = GL_EQUAL;
        state.stencilRef = STENCIL_BIT_HEIGHT_MAP;
        state.stencilReadMask = 0xFF;
        state.stencilWriteMask = 0x00;
        state.stencilFailOp = GL_KEEP;
        state.stencilDepthFailOp = GL_KEEP;
        state.stencilPassOp = GL_KEEP;

        OpenGL::RasterizerStateManager::SetRasterizerState(state);
        RenderFullscreenTriangle();
    }

    void MaterialResolveSkinnedPass() {
        ProfilerOpenGLZoneFunction();

        Hell::TextureArray* woundMaskArray = Hell::ResourceManager::GetTextureArrayPtr("WoundMasks");

        OpenGLFrameBuffer& gbufferFbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        gbufferFbo.Bind();
        gbufferFbo.SetViewport();
        gbufferFbo.DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "VelocityXYOcclusionSubSurface" });

        OpenGL::BindShader("MaterialResolve");
        OpenGL::SetUniformBool("u_hasPreviousSkinnedPositions", true);
        OpenGL::SetUniformBool("u_woundMaskEnabled", woundMaskArray != nullptr);
        OpenGL::SetUniformBool("u_heightMapResolve", false);

        OpenGL::BindImageTexture(0, gbufferFbo.GetColorAttachmentHandleByName("Visibility"), GL_READ_ONLY, GL_RG32UI);
        OpenGL::BindTextureUnit(1, gbufferFbo.GetDepthAttachmentHandle());
        if (woundMaskArray) OpenGL::BindTextureUnit(2, woundMaskArray->GetHandle());

        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
        OpenGL::BindSSBO(SSBO_IDX_MATERIAL_RESOLVE_VERTICES, OpenGL::BackEnd::GetSkinnedVertexDataVBO());
        OpenGL::BindSSBO(SSBO_IDX_MATERIAL_RESOLVE_INDICES, OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry").GetEBO());
        OpenGL::BindSSBO(SSBO_IDX_MATERIAL_RESOLVE_PREVIOUS_POSITIONS, OpenGL::BackEnd::GetPreviousSkinnedPositionBuffer());

        OpenGLRasterizerState state;
        state.depthTestEnabled = false;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.colorMask = true;

        state.stencilTestEnabled = true;
        state.stencilFunc = GL_EQUAL;
        state.stencilReadMask = 0xFF;
        state.stencilWriteMask = 0x00;
        state.stencilFailOp = GL_KEEP;
        state.stencilDepthFailOp = GL_KEEP;
        state.stencilPassOp = GL_KEEP;

        state.stencilRef = STENCIL_BIT_SKINNED;
        OpenGL::RasterizerStateManager::SetRasterizerState(state);
        RenderFullscreenTriangle();

        state.stencilRef = STENCIL_BIT_SKINNED_HAIR;
        OpenGL::RasterizerStateManager::SetRasterizerState(state);
        RenderFullscreenTriangle();
    }

    void MaterialResolveProceduralPass() {
        ProfilerOpenGLZoneFunction();

        OpenGLMeshBuffer& proceduralMeshBuffer = OpenGL::ResourceManager::GetMeshBuffer("Procedural");

        OpenGLFrameBuffer& gbufferFbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        gbufferFbo.Bind();
        gbufferFbo.SetViewport();
        gbufferFbo.DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "VelocityXYOcclusionSubSurface" });

        OpenGL::BindShader("MaterialResolve");
        OpenGL::SetUniformBool("u_hasPreviousSkinnedPositions", false);
        OpenGL::SetUniformBool("u_woundMaskEnabled", false);
        OpenGL::SetUniformBool("u_heightMapResolve", false);

        OpenGL::BindImageTexture(0, gbufferFbo.GetColorAttachmentHandleByName("Visibility"), GL_READ_ONLY, GL_RG32UI);
        OpenGL::BindTextureUnit(1, gbufferFbo.GetDepthAttachmentHandle());

        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
        OpenGL::BindSSBO(SSBO_IDX_MATERIAL_RESOLVE_VERTICES, proceduralMeshBuffer.GetVBO());
        OpenGL::BindSSBO(SSBO_IDX_MATERIAL_RESOLVE_INDICES, proceduralMeshBuffer.GetEBO());

        OpenGLRasterizerState state;
        state.depthTestEnabled = false;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.colorMask = true;

        state.stencilTestEnabled = true;
        state.stencilFunc = GL_EQUAL;
        state.stencilRef = STENCIL_BIT_PROCEDUAL;
        state.stencilReadMask = 0xFF;
        state.stencilWriteMask = 0x00;
        state.stencilFailOp = GL_KEEP;
        state.stencilDepthFailOp = GL_KEEP;
        state.stencilPassOp = GL_KEEP;

        OpenGL::RasterizerStateManager::SetRasterizerState(state);
        RenderFullscreenTriangle();
    }

    void HairLightingSkinnedResolvePass() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gbufferFbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        gbufferFbo.Bind();
        gbufferFbo.SetViewport();
        gbufferFbo.DrawBuffers({ "Lighting", "BaseColorMetallic", "NormalXYRoughnessMisc", "VelocityXYOcclusionSubSurface" });

        OpenGL::BindShader("HairLightingResolve");

        OpenGL::BindImageTexture(0, gbufferFbo.GetColorAttachmentHandleByName("Visibility"), GL_READ_ONLY, GL_RG32UI);
        OpenGL::BindTextureUnit(1, gbufferFbo.GetDepthAttachmentHandle());

        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
        OpenGL::BindSSBO(SSBO_IDX_MATERIAL_RESOLVE_VERTICES, OpenGL::BackEnd::GetSkinnedVertexDataVBO());
        OpenGL::BindSSBO(SSBO_IDX_MATERIAL_RESOLVE_INDICES, OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry").GetEBO());

        OpenGLRasterizerState state;
        state.depthTestEnabled = false;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.colorMask = true;

        state.stencilTestEnabled = true;
        state.stencilFunc = GL_EQUAL;
        state.stencilRef = STENCIL_BIT_SKINNED_HAIR;
        state.stencilReadMask = 0xFF;
        state.stencilWriteMask = 0x00;
        state.stencilFailOp = GL_KEEP;
        state.stencilDepthFailOp = GL_KEEP;
        state.stencilPassOp = GL_KEEP;

        OpenGL::RasterizerStateManager::SetRasterizerState(state);
        RenderFullscreenTriangle();
    }
}
