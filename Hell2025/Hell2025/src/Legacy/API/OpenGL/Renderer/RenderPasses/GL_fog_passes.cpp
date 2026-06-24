#include "Hell/Logging.h"
#include "API/OpenGL/Renderer/GL_renderer.h"
#include "API/OpenGL/GL_backend.h"
#include "Editor/Editor.h"
#include "Renderer/RenderDataManager.h"
#include "Renderer/Renderer.h"
#include "Viewport/ViewportManager.h"
#include "Core/GameOLD.h"

namespace OpenGLRenderer {

    void InitFog() {
        OpenGLShader* shader = GetShaderOLD("PerlinNoise3D");
        OpenGLTexture3D* perlinNoiseTexture = GetTexture3D("PerlinNoise");

        if (!shader) return;
        if (!perlinNoiseTexture) return;

        glBindImageTexture(0, perlinNoiseTexture->GetHandle(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32F);

        int size = perlinNoiseTexture->GetSize();

        shader->Bind();
        shader->SetFloat("uScale", 8.0f);
        shader->SetVec3("uDimensions", glm::vec3(size));
        
        glDispatchCompute((size + 7) / 8, (size + 7) / 8, (size + 7) / 8);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        perlinNoiseTexture->GenerateMipmaps();

        Logging::Init() << "Initialized the BERLIN NOISE";
    }

    void RayMarchFog() {
        ProfilerOpenGLZoneFunction();

        if (Editor::IsOpen()) return;

        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();

        OpenGLFrameBuffer* fogFbo = GetFrameBufferOLD("Fog");
        OpenGLTexture3D* perlinNoiseTexture = GetTexture3D("PerlinNoise");

        std::string gBufferName = (Renderer::GetRendererMode() == RendererMode::RE_STYLE) ? "GBufferRE" : "GBuffer";
        OpenGLFrameBuffer& gBuffer = GetFrameBuffer(gBufferName);

        if (!fogFbo) return;
        if (!perlinNoiseTexture) return;

        static float time = 0.0f;
        time += GameOLD::GetDeltaTime();

        static int noiseSeed = 0;
        noiseSeed++;

        // Ray march the fog
        BindShader("FogRayMarch");
        SetUniformFloat("u_time", time);
        SetUniformInt("u_noiseSeed", noiseSeed);
        BindImageTexture(4, fogFbo->GetColorAttachmentHandleByName("Color"), GL_WRITE_ONLY, GL_RGBA16F);
        BindTextureUnit(1, gBuffer.GetDepthAttachmentHandle());
        BindTextureUnit(2, perlinNoiseTexture->GetHandle());

        glDispatchCompute((fogFbo->GetWidth() + 15) / 16, (fogFbo->GetHeight() + 15) / 16, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        // Composite
        BindShader("FogComposite");
        BindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("Lighting"), GL_READ_WRITE, GL_RGBA16F);
        BindTextureUnit(1, fogFbo->GetColorAttachmentHandleByName("Color"));

        glDispatchCompute((gBuffer.GetWidth() + 7) / 8, (gBuffer.GetHeight() + 7) / 8, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }


    void BlitFog() {
        OpenGLTexture3D* perlinNoiseTexture = GetTexture3D("PerlinNoise");
        if (!perlinNoiseTexture) return;

        GLuint fbo;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);

        static int z = 0;
        z = (z + 1) % 128;
        glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, perlinNoiseTexture->GetHandle(), 0, z);

        GLenum status = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "FBO incomplete: " << status << "\n";
        }

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        int sliceSize = 128;

        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glDrawBuffer(GL_BACK);

        glBlitFramebuffer(
            0, 0, sliceSize, sliceSize,
            0, 0, sliceSize * 4, sliceSize * 4,
            GL_COLOR_BUFFER_BIT,
            GL_NEAREST
        );

        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &fbo);
    }


}