#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/Ocean/Ocean.h"
#include "Hell/Input.h"
namespace Input = Hell::Input;


namespace OpenGL::Renderer {

    void ComputeInverseFFT2D(GLuint handleA, GLuint handleB);
    float g_globalTime = 50.0f;

    void ComputeOceanFFTPass() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer* fftFrameBuffer_band0 = OpenGL::ResourceManager::GetFrameBufferPtr("FFT_band0");
        OpenGLFrameBuffer* fftFrameBuffer_band1 = OpenGL::ResourceManager::GetFrameBufferPtr("FFT_band1");
        OpenGLShader* oceanCalculateSpectrumShader = OpenGL::ResourceManager::GetShaderPtr("OceanCalculateSpectrum");
        OpenGLShader* oceanUpdateTexturesShader = OpenGL::ResourceManager::GetShaderPtr("OceanUpdateTextures");

        OpenGLSSBO* fftH0SSBO_band0 = OpenGL::ResourceManager::GetSSBOPtr("ffth0Band0");
        OpenGLSSBO* fftH0SSBO_band1 = OpenGL::ResourceManager::GetSSBOPtr("ffth0Band1");
        OpenGLSSBO* fftSpectrumInSSBO = OpenGL::ResourceManager::GetSSBOPtr("fftSpectrumInSSBO");
        OpenGLSSBO* fftSpectrumOutSSBO = OpenGL::ResourceManager::GetSSBOPtr("fftSpectrumOutSSBO");
        OpenGLSSBO* fftDispXInSSBO = OpenGL::ResourceManager::GetSSBOPtr("fftDispInXSSBO");
        OpenGLSSBO* fftDispZInSSBO = OpenGL::ResourceManager::GetSSBOPtr("fftDispZInSSBO");
        OpenGLSSBO* fftGradXInSSBO = OpenGL::ResourceManager::GetSSBOPtr("fftGradXInSSBO");
        OpenGLSSBO* fftGradZInSSBO = OpenGL::ResourceManager::GetSSBOPtr("fftGradZInSSBO");
        OpenGLSSBO* fftDispXOutSSBO = OpenGL::ResourceManager::GetSSBOPtr("fftDispXOutSSBO");
        OpenGLSSBO* fftDispZOutSSBO = OpenGL::ResourceManager::GetSSBOPtr("fftDispZOutSSBO");
        OpenGLSSBO* fftGradXOutSSBO = OpenGL::ResourceManager::GetSSBOPtr("fftGradXOutSSBO");
        OpenGLSSBO* fftGradZOutSSBO = OpenGL::ResourceManager::GetSSBOPtr("fftGradZOutSSBO");

        if (!fftFrameBuffer_band0) return;
        if (!fftFrameBuffer_band1) return;
        if (!fftH0SSBO_band0) return;
        if (!fftH0SSBO_band1) return;
        if (!fftSpectrumInSSBO) return;
        if (!fftSpectrumOutSSBO) return;
        if (!fftDispXInSSBO) return;
        if (!fftDispZInSSBO) return;
        if (!fftGradXInSSBO) return;
        if (!fftGradZInSSBO) return;
        if (!fftDispXOutSSBO) return;
        if (!fftDispZOutSSBO) return;
        if (!fftGradXOutSSBO) return;
        if (!fftGradZOutSSBO) return;
        if (!oceanUpdateTexturesShader) return;
        if (!oceanCalculateSpectrumShader) return;

        static double lastTime = glfwGetTime();
        double currentTime = glfwGetTime();
        float deltaTime = float(currentTime - lastTime);
        lastTime = currentTime;

        static bool doTime = true;

        if (doTime) {
            g_globalTime += deltaTime * 2.0f;
        }
        else {
            g_globalTime = 50.0f;
        }
        if (Input::KeyPressed(HELL_KEY_T)) {
            doTime = !doTime;
        }

        int bandCount = 2;

        const float gravity = Ocean::GetGravity();

        for (int i = 0; i < bandCount; i++) {

            if (GetFftDisplayMode() == 1 && i == 1) {
                continue;
            }
            if (GetFftDisplayMode() == 2 && i == 0) {
                continue;
            }

            const glm::uvec2 fftResolution = Ocean::GetFFTResolution(i);
            const glm::vec2 patchSimSize = Ocean::GetPatchSimSize(i);

            const GLuint blocksPerSide = 16;
            const GLuint blockSizeX = fftResolution.x / blocksPerSide;
            const GLuint blockSizeY = fftResolution.y / blocksPerSide;

            // Generate spectrum on GPU
            if (i == 0) {
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, fftH0SSBO_band0->GetHandle());
            }
            if (i == 1) {
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, fftH0SSBO_band1->GetHandle());
            }

            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, fftSpectrumInSSBO->GetHandle());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, fftDispXInSSBO->GetHandle());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, fftDispZInSSBO->GetHandle());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, fftGradXInSSBO->GetHandle());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, fftGradZInSSBO->GetHandle());

            OpenGL::BindShader("OceanCalculateSpectrum");
            OpenGL::SetUniformUVec2("u_fftGridSize", fftResolution);
            OpenGL::SetUniformVec2("u_patchSimSize", patchSimSize);
            OpenGL::SetUniformFloat("u_gravity", gravity);
            OpenGL::SetUniformFloat("u_time", g_globalTime);
            OpenGL::DispatchCompute(blockSizeX, blockSizeY, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

            // Perform FFT
            ComputeInverseFFT2D(fftSpectrumInSSBO->GetHandle(), fftSpectrumOutSSBO->GetHandle());
            ComputeInverseFFT2D(fftDispXInSSBO->GetHandle(), fftDispXOutSSBO->GetHandle());
            ComputeInverseFFT2D(fftDispZInSSBO->GetHandle(), fftDispZOutSSBO->GetHandle());
            ComputeInverseFFT2D(fftGradXInSSBO->GetHandle(), fftGradXOutSSBO->GetHandle());
            ComputeInverseFFT2D(fftGradZInSSBO->GetHandle(), fftGradZOutSSBO->GetHandle());

            // Update mesh position
            if (i == 0) {
                glBindImageTexture(0, fftFrameBuffer_band0->GetColorAttachmentHandleByName("Displacement"), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
                glBindImageTexture(1, fftFrameBuffer_band0->GetColorAttachmentHandleByName("Normals"), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
            }
            if (i == 1) {
                glBindImageTexture(0, fftFrameBuffer_band1->GetColorAttachmentHandleByName("Displacement"), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
                glBindImageTexture(1, fftFrameBuffer_band1->GetColorAttachmentHandleByName("Normals"), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
            }

            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, fftSpectrumInSSBO->GetHandle());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, fftDispXInSSBO->GetHandle());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, fftDispZInSSBO->GetHandle());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, fftGradXInSSBO->GetHandle());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, fftGradZInSSBO->GetHandle());

            OpenGL::BindShader("OceanUpdateTextures");
            OpenGL::SetUniformUVec2("u_fftGridSize", fftResolution);
            OpenGL::SetUniformFloat("u_dispScale", Ocean::GetDisplacementScale());
            OpenGL::SetUniformFloat("u_heightScale", Ocean::GetHeightScale());

            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            OpenGL::DispatchCompute(blockSizeX, blockSizeY, 1);
        }

        // Generate mips for the normals
        glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
        glGenerateTextureMipmap(fftFrameBuffer_band0->GetColorAttachmentHandleByName("Normals"));
        glGenerateTextureMipmap(fftFrameBuffer_band1->GetColorAttachmentHandleByName("Normals"));
    }

    void ComputeInverseFFT2D(GLuint handleA, GLuint handleB) {
        OpenGLShader* radix64Vert = OpenGL::ResourceManager::GetShaderPtr("FttRadix64Vertical");
        OpenGLShader* radix8Vert = OpenGL::ResourceManager::GetShaderPtr("FttRadix8Vertical");
        OpenGLShader* radix64Hori = OpenGL::ResourceManager::GetShaderPtr("FttRadix64Horizontal");
        OpenGLShader* radix8Hori = OpenGL::ResourceManager::GetShaderPtr("FttRadix8Horizontal");

        if (!radix64Vert) return;
        if (!radix8Vert) return;
        if (!radix64Hori) return;
        if (!radix8Hori) return;

        OpenGL::BindShader("FttRadix64Vertical");
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, handleA);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, handleB);
        OpenGL::DispatchCompute(32, 8, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        OpenGL::BindShader("FttRadix8Vertical");
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, handleB);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, handleA);
        OpenGL::DispatchCompute(32, 8, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        OpenGL::BindShader("FttRadix64Horizontal");
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, handleA);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, handleB);
        OpenGL::DispatchCompute(1, 512, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        OpenGL::BindShader("FttRadix8Horizontal");
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, handleB);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, handleA);
        OpenGL::DispatchCompute(1, 256, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
}