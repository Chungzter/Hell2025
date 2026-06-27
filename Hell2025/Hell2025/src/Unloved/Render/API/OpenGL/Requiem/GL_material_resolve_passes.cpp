#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Renderer/Renderer.h"

#include "Unloved/Render/RendererConstants.h"

using namespace Hell;

namespace OpenGLRenderer {

    void MaterialResolvePass() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gbufferFbo = OpenGL::ResourceManager::GetFrameBuffer("GBufferRE");
        gbufferFbo.Bind();
        gbufferFbo.SetViewport();
        gbufferFbo.DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "VelocityXYOcclusionSubSurface" });

        OpenGL::BindShader("MaterialResolve");

        OpenGL::BindImageTexture(0, gbufferFbo.GetColorAttachmentHandleByName("Visibility"), GL_READ_ONLY, GL_RG32UI);
        OpenGL::BindTextureUnit(1, gbufferFbo.GetDepthAttachmentHandle());

        MeshBuffer& meshBuffer = ResourceManager::GetMeshBuffer("AssetGeometry");
        OpenGL::BindSSBO(0, meshBuffer.GetVBO());
        OpenGL::BindSSBO(1, meshBuffer.GetEBO());
        OpenGL::BindSSBO(2, "ViewportData");
        OpenGL::BindSSBO(3, "InstanceData");
        OpenGL::BindSSBO(4, "Samplers");
        OpenGL::BindSSBO(5, "RendererData");

        OpenGLRasterizerState state;
        state.depthTestEnabled = false;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.colorMask = true;

        state.stencilTestEnabled = true;
        state.stencilFunc = GL_EQUAL;
        state.stencilRef = STENCIL_BIT_STATIC;
        state.stencilReadMask = 0xFF;
        state.stencilWriteMask = 0x00;
        state.stencilFailOp = GL_KEEP;
        state.stencilDepthFailOp = GL_KEEP;
        state.stencilPassOp = GL_KEEP;

        OpenGLRasterizerStateManager::SetRasterizerState(state);
        RenderFullscreenTriangle();
    }

    void MaterialResolveSkinnedPass() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gbufferFbo = OpenGL::ResourceManager::GetFrameBuffer("GBufferRE");
        gbufferFbo.Bind();
        gbufferFbo.SetViewport();
        gbufferFbo.DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "VelocityXYOcclusionSubSurface" });

        OpenGL::BindShader("MaterialResolveSkinning");

        OpenGL::BindImageTexture(0, gbufferFbo.GetColorAttachmentHandleByName("Visibility"), GL_READ_ONLY, GL_RG32UI);
        OpenGL::BindTextureUnit(1, gbufferFbo.GetDepthAttachmentHandle());

        OpenGL::BindSSBO(0, OpenGL::BackEnd::GetSkinnedVertexDataVBO());
        OpenGL::BindSSBO(1, ResourceManager::GetMeshBuffer("AssetGeometry").GetEBO());
        OpenGL::BindSSBO(2, "ViewportData");
        OpenGL::BindSSBO(3, "InstanceData");
        OpenGL::BindSSBO(4, "Samplers");
        OpenGL::BindSSBO(5, "RendererData");

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
        OpenGLRasterizerStateManager::SetRasterizerState(state);
        RenderFullscreenTriangle();

        state.stencilRef = STENCIL_BIT_SKINNED_HAIR;
        OpenGLRasterizerStateManager::SetRasterizerState(state);
        RenderFullscreenTriangle();
    }

    void MaterialResolveProceduralPass() {
        ProfilerOpenGLZoneFunction();

        MeshBuffer& proceduralMeshBuffer = ResourceManager::GetMeshBuffer("Procedural");

        OpenGLFrameBuffer& gbufferFbo = OpenGL::ResourceManager::GetFrameBuffer("GBufferRE");
        gbufferFbo.Bind();
        gbufferFbo.SetViewport();
        gbufferFbo.DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "VelocityXYOcclusionSubSurface" });

        OpenGL::BindShader("MaterialResolve");

        OpenGL::BindImageTexture(0, gbufferFbo.GetColorAttachmentHandleByName("Visibility"), GL_READ_ONLY, GL_RG32UI);
        OpenGL::BindTextureUnit(1, gbufferFbo.GetDepthAttachmentHandle());

        OpenGL::BindSSBO(0, proceduralMeshBuffer.GetVBO());
        OpenGL::BindSSBO(1, proceduralMeshBuffer.GetEBO());
        OpenGL::BindSSBO(2, "ViewportData");
        OpenGL::BindSSBO(3, "InstanceData");
        OpenGL::BindSSBO(4, "Samplers");
        OpenGL::BindSSBO(5, "RendererData");

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

        OpenGLRasterizerStateManager::SetRasterizerState(state);
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

        OpenGL::BindSSBO(0, OpenGL::BackEnd::GetSkinnedVertexDataVBO());
        OpenGL::BindSSBO(1, ResourceManager::GetMeshBuffer("AssetGeometry").GetEBO());
        OpenGL::BindSSBO(2, "ViewportData");
        OpenGL::BindSSBO(3, "InstanceData");
        OpenGL::BindSSBO(4, "Samplers");
        OpenGL::BindSSBO(5, "RendererData");

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

        OpenGLRasterizerStateManager::SetRasterizerState(state);
        RenderFullscreenTriangle();
    }
}
