#include "API/OpenGL/Renderer/GL_renderer.h"
#include "API/OpenGL/GL_backend.h"
#include "Hell/RendereringConstants.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Renderer/Renderer.h"

using namespace Hell;

namespace OpenGLRenderer {

    void MaterialResolvePass() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gbufferFbo = GetFrameBuffer("GBufferRE");
        gbufferFbo.Bind();
        gbufferFbo.SetViewport();
        gbufferFbo.DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "VelocityXYOcclusionSubSurface" });

        BindShader("MaterialResolve");

        BindImageTexture(0, gbufferFbo.GetColorAttachmentHandleByName("Visibility"), GL_READ_ONLY, GL_RG32UI);
        BindTextureUnit(1, gbufferFbo.GetDepthAttachmentHandle());

        BindSSBO(0, OpenGLBackEnd::GetVertexDataVBO());
        BindSSBO(1, OpenGLBackEnd::GetVertexDataEBO());
        BindSSBO(2, "ViewportData");
        BindSSBO(3, "InstanceData");
        BindSSBO(4, "Samplers");
        BindSSBO(5, "RendererData");

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

        SetRasterizerState(state);
        RenderFullscreenTriangle();
    }

    void MaterialResolveSkinnedPass() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gbufferFbo = GetFrameBuffer("GBufferRE");
        gbufferFbo.Bind();
        gbufferFbo.SetViewport();
        gbufferFbo.DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "VelocityXYOcclusionSubSurface" });

        BindShader("MaterialResolveSkinning");

        BindImageTexture(0, gbufferFbo.GetColorAttachmentHandleByName("Visibility"), GL_READ_ONLY, GL_RG32UI);
        BindTextureUnit(1, gbufferFbo.GetDepthAttachmentHandle());

        BindSSBO(0, OpenGLBackEnd::GetSkinnedVertexDataVBO());
        BindSSBO(1, OpenGLBackEnd::GetVertexDataEBO());
        BindSSBO(2, "ViewportData");
        BindSSBO(3, "InstanceData");
        BindSSBO(4, "Samplers");
        BindSSBO(5, "RendererData");

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
        SetRasterizerState(state);
        RenderFullscreenTriangle();

        state.stencilRef = STENCIL_BIT_SKINNED_HAIR;
        SetRasterizerState(state);
        RenderFullscreenTriangle();
    }

    void MaterialResolveProceduralPass() {
        ProfilerOpenGLZoneFunction();

        MeshBuffer& proceduralMeshBuffer = ResourceManager::GetMeshBuffer("Procedural");

        OpenGLFrameBuffer& gbufferFbo = GetFrameBuffer("GBufferRE");
        gbufferFbo.Bind();
        gbufferFbo.SetViewport();
        gbufferFbo.DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "VelocityXYOcclusionSubSurface" });

        BindShader("MaterialResolve");

        BindImageTexture(0, gbufferFbo.GetColorAttachmentHandleByName("Visibility"), GL_READ_ONLY, GL_RG32UI);
        BindTextureUnit(1, gbufferFbo.GetDepthAttachmentHandle());

        BindSSBO(0, proceduralMeshBuffer.GetVBO());
        BindSSBO(1, proceduralMeshBuffer.GetEBO());
        BindSSBO(2, "ViewportData");
        BindSSBO(3, "InstanceData");
        BindSSBO(4, "Samplers");
        BindSSBO(5, "RendererData");

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

        SetRasterizerState(state);
        RenderFullscreenTriangle();
    }

    void HairLightingSkinnedResolvePass() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gbufferFbo = GetFrameBuffer("GBufferRE");
        gbufferFbo.Bind();
        gbufferFbo.SetViewport();
        gbufferFbo.DrawBuffers({ "Lighting", "BaseColorMetallic", "NormalXYRoughnessMisc", "VelocityXYOcclusionSubSurface" });

        BindShader("HairLightingResolve");

        BindImageTexture(0, gbufferFbo.GetColorAttachmentHandleByName("Visibility"), GL_READ_ONLY, GL_RG32UI);
        BindTextureUnit(1, gbufferFbo.GetDepthAttachmentHandle());

        BindSSBO(0, OpenGLBackEnd::GetSkinnedVertexDataVBO());
        BindSSBO(1, OpenGLBackEnd::GetVertexDataEBO());
        BindSSBO(2, "ViewportData");
        BindSSBO(3, "InstanceData");
        BindSSBO(4, "Samplers");
        BindSSBO(5, "RendererData");

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

        SetRasterizerState(state);
        RenderFullscreenTriangle();
    }
}