#include "../GL_renderer.h" 
#include "../../GL_backend.h"
#include "Editor/Editor.h"
#include "Renderer/RenderDataManager.h"
#include "Viewport/ViewportManager.h"
#include "Hell/ResourceManagement/ResourceManager.h"

namespace OpenGLRenderer {

    void SkyBoxPass() {
        if (Editor::IsOpen()) return;

        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gBuffer = GetFrameBuffer("GBuffer");
        OpenGLCubemapView* skyboxCubemapView = GetCubemapViewOLD("SkyboxNightSky");

        gBuffer.Bind();
        gBuffer.SetViewport();
        gBuffer.DrawBuffers({ "Lighting" });

        BindShader("SkyboxRE");

        OpenGLRasterizerState state;
        state.depthTestEnabled = false;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.depthFunc = GL_GREATER;

        SetRasterizerState(state);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxCubemapView->GetHandle());
        glBindVertexArray(Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());

        RenderFullscreenTriangle();
    }
}
