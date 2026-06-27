#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Hell/Common/String.h"
#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Viewport/ViewportManager.h"


// todo remove
#include "Unloved/Debug/Debug.h"
#include "Renderer/Renderer.h"
#include "Hell/Input.h"
namespace Input = Hell::Input;

namespace {
    std::string GetBlurBufferName(int viewportIndex, int bufferIndex) {
        return "BlurBuffer_" + std::to_string(viewportIndex) + "_" + std::to_string(bufferIndex);
    }
}

namespace OpenGLRenderer {

    void EmissivePass() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer* gBuffer = nullptr;
        std::string outputTextureName = UNDEFINED_STRING;

        if (Renderer::GetRendererMode() == RendererMode::OLD_DEFERRED) {
            gBuffer = &OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
            outputTextureName = "Lighting";
        }
        if (Renderer::GetRendererMode() == RendererMode::RE_STYLE) {
            gBuffer = &OpenGL::ResourceManager::GetFrameBuffer("GBufferRE");
            outputTextureName = "Lighting";
        }
        if (!gBuffer) return;

        static bool old = true;

        if (Input::KeyPressed(HELL_KEY_NUMPAD_4)) {
            old = !old;
            Debug::BlitQuickDebugMessage("OLD: " + Hell::String::FormatBool(old));
        }

        if (old) {
            OpenGLRasterizerStateManager::ForceRasterizerState("EmissivePass");


            //OpenGLFrameBuffer* finalImageFBO = OpenGL::ResourceManager::GetFrameBufferPtr("FinalImage");
            OpenGLShader* horizontalShader = OpenGL::ResourceManager::GetShaderPtr("BlurHorizontal");
            OpenGLShader* verticalShader = OpenGL::ResourceManager::GetShaderPtr("BlurVertical");
            OpenGLShader* compositeShader = OpenGL::ResourceManager::GetShaderPtr("EmissiveComposite");

            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);

            for (int i = 0; i < 4; i++) {
                Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
                if (!viewport->IsVisible()) continue;

                OpenGLFrameBuffer* blurBuffer = OpenGL::ResourceManager::GetFrameBufferPtr(GetBlurBufferName(i, 0));

                BlitRect srcRect = OpenGLRenderer::BlitRectFromFrameBufferViewport(gBuffer, viewport);
                BlitRect dstRect;
                dstRect.x0 = 0;
                dstRect.x1 = blurBuffer->GetWidth();
                dstRect.y0 = 0;
                dstRect.y1 = blurBuffer->GetHeight();

                OpenGL::BlitFrameBuffer(gBuffer, blurBuffer, "Emissive", "ColorA", srcRect, dstRect, GL_COLOR_BUFFER_BIT, GL_LINEAR);

                // First round blur (vertical)
                blurBuffer->Bind();
                blurBuffer->SetViewport();
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, blurBuffer->GetColorAttachmentHandleByName("ColorA"));
                glDrawBuffer(GL_COLOR_ATTACHMENT1);
                OpenGL::BindShader("BlurVertical");
                OpenGL::SetUniformFloat("targetHeight", blurBuffer->GetHeight());
                DrawFullscreenTriangle();

                for (int j = 1; j < 4; j++) {

                    GLuint horizontalSourceHandle = OpenGL::ResourceManager::GetFrameBufferPtr(GetBlurBufferName(i, j - 1))->GetColorAttachmentHandleByName("ColorB");
                    GLuint verticalSourceHandle = OpenGL::ResourceManager::GetFrameBufferPtr(GetBlurBufferName(i, j))->GetColorAttachmentHandleByName("ColorA");

                    OpenGL::ResourceManager::GetFrameBufferPtr(GetBlurBufferName(i, j))->Bind();
                    OpenGL::ResourceManager::GetFrameBufferPtr(GetBlurBufferName(i, j))->SetViewport();

                    // Second round blur (horizontal)
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, horizontalSourceHandle);
                    glDrawBuffer(GL_COLOR_ATTACHMENT0);
                    OpenGL::BindShader("BlurHorizontal");
                    OpenGL::SetUniformFloat("targetWidth", OpenGL::ResourceManager::GetFrameBufferPtr(GetBlurBufferName(i, j))->GetWidth());
                    DrawFullscreenTriangle();

                    // Second round blur (vertical)
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, verticalSourceHandle);
                    glDrawBuffer(GL_COLOR_ATTACHMENT1);
                    OpenGL::BindShader("BlurVertical");
                    OpenGL::SetUniformFloat("targetHeight", OpenGL::ResourceManager::GetFrameBufferPtr(GetBlurBufferName(i, j))->GetHeight());
                    DrawFullscreenTriangle();
                }

                // Composite those blurred textures into the main lighting image
                float viewportWidth = gBuffer->GetWidth() * viewport->GetSize().x;
                float viewportHeight = gBuffer->GetHeight() * viewport->GetSize().y;
                float viewportOffsetX = gBuffer->GetWidth() * viewport->GetPosition().x;
                float viewportOffsetY = gBuffer->GetHeight() * viewport->GetPosition().y;

                OpenGL::BindShader("EmissiveComposite");
                OpenGL::SetUniformInt("u_viewportIndex", i);

                glBindImageTexture(0, gBuffer->GetColorAttachmentHandleByName(outputTextureName), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
                glBindTextureUnit(1, OpenGL::ResourceManager::GetFrameBufferPtr(GetBlurBufferName(i, 0))->GetColorAttachmentHandleByName("ColorB"));
                glBindTextureUnit(2, OpenGL::ResourceManager::GetFrameBufferPtr(GetBlurBufferName(i, 1))->GetColorAttachmentHandleByName("ColorB"));
                glBindTextureUnit(3, OpenGL::ResourceManager::GetFrameBufferPtr(GetBlurBufferName(i, 2))->GetColorAttachmentHandleByName("ColorB"));
                glBindTextureUnit(4, OpenGL::ResourceManager::GetFrameBufferPtr(GetBlurBufferName(i, 3))->GetColorAttachmentHandleByName("ColorB"));

                int dispatchX = static_cast<int>(std::ceil(viewportWidth / 16.0f));
                int dispatchY = static_cast<int>(std::ceil(viewportHeight / 4.0f));
                OpenGL::DispatchCompute(dispatchX, dispatchY, 1);
            }
        }
        else {
            ProfilerOpenGLZoneFunction();
            OpenGLRasterizerStateManager::ForceRasterizerState("EmissivePass");

            OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
            OpenGLFrameBuffer* emissiveBlurFBO = OpenGL::ResourceManager::GetFrameBufferPtr("EmissiveBlur");
            OpenGLShader* horizontalShader = OpenGL::ResourceManager::GetShaderPtr("BlurHorizontal");
            OpenGLShader* verticalShader = OpenGL::ResourceManager::GetShaderPtr("BlurVertical");
            OpenGLShader* compositeShader = OpenGL::ResourceManager::GetShaderPtr("EmissiveCompositeNew");

            GLuint handleA = emissiveBlurFBO->GetColorAttachmentHandleByName("ColorA");
            GLuint handleB = emissiveBlurFBO->GetColorAttachmentHandleByName("ColorB");

            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);

            float fullWidth = gBuffer->GetWidth();
            float fullHeight = gBuffer->GetHeight();

            // blit initial emissive data to mip 0
            emissiveBlurFBO->Bind();
            emissiveBlurFBO->SetColorAttachmentMipLevel("ColorA", 0);
            emissiveBlurFBO->DrawBuffer("ColorA");

            BlitRect fullScreenRect;
            fullScreenRect.x0 = 0;
            fullScreenRect.x1 = fullWidth;
            fullScreenRect.y0 = 0;
            fullScreenRect.y1 = fullHeight;

            OpenGL::BlitFrameBuffer(gBuffer, emissiveBlurFBO, "Emissive", "ColorA", fullScreenRect, fullScreenRect, GL_COLOR_BUFFER_BIT, GL_LINEAR);

            // perform vertical blur only for base mip
            glViewport(0, 0, fullWidth, fullHeight);
            emissiveBlurFBO->SetColorAttachmentMipLevel("ColorB", 0);
            emissiveBlurFBO->DrawBuffer("ColorB");

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, handleA);
            glTextureParameteri(handleA, GL_TEXTURE_BASE_LEVEL, 0);
            glTextureParameteri(handleA, GL_TEXTURE_MAX_LEVEL, 0);

            OpenGL::BindShader("BlurVertical");
            OpenGL::SetUniformFloat("targetHeight", fullHeight);
            DrawFullscreenTriangle();

            for (int mip = 1; mip < 4; mip++) {
                float currentW = fullWidth / std::pow(2.0f, mip);
                float currentH = fullHeight / std::pow(2.0f, mip);

                glViewport(0, 0, currentW, currentH);

                // downsample horizontally from previous mip b to current mip a
                emissiveBlurFBO->SetColorAttachmentMipLevel("ColorA", mip);
                emissiveBlurFBO->DrawBuffer("ColorA");

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, handleB);
                glTextureParameteri(handleB, GL_TEXTURE_BASE_LEVEL, mip - 1);
                glTextureParameteri(handleB, GL_TEXTURE_MAX_LEVEL, mip - 1);

                OpenGL::BindShader("BlurHorizontal");
                OpenGL::SetUniformFloat("targetWidth", currentW);
                DrawFullscreenTriangle();

                // perform vertical blur on current mip
                emissiveBlurFBO->SetColorAttachmentMipLevel("ColorB", mip);
                emissiveBlurFBO->DrawBuffer("ColorB");

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, handleA);
                glTextureParameteri(handleA, GL_TEXTURE_BASE_LEVEL, mip);
                glTextureParameteri(handleA, GL_TEXTURE_MAX_LEVEL, mip);

                OpenGL::BindShader("BlurVertical");
                OpenGL::SetUniformFloat("targetHeight", currentH);

                DrawFullscreenTriangle();
            }

            // restore mip chain visibility
            int maxDim = std::max((int)fullWidth, (int)fullHeight);
            int maxMips = 1 + (int)floor(log2(maxDim));

            glTextureParameteri(handleA, GL_TEXTURE_BASE_LEVEL, 0);
            glTextureParameteri(handleA, GL_TEXTURE_MAX_LEVEL, maxMips - 1);

            glTextureParameteri(handleB, GL_TEXTURE_BASE_LEVEL, 0);
            glTextureParameteri(handleB, GL_TEXTURE_MAX_LEVEL, maxMips - 1);

            glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

            // reset viewports for composite
            for (int i = 0; i < 4; i++) {
                Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
                if (!viewport->IsVisible()) continue;

                float vW = viewport->GetSize().x * fullWidth;
                float vH = viewport->GetSize().y * fullHeight;

                OpenGL::BindShader("EmissiveCompositeNew");
                OpenGL::SetUniformInt("u_viewportIndex", i);

                glBindImageTexture(0, gBuffer->GetColorAttachmentHandleByName(outputTextureName), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
                glBindTextureUnit(1, handleA);
                glBindTextureUnit(1, handleB);

                int dispatchX = static_cast<int>(std::ceil(vW / 16.0f));
                int dispatchY = static_cast<int>(std::ceil(vH / 4.0f));
                OpenGL::DispatchCompute(dispatchX, dispatchY, 1);
            }

            // reset mip attachments to 0 to avoid breaking future passes
            emissiveBlurFBO->SetColorAttachmentMipLevel("ColorA", 0);
            emissiveBlurFBO->SetColorAttachmentMipLevel("ColorB", 0);
        }
    }
}
