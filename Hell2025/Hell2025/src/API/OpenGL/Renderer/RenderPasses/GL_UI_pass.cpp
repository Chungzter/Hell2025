#include "API/OpenGL/Renderer/GL_renderer.h"
#include "API/OpenGL/GL_backend.h"

#include "AssetManagement/AssetManager.h"
#include "BackEnd/BackEnd.h"
#include "Config/Config.h"
#include "Viewport/ViewportManager.h"
#include "Renderer/RenderDataManager.h"
#include "ResourceManagement/ResourceManager.h"
#include "UI/UIBackEnd.h"

namespace OpenGLRenderer {
    GLint g_quadVAO = 0;
    GLuint g_linearSampler = 0;
    GLuint g_nearestSampler = 0;

    void UIPass() {
        ProfilerOpenGLZoneFunction();

        // First blit the final image into the UI fbo, which is double the size. UI has double resolution as the main game
        OpenGLFrameBuffer& finalImageFbo = GetFrameBuffer("FinalImage");
        OpenGLFrameBuffer& uiFbo = GetFrameBuffer("UI");
        OpenGLRenderer::BlitFrameBuffer(&finalImageFbo, &uiFbo, "Color", "Color", GL_COLOR_BUFFER_BIT, GL_NEAREST);

        const Resolutions& resolutions = Config::GetResolutions();

        OpenGLShader* shader = GetShaderOLD("UI");
        OpenGLFrameBuffer* uiFrameBuffer = GetFrameBufferOLD("UI");

        if (!shader) return;
        if (!uiFrameBuffer) return;

        if (g_linearSampler == 0) {
            glGenSamplers(1, &g_linearSampler); 
            glSamplerParameteri(g_linearSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glSamplerParameteri(g_linearSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glGenSamplers(1, &g_nearestSampler); 
            glSamplerParameteri(g_nearestSampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glSamplerParameteri(g_nearestSampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }


        uiFrameBuffer->Bind();
        uiFrameBuffer->SetViewport();
        //uiFrameBuffer->PrintCacheDebugInfo();
        uiFrameBuffer->DrawBuffer("Color");
        shader->Bind();

        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_CLIP_DISTANCE0);
        glEnable(GL_CLIP_DISTANCE1);
        glEnable(GL_CLIP_DISTANCE2);
        glEnable(GL_CLIP_DISTANCE3);

        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);

        shader->SetFloat("u_renderTargetWidth", resolutions.ui.x);
        shader->SetFloat("u_renderTargetHeight", resolutions.ui.y);

        GenericMesh& genericMesh = ResourceManager::GetGenericMesh("UI");
        glBindVertexArray(genericMesh.GetVAO());

        int lastFilter = -1; // -1 = unknown, 0 = linear, 1 = nearest

        for (const RenderItemUI& renderItem : UIBackEnd::GetRenderItems()) {

            OpenGLTexture& glTexture = AssetManager::GetTextureByIndex(renderItem.textureIndex)->GetGLTexture();
            glBindTextureUnit(0, glTexture.GetHandle());

            if (renderItem.filter != lastFilter) {
                switch (renderItem.filter) {
                    case 0: glBindSampler(0, g_linearSampler);  break;
                    case 1: glBindSampler(0, g_nearestSampler); break;
                }
                lastFilter = renderItem.filter;
            }
            
            shader->SetFloat("u_clipMinX", renderItem.clipMinX);
            shader->SetFloat("u_clipMinY", renderItem.clipMinY);
            shader->SetFloat("u_clipMaxX", renderItem.clipMaxX);
            shader->SetFloat("u_clipMaxY", renderItem.clipMaxY);

            glDrawElementsInstancedBaseVertex(GL_TRIANGLES, renderItem.indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * renderItem.baseIndex), 1, renderItem.baseVertex);
        }

        glDisable(GL_CLIP_DISTANCE0);
        glDisable(GL_CLIP_DISTANCE1);
        glDisable(GL_CLIP_DISTANCE2);
        glDisable(GL_CLIP_DISTANCE3);

        glBindVertexArray(0);

        // Blit this image into the default framebuffer. It is the last point of call. Probably clean this up at some point because it is kinda hidden in here.
        OpenGLRenderer::BlitToDefaultFrameBuffer(&uiFbo, "Color", GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
}