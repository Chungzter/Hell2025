#include "Hell/Logging.h"
#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Unloved/Editor/Editor.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/Session/Session.h"
#include "Hell/Time.h"

namespace OpenGLRenderer {

    void InitFog() {
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("PerlinNoise3D");
        OpenGLTexture3D* perlinNoiseTexture = OpenGL::ResourceManager::GetTexture3DPtr("PerlinNoise");

        if (!shader) return;
        if (!perlinNoiseTexture) return;

        glBindImageTexture(0, perlinNoiseTexture->GetHandle(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32F);

        int size = perlinNoiseTexture->GetSize();

        OpenGL::BindShader("PerlinNoise3D");
        OpenGL::SetUniformFloat("uScale", 8.0f);
        OpenGL::SetUniformVec3("uDimensions", glm::vec3(size));

        OpenGL::DispatchCompute((size + 7) / 8, (size + 7) / 8, (size + 7) / 8);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        perlinNoiseTexture->GenerateMipmaps();

        Logging::Init() << "Initialized the BERLIN NOISE";
    }

    void RayMarchFog() {
        ProfilerOpenGLZoneFunction();

        if (Unloved::Editor::IsOpen()) return;

        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();

        OpenGLFrameBuffer* fogFbo = OpenGL::ResourceManager::GetFrameBufferPtr("Fog");
        OpenGLTexture3D* perlinNoiseTexture = OpenGL::ResourceManager::GetTexture3DPtr("PerlinNoise");

        std::string gBufferName = (Unloved::Renderer::GetRendererMode() == RendererMode::RE_STYLE) ? "GBufferRE" : "GBuffer";
        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer(gBufferName);

        if (!fogFbo) return;
        if (!perlinNoiseTexture) return;

        static float time = 0.0f;
        time += Hell::Time::DeltaTime();

        static int noiseSeed = 0;
        noiseSeed++;

        // Ray march the fog
        OpenGL::BindShader("FogRayMarch");
        OpenGL::SetUniformFloat("u_time", time);
        OpenGL::SetUniformInt("u_noiseSeed", noiseSeed);
        OpenGL::BindImageTexture(4, fogFbo->GetColorAttachmentHandleByName("Color"), GL_WRITE_ONLY, GL_RGBA16F);
        OpenGL::BindTextureUnit(1, gBuffer.GetDepthAttachmentHandle());
        OpenGL::BindTextureUnit(2, perlinNoiseTexture->GetHandle());

        OpenGL::DispatchCompute((fogFbo->GetWidth() + 15) / 16, (fogFbo->GetHeight() + 15) / 16, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        // Composite
        OpenGL::BindShader("FogComposite");
        OpenGL::BindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("Lighting"), GL_READ_WRITE, GL_RGBA16F);
        OpenGL::BindTextureUnit(1, fogFbo->GetColorAttachmentHandleByName("Color"));

        OpenGL::DispatchCompute((gBuffer.GetWidth() + 7) / 8, (gBuffer.GetHeight() + 7) / 8, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }


    void BlitFog() {
        OpenGLTexture3D* perlinNoiseTexture = OpenGL::ResourceManager::GetTexture3DPtr("PerlinNoise");
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