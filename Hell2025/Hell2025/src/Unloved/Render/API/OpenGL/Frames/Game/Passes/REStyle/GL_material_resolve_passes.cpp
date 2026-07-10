#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Unloved/Render/Renderer.h"

#include "Unloved/Render/RendererConstants.h"

namespace OpenGL::Renderer {

    void MaterialResolvePass() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gbufferFbo = OpenGL::ResourceManager::GetFrameBuffer("GBufferRE");
        gbufferFbo.Bind();
        gbufferFbo.SetViewport();
        gbufferFbo.DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "VelocityXYOcclusionSubSurface" });

        OpenGL::BindShader("MaterialResolve");

        OpenGL::BindImageTexture(0, gbufferFbo.GetColorAttachmentHandleByName("Visibility"), GL_READ_ONLY, GL_RG32UI);
        OpenGL::BindTextureUnit(1, gbufferFbo.GetDepthAttachmentHandle());

        OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry");
        OpenGL::BindSSBO(0, "Samplers");
        OpenGL::BindSSBO(1, "Materials");
        OpenGL::BindSSBO(2, "RendererData");
        OpenGL::BindSSBO(3, "ViewportData");
        OpenGL::BindSSBO(4, "InstanceData");
        OpenGL::BindSSBO(6, meshBuffer.GetVBO());
        OpenGL::BindSSBO(7, meshBuffer.GetEBO());

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

    void MaterialResolveSkinnedPass() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gbufferFbo = OpenGL::ResourceManager::GetFrameBuffer("GBufferRE");
        gbufferFbo.Bind();
        gbufferFbo.SetViewport();
        gbufferFbo.DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "VelocityXYOcclusionSubSurface" });

        OpenGL::BindShader("MaterialResolve");

        OpenGL::BindImageTexture(0, gbufferFbo.GetColorAttachmentHandleByName("Visibility"), GL_READ_ONLY, GL_RG32UI);
        OpenGL::BindTextureUnit(1, gbufferFbo.GetDepthAttachmentHandle());

        OpenGL::BindSSBO(0, "Samplers");
        OpenGL::BindSSBO(1, "Materials");
        OpenGL::BindSSBO(2, "RendererData");
        OpenGL::BindSSBO(3, "ViewportData");
        OpenGL::BindSSBO(4, "InstanceData");
        OpenGL::BindSSBO(6, OpenGL::BackEnd::GetSkinnedVertexDataVBO());
        OpenGL::BindSSBO(7, OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry").GetEBO());

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

        OpenGLFrameBuffer& gbufferFbo = OpenGL::ResourceManager::GetFrameBuffer("GBufferRE");
        gbufferFbo.Bind();
        gbufferFbo.SetViewport();
        gbufferFbo.DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "VelocityXYOcclusionSubSurface" });

        OpenGL::BindShader("MaterialResolve");

        OpenGL::BindImageTexture(0, gbufferFbo.GetColorAttachmentHandleByName("Visibility"), GL_READ_ONLY, GL_RG32UI);
        OpenGL::BindTextureUnit(1, gbufferFbo.GetDepthAttachmentHandle());

        OpenGL::BindSSBO(0, "Samplers");
        OpenGL::BindSSBO(1, "Materials");
        OpenGL::BindSSBO(2, "RendererData");
        OpenGL::BindSSBO(3, "ViewportData");
        OpenGL::BindSSBO(4, "InstanceData");
        OpenGL::BindSSBO(6, proceduralMeshBuffer.GetVBO());
        OpenGL::BindSSBO(7, proceduralMeshBuffer.GetEBO());

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

        OpenGLFrameBuffer& gbufferFbo = OpenGL::ResourceManager::GetFrameBuffer("GBufferRE");
        gbufferFbo.Bind();
        gbufferFbo.SetViewport();
        gbufferFbo.DrawBuffers({ "Lighting", "BaseColorMetallic", "NormalXYRoughnessMisc", "VelocityXYOcclusionSubSurface" });

        OpenGL::BindShader("HairLightingResolve");

        OpenGL::BindImageTexture(0, gbufferFbo.GetColorAttachmentHandleByName("Visibility"), GL_READ_ONLY, GL_RG32UI);
        OpenGL::BindTextureUnit(1, gbufferFbo.GetDepthAttachmentHandle());

        OpenGL::BindSSBO(0, "Samplers");
        OpenGL::BindSSBO(1, "Materials");
        OpenGL::BindSSBO(2, "RendererData");
        OpenGL::BindSSBO(3, "ViewportData");
        OpenGL::BindSSBO(4, "InstanceData");
        OpenGL::BindSSBO(6, OpenGL::BackEnd::GetSkinnedVertexDataVBO());
        OpenGL::BindSSBO(7, OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry").GetEBO());

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
