#include "../GL_renderer.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Unloved/Editor/Editor.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Hell/ResourceManagement/ResourceManager.h"

namespace OpenGLRenderer {

    void SkyBoxPass() {
        if (Unloved::Editor::IsOpen()) return;

        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        OpenGLCubemapView* skyboxCubemapView = OpenGL::ResourceManager::GetCubemapViewPtr("SkyboxNightSky");

        gBuffer.Bind();
        gBuffer.SetViewport();
        gBuffer.DrawBuffers({ "Lighting" });

        OpenGL::BindShader("SkyboxRE");

        OpenGLRasterizerState state;
        state.depthTestEnabled = false;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.depthFunc = GL_GREATER;

        OpenGLRasterizerStateManager::SetRasterizerState(state);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxCubemapView->GetHandle());
        glBindVertexArray(Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());

        RenderFullscreenTriangle();
    }
}
