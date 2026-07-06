#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Hell/Backend/BackEnd.h"
#include "Unloved/Config/Config.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Hell/UI/UIBackEnd.h"

namespace OpenGL::Renderer {

    void UIPass() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& presentFbo = OpenGL::ResourceManager::GetFrameBuffer("Present");

        const Resolutions& resolutions = Config::GetResolutions();

        presentFbo.Bind();
        presentFbo.SetViewport();
        presentFbo.DrawBuffer("Color");

        OpenGL::BindShader("UI");

        OpenGL::BindSSBO(0, "Samplers");
        OpenGL::BindSSBO(6, "RenderItemsUI");

        OpenGL::SetUniformFloat("u_renderTargetWidth", resolutions.ui.x);
        OpenGL::SetUniformFloat("u_renderTargetHeight", resolutions.ui.y);

        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_CLIP_DISTANCE0);
        glEnable(GL_CLIP_DISTANCE1);
        glEnable(GL_CLIP_DISTANCE2);
        glEnable(GL_CLIP_DISTANCE3);

        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);

        OpenGLGenericMesh& genericMesh = OpenGL::ResourceManager::GetGenericMesh("UI");
        glBindVertexArray(genericMesh.GetVAO());

        const std::vector<DrawIndexedIndirectCommand>& drawCommands = Unloved::RenderDataManager::GetDrawCommandsUI();
        MultiDrawIndirect(drawCommands);

        glDisable(GL_CLIP_DISTANCE0);
        glDisable(GL_CLIP_DISTANCE1);
        glDisable(GL_CLIP_DISTANCE2);
        glDisable(GL_CLIP_DISTANCE3);

        glBindVertexArray(0);
    }
}
