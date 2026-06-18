#include "API/OpenGL/Renderer/GL_renderer.h"
#include "API/OpenGL/GL_backend.h"
#include "Hell/RendereringConstants.h"
#include "Managers/ResourceManager.h"
#include "Renderer/RenderDataManager.h"
#include "Renderer/Renderer.h"

namespace OpenGLRenderer {

    void VisibilityPass() {
        ProfilerOpenGLZoneFunction();
        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();

        OpenGLFrameBuffer& fbo = GetFrameBuffer("GBufferRE");
        fbo.Bind();
        fbo.DrawBuffers({ "Visibility" });

        BindShader("Visibility");
        
        BindSSBO(2, "ViewportData");
        BindSSBO(3, "InstanceData");

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.blendEnable = false;
        state.cullfaceEnable = true;
        state.depthMask = true;
        state.colorMask = true;
        state.depthFunc = GL_GREATER;

        state.stencilTestEnabled = true;
        state.stencilFunc = GL_ALWAYS;
        state.stencilReadMask = 0xFF;
        state.stencilWriteMask = 0xFF;
        state.stencilFailOp = GL_KEEP;
        state.stencilDepthFailOp = GL_KEEP;
        state.stencilPassOp = GL_REPLACE;

        state.stencilRef = STENCIL_BIT_PROCEDUAL;

        glBindVertexArray(ResourceManager::GetMeshBuffer("Procedural").GetVAO());
        MultiDrawPerViewportRE(fbo, drawInfoSet.procedural, state);

        state.stencilRef = STENCIL_BIT_STATIC;

        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
        MultiDrawPerViewportRE(fbo, drawInfoSet.standard, state);
        MultiDrawPerViewportRE(fbo, drawInfoSet.skinnedNonDeformingStandard, state);
    }

    void VisibilityAlphaDiscardPass() {
        ProfilerOpenGLZoneFunction();
        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();

        OpenGLFrameBuffer& fbo = GetFrameBuffer("GBufferRE");
        fbo.Bind();
        fbo.DrawBuffers({ "Visibility" });

        static int frameCount = 0;
        frameCount++;

        BindShader("VisibilityAlphaDiscard");
        SetUniformUInt("u_frameCount", frameCount);

        BindSSBO(0, "Samplers");
        BindSSBO(2, "ViewportData");
        BindSSBO(3, "InstanceData");

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = true;
        state.colorMask = true;
        state.depthFunc = GL_GREATER;

        state.stencilTestEnabled = true;
        state.stencilFunc = GL_ALWAYS;
        state.stencilReadMask = 0xFF;
        state.stencilWriteMask = 0xFF;
        state.stencilFailOp = GL_KEEP;
        state.stencilDepthFailOp = GL_KEEP;
        state.stencilPassOp = GL_REPLACE;

        state.stencilRef = STENCIL_BIT_STATIC;

        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());

        MultiDrawPerViewportRE(fbo, drawInfoSet.alphaDiscard, state);
        MultiDrawPerViewportRE(fbo, drawInfoSet.skinnedNonDeformingAlphaDiscard, state);

        // Hair
        SetUniformBool("u_depthOffset", true);
        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
        MultiDrawPerViewportRE(fbo, drawInfoSet.hair, state);
        SetUniformBool("u_depthOffset", false);
    }

    void VisibilitySkinnedPass() {
        ProfilerOpenGLZoneFunction();
        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();

        OpenGLFrameBuffer& fbo = GetFrameBuffer("GBufferRE");
        fbo.Bind();
        fbo.DrawBuffers({ "Visibility" });

        BindShader("Visibility");

        BindSSBO(2, "ViewportData");
        BindSSBO(3, "InstanceData");

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.blendEnable = false;
        state.cullfaceEnable = true;
        state.depthMask = true;
        state.colorMask = true;
        state.depthFunc = GL_GREATER;

        state.stencilTestEnabled = true;
        state.stencilFunc = GL_ALWAYS;
        state.stencilReadMask = 0xFF;
        state.stencilWriteMask = 0xFF;
        state.stencilFailOp = GL_KEEP;
        state.stencilDepthFailOp = GL_KEEP;
        state.stencilPassOp = GL_REPLACE;

        state.stencilRef = STENCIL_BIT_SKINNED;

        glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
        MultiDrawPerViewportRE(fbo, drawInfoSet.skinnedStandard, state);
    }

    void VisibilitySkinnedHairPass() {
        ProfilerOpenGLZoneFunction();
        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();

        OpenGLFrameBuffer& fbo = GetFrameBuffer("GBufferRE");
        fbo.Bind();
        fbo.DrawBuffers({ "Visibility" });

        BindShader("VisibilityAlphaDiscard");

        BindSSBO(2, "ViewportData");
        BindSSBO(3, "InstanceData");

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = true;
        state.colorMask = true;
        state.depthFunc = GL_GREATER;

        state.stencilTestEnabled = true;
        state.stencilFunc = GL_ALWAYS;
        state.stencilReadMask = 0xFF;
        state.stencilWriteMask = 0xFF;
        state.stencilFailOp = GL_KEEP;
        state.stencilDepthFailOp = GL_KEEP;
        state.stencilPassOp = GL_REPLACE;
        
        state.stencilRef = STENCIL_BIT_SKINNED_HAIR;

        SetUniformBool("u_depthOffset", true);

        glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
        MultiDrawPerViewportRE(fbo, drawInfoSet.skinnedHair, state);

        SetUniformBool("u_depthOffset", false);
    }
}