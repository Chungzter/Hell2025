#include "API/OpenGL/Renderer/GL_renderer.h"
#include "API/OpenGL/GL_backend.h"
#include "Hell/RendereringConstants.h"
#include "Renderer/Renderer.h"

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

        OpenGLRasterizerState resolveState;
        resolveState.depthTestEnabled = false;
        resolveState.blendEnable = false;
        resolveState.cullfaceEnable = false;
        resolveState.depthMask = false;
        resolveState.colorMask = true;

        resolveState.stencilTestEnabled = true;
        resolveState.stencilFunc = GL_EQUAL;
        resolveState.stencilRef = STENCIL_REF_STATIC;
        resolveState.stencilReadMask = 0xFF;
        resolveState.stencilWriteMask = 0x00;
        resolveState.stencilFailOp = GL_KEEP;
        resolveState.stencilDepthFailOp = GL_KEEP;
        resolveState.stencilPassOp = GL_KEEP;

        SetRasterizerState(resolveState);
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

        OpenGLRasterizerState resolveState;
        resolveState.depthTestEnabled = false;
        resolveState.blendEnable = false;
        resolveState.cullfaceEnable = false;
        resolveState.depthMask = false;
        resolveState.colorMask = true;

        resolveState.stencilTestEnabled = true;
        resolveState.stencilFunc = GL_EQUAL;
        resolveState.stencilRef = STENCIL_REF_SKINNED;
        resolveState.stencilReadMask = 0xFF;
        resolveState.stencilWriteMask = 0x00;
        resolveState.stencilFailOp = GL_KEEP;
        resolveState.stencilDepthFailOp = GL_KEEP;
        resolveState.stencilPassOp = GL_KEEP;

        SetRasterizerState(resolveState);
        RenderFullscreenTriangle();
    }

    void MaterialResolveProceduralPass() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gbufferFbo = GetFrameBuffer("GBufferRE");
        gbufferFbo.Bind();
        gbufferFbo.SetViewport();
        gbufferFbo.DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "VelocityXYOcclusionSubSurface" });

        BindShader("MaterialResolve");

        BindImageTexture(0, gbufferFbo.GetColorAttachmentHandleByName("Visibility"), GL_READ_ONLY, GL_RG32UI);
        BindTextureUnit(1, gbufferFbo.GetDepthAttachmentHandle());

        BindSSBO(0, Renderer::GetProceduralMeshBuffer().GetVBO());
        BindSSBO(1, Renderer::GetProceduralMeshBuffer().GetEBO());
        BindSSBO(2, "ViewportData");
        BindSSBO(3, "InstanceData");
        BindSSBO(4, "Samplers");
        BindSSBO(5, "RendererData");

        OpenGLRasterizerState resolveState;
        resolveState.depthTestEnabled = false;
        resolveState.blendEnable = false;
        resolveState.cullfaceEnable = false;
        resolveState.depthMask = false;
        resolveState.colorMask = true;

        resolveState.stencilTestEnabled = true;
        resolveState.stencilFunc = GL_EQUAL;
        resolveState.stencilRef = STENCIL_REF_PROCEDUAL;
        resolveState.stencilReadMask = 0xFF;
        resolveState.stencilWriteMask = 0x00;
        resolveState.stencilFailOp = GL_KEEP;
        resolveState.stencilDepthFailOp = GL_KEEP;
        resolveState.stencilPassOp = GL_KEEP;

        SetRasterizerState(resolveState);
        RenderFullscreenTriangle();
    }

    void MaterialResolveSkinnedHairPass() {
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

        OpenGLRasterizerState resolveState;
        resolveState.depthTestEnabled = false;
        resolveState.blendEnable = false;
        resolveState.cullfaceEnable = false;
        resolveState.depthMask = false;
        resolveState.colorMask = true;

        resolveState.stencilTestEnabled = true;
        resolveState.stencilFunc = GL_EQUAL;
        resolveState.stencilRef = STENCIL_REF_SKINNED_HAIR;
        resolveState.stencilReadMask = 0xFF;
        resolveState.stencilWriteMask = 0x00;
        resolveState.stencilFailOp = GL_KEEP;
        resolveState.stencilDepthFailOp = GL_KEEP;
        resolveState.stencilPassOp = GL_KEEP;

        SetRasterizerState(resolveState);
        RenderFullscreenTriangle();
    }
}