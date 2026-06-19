#include "API/OpenGL/GL_backend.h" 
#include "API/OpenGL/Renderer/GL_renderer.h" 
#include "Renderer/RenderDataManager.h"
#include "UI/UIBackEnd.h"

namespace OpenGLRenderer {

    void RenderLoadingScreen() {
        const std::vector<GLuint64>& samplers = OpenGLBackEnd::GetBindlessTextureIDs();
        UpdateSSBO("Samplers", sizeof(GLuint64) * samplers.size(), samplers.data());

        const std::vector<RenderItemUI>& renderItemsUI = UIBackEnd::GetRenderItems();
        UpdateSSBO("RenderItemsUI", renderItemsUI.size() * sizeof(RenderItemUI), renderItemsUI.data());

        OpenGLFrameBuffer& presentFbo = GetFrameBuffer("Present");
        presentFbo.ClearAttachment("Color", 0.0f, 0.0f, 0.0f, 0.0f);

        UIPass();

        OpenGLRenderer::BlitToDefaultFrameBuffer(&presentFbo, "Color", GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
}