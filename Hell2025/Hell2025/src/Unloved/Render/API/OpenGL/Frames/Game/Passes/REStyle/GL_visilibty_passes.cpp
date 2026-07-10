#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/Renderer.h"

namespace OpenGL::Renderer {

    void VisibilityPass() {
        ProfilerOpenGLZoneFunction();
        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();
        OpenGLMeshBuffer& meshBufferProcedural = OpenGL::ResourceManager::GetMeshBuffer("Procedural");
        OpenGLMeshBuffer& meshBufferAssets = OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry");

        OpenGLFrameBuffer& fbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        fbo.Bind();
        fbo.DrawBuffers({ "Visibility" });

        OpenGL::BindShader("Visibility");

        OpenGL::BindSSBO(3, "ViewportData");
        OpenGL::BindSSBO(4, "InstanceData");

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

        glBindVertexArray(meshBufferProcedural.GetVAO());
        MultiDrawPerViewportRE(fbo, drawInfoSet.procedural, state);

        state.stencilRef = STENCIL_BIT_ASSET;

        glBindVertexArray(meshBufferAssets.GetVAO());
        MultiDrawPerViewportRE(fbo, drawInfoSet.standard, state);
        MultiDrawPerViewportRE(fbo, drawInfoSet.skinnedNonDeformingStandard, state);
    }

    void VisibilityAlphaDiscardPass() {
        ProfilerOpenGLZoneFunction();
        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();
        OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry");

        OpenGLFrameBuffer& fbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        fbo.Bind();
        fbo.DrawBuffers({ "Visibility" });

        static int frameCount = 0;
        frameCount++;

        OpenGL::BindShader("VisibilityAlphaDiscard");
        OpenGL::SetUniformUInt("u_frameCount", frameCount);

        OpenGL::BindSSBO(0, "Samplers");
        OpenGL::BindSSBO(3, "ViewportData");
        OpenGL::BindSSBO(4, "InstanceData");

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

        state.stencilRef = STENCIL_BIT_ASSET;

        glBindVertexArray(meshBuffer.GetVAO());

        MultiDrawPerViewportRE(fbo, drawInfoSet.alphaDiscard, state);
        MultiDrawPerViewportRE(fbo, drawInfoSet.skinnedNonDeformingAlphaDiscard, state);

        // Hair
        OpenGL::SetUniformBool("u_depthOffset", true);
        glBindVertexArray(meshBuffer.GetVAO());
        MultiDrawPerViewportRE(fbo, drawInfoSet.hair, state);
        OpenGL::SetUniformBool("u_depthOffset", false);
    }

    void VisibilitySkinnedPass() {
        ProfilerOpenGLZoneFunction();
        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();

        OpenGLFrameBuffer& fbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        fbo.Bind();
        fbo.DrawBuffers({ "Visibility" });

        OpenGL::BindShader("Visibility");

        OpenGL::BindSSBO(3, "ViewportData");
        OpenGL::BindSSBO(4, "InstanceData");

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

        glBindVertexArray(OpenGL::BackEnd::GetSkinnedVertexDataVAO());
        MultiDrawPerViewportRE(fbo, drawInfoSet.skinnedStandard, state);
    }

    void VisibilitySkinnedHairPass() {
        ProfilerOpenGLZoneFunction();
        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();

        OpenGLFrameBuffer& fbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        fbo.Bind();
        fbo.DrawBuffers({ "Visibility" });

        OpenGL::BindShader("VisibilityAlphaDiscard");

        OpenGL::BindSSBO(3, "ViewportData");
        OpenGL::BindSSBO(4, "InstanceData");

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

        OpenGL::SetUniformBool("u_depthOffset", true);

        glBindVertexArray(OpenGL::BackEnd::GetSkinnedVertexDataVAO());
        MultiDrawPerViewportRE(fbo, drawInfoSet.skinnedHair, state);

        OpenGL::SetUniformBool("u_depthOffset", false);
    }
}
