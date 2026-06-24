#include "API/OpenGL/Renderer/GL_renderer.h"
#include "API/OpenGL/GL_backend.h"
#include "BackEnd/BackEnd.h"
#include "Config/Config.h"
#include "Viewport/ViewportManager.h"
#include "Renderer/RenderDataManager.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "UI/UIBackEnd.h"

using namespace Hell;

namespace OpenGLRenderer {

    void UIPass() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& presentFbo = GetFrameBuffer("Present");

        const Resolutions& resolutions = Config::GetResolutions();

        presentFbo.Bind();
        presentFbo.SetViewport();
        presentFbo.DrawBuffer("Color");

        BindShader("UI");

        BindSSBO(0, "Samplers");
        BindSSBO(5, "RenderItemsUI");

        SetUniformFloat("u_renderTargetWidth", resolutions.ui.x);
        SetUniformFloat("u_renderTargetHeight", resolutions.ui.y);

        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_CLIP_DISTANCE0);
        glEnable(GL_CLIP_DISTANCE1);
        glEnable(GL_CLIP_DISTANCE2);
        glEnable(GL_CLIP_DISTANCE3);

        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);

        GenericMesh& genericMesh = ResourceManager::GetGenericMesh("UI");
        glBindVertexArray(genericMesh.GetVAO());

        const std::vector<DrawIndexedIndirectCommand>& drawCommands = RenderDataManager::GetDrawCommandsUI();
        MultiDrawIndirect(drawCommands);

        glDisable(GL_CLIP_DISTANCE0);
        glDisable(GL_CLIP_DISTANCE1);
        glDisable(GL_CLIP_DISTANCE2);
        glDisable(GL_CLIP_DISTANCE3);

        glBindVertexArray(0);
    }
}
