#include "API/OpenGL//Renderer/GL_renderer.h"
#include "Editor/Editor.h"
#include "Core/GameOLD.h"
#include "Ocean/Ocean.h"
#include "World/LegacyWorld.h"

#include "Renderer/RenderDataManager.h"
#include "Viewport/ViewportManager.h"

#include "Renderer/Renderer.h"
#include "Hell/Input.h"
namespace Input = Hell::Input;


namespace OpenGLRenderer {

    static const int g_readbackBufferCount = 3;
    GLuint readbackSSBOs[g_readbackBufferCount];
    GLsync readbackSyncs[g_readbackBufferCount] = { 0 };

    void OceanGeometryPass() {
        ProfilerOpenGLZoneFunction();

        if (!LegacyWorld::HasOcean()) return;

        OpenGLFrameBuffer* gBuffer = GetFrameBufferOLD("GBuffer");
        OpenGLFrameBuffer* waterFrameBuffer = GetFrameBufferOLD("Water");
        OpenGLFrameBuffer* fftFrameBuffer_band0 = GetFrameBufferOLD("FFT_band0");
        OpenGLFrameBuffer* fftFrameBuffer_band1 = GetFrameBufferOLD("FFT_band1");
        OpenGLCubemapView* skyboxCubemapView = GetCubemapViewOLD("SkyboxNightSky");
        OpenGLShadowMap* flashLightShadowMapsFBO = GetShadowMapOLD("FlashlightShadowMaps");
        OpenGLMeshPatch* oceanMeshPatch = GetOceanMeshPatch();
        OpenGLShader* shader = GetShaderOLD("OceanGeometry");

        if (!gBuffer) return;
        if (!waterFrameBuffer) return;
        if (!fftFrameBuffer_band0) return;
        if (!fftFrameBuffer_band1) return;
        if (!skyboxCubemapView) return;
        if (!oceanMeshPatch) return;
        if (!shader) return;
        if (!flashLightShadowMapsFBO) return;

        static bool wireframe = false;
        static bool swap = false;
        static bool test = false;

        if (Input::KeyPressed(HELL_KEY_8)) {
            test = !test;
        }
        if (Input::KeyPressed(HELL_KEY_9)) {
            wireframe = !wireframe;
        }
        if (Input::KeyPressed(HELL_KEY_0)) {
            swap = !swap;
        }


        float scale = 0.05;

        int min = -10;
        int max = 10;
        float offset = (max - min) * Ocean::GetBaseFFTResolution().x * scale;

        if (test) {
            min = 0;
            max = 1;
            offset = Ocean::GetBaseFFTResolution().x * scale;
        }

        // Copy the gbuffer depth to prevent drawing over other geometry
        OpenGLRenderer::BlitFrameBufferDepth(gBuffer, waterFrameBuffer);

        waterFrameBuffer->Bind();
        waterFrameBuffer->DrawBuffers({ "Lighting", "OceanMask" });

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGLRenderer::SetViewport(waterFrameBuffer, viewport);

            const ViewportData& viewportData = RenderDataManager::GetViewportData()[i];
            glm::mat4 projectionMatrix = viewportData.projectionReverseZ;
            glm::mat4 viewMatrix = viewportData.view;
            glm::vec3 viewPos = viewportData.viewPos;
            glm::mat4 projectionView = viewportData.projectionViewReverseZ;

            float patchOffset = Ocean::GetBaseFFTResolution().y * scale;

            Transform tesseleationTransform;
            tesseleationTransform.scale = glm::vec3(scale);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, fftFrameBuffer_band0->GetColorAttachmentHandleByName("Displacement"));
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, fftFrameBuffer_band0->GetColorAttachmentHandleByName("Normals"));
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, fftFrameBuffer_band1->GetColorAttachmentHandleByName("Displacement"));
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, fftFrameBuffer_band1->GetColorAttachmentHandleByName("Normals"));
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxCubemapView->GetHandle());
            glBindTextureUnit(6, GetTextureHandleByName("Flashlight2"));
            glBindTextureUnit(7, flashLightShadowMapsFBO->GetDepthTextureHandle());
            glActiveTexture(GL_TEXTURE8);
            glBindTexture(GL_TEXTURE_2D, GetTextureHandleByName("WaterNormals"));

            BindTextureUnit(5, gBuffer->GetDepthAttachmentHandle());

            glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
            glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

            glEnable(GL_CULL_FACE);
            glEnable(GL_DEPTH_TEST);

            // Tessellated ocean
            tesseleationTransform.position.x = -patchOffset;
            shader->Bind();
            shader->SetInt("u_viewportIndex", i);
            shader->SetMat4("u_projectionView", projectionView);
            shader->SetVec3("u_wireframeColor", GREEN);
            shader->SetMat4("u_model", tesseleationTransform.to_mat4());
            shader->SetInt("u_mode", GetFftDisplayMode());
            shader->SetVec3("u_viewPos", viewPos);
            shader->SetVec2("u_fftGridSize", Ocean::GetBaseFFTResolution());
            shader->SetBool("u_wireframe", wireframe);
            shader->SetFloat("u_meshSubdivisionFactor", Ocean::GetMeshSubdivisionFactor());
            shader->SetFloat("u_oceanOriginY", Ocean::GetOceanOriginY());
            shader->SetFloat("u_time", GameOLD::GetTotalTime());

            glGenerateTextureMipmap(fftFrameBuffer_band0->GetColorAttachmentHandleByName("Normals"));
            glGenerateTextureMipmap(fftFrameBuffer_band1->GetColorAttachmentHandleByName("Normals"));

            glBindVertexArray(oceanMeshPatch->GetVAO());
            glPatchParameteri(GL_PATCH_VERTICES, 4);

            Frustum frustum;
            frustum.Update(projectionView);

            // Surface
            glDisable(GL_CULL_FACE);
            for (int x = min; x < max; x++) {
                for (int z = min; z < max; z++) {
                    tesseleationTransform.position = glm::vec3(patchOffset * x, Ocean::GetOceanOriginY(), patchOffset * z);
                    if (swap) {
                        tesseleationTransform.position += glm::vec3(offset, 0.0f, 0.0f);
                    }

                    float threshold = 1.0f;
                    glm::vec3 aabbMin = tesseleationTransform.position - glm::vec3(0, threshold / 2, 0);
                    glm::vec3 aabbMax = tesseleationTransform.position + glm::vec3(patchOffset, threshold / 2, patchOffset);
                    AABB aabb(aabbMin, aabbMax);
                    //DrawAABB(aabb, BLUE);

                    if (frustum.IntersectsAABB(aabb)) {
                        shader->SetMat4("u_model", tesseleationTransform.to_mat4());
                        glDrawElements(GL_PATCHES, oceanMeshPatch->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
                    }
                }
            }
        }

        // Cleanup
        shader->SetBool("u_wireframe", false);
        glEnable(GL_DEPTH_TEST);
        glCullFace(GL_BACK);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    void OceanUnderWaterFlags() {
        ProfilerOpenGLZoneFunction();
        if (!LegacyWorld::HasOcean()) return;

        OpenGLFrameBuffer& fftFrameBuffer_band0 = GetFrameBuffer("FFT_band0");
        OpenGLFrameBuffer& fftFrameBuffer_band1 = GetFrameBuffer("FFT_band1");
        OpenGLFrameBuffer& waterFrameBuffer = GetFrameBuffer("Water");

        BindShader("OceanFlags");

        SetUniformInt("u_mode", GetFftDisplayMode());
        SetUniformFloat("u_oceanOriginY", Ocean::GetOceanOriginY());

        BindImageTexture(0, waterFrameBuffer.GetColorAttachmentHandleByName("OceanFlags"), GL_WRITE_ONLY, GL_R8UI);
        BindImageTexture(1, waterFrameBuffer.GetColorAttachmentHandleByName("OceanMask"), GL_READ_ONLY, GL_R8UI);
        BindTextureUnit(2, fftFrameBuffer_band0.GetColorAttachmentHandleByName("Displacement"));
        BindTextureUnit(3, fftFrameBuffer_band1.GetColorAttachmentHandleByName("Displacement"));

        glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        glDispatchCompute((waterFrameBuffer.GetWidth() + 7) / 8, (waterFrameBuffer.GetHeight() + 7) / 8, 1);
    }

    void OceanSurfaceCompositePass() {
        ProfilerOpenGLZoneFunction();
        if (!LegacyWorld::HasOcean()) return;

        std::string gBufferName = (Renderer::GetRendererMode() == RendererMode::RE_STYLE) ? "GBufferRE" : "GBuffer";
        OpenGLFrameBuffer& gBuffer = GetFrameBuffer(gBufferName);

        OpenGLFrameBuffer& waterFrameBuffer = GetFrameBuffer("Water");
        OpenGLFrameBuffer& quaterSizeFrameBuffer = GetFrameBuffer("QuarterSize");

        // Down sample the final lighting to 25%
        // TODO: try using Gaussian blur of final lighting. It's currently calculated in the underwater composite pass so will have to move it before
        BlitFrameBuffer(&gBuffer, &quaterSizeFrameBuffer, "Lighting", "DownsampledFinalLighting", GL_COLOR_BUFFER_BIT, GL_LINEAR);

        BindShader("OceanSurfaceComposite");

        SetUniformFloat("u_time", GameOLD::GetTotalTime());
        SetUniformFloat("u_oceanYOrigin", Ocean::GetOceanOriginY());
        SetUniformVec2("u_resolution", glm::vec2(gBuffer.GetWidth(), gBuffer.GetHeight()));

        BindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("Lighting"), GL_READ_WRITE, GL_RGBA16F);
        BindImageTexture(1, waterFrameBuffer.GetColorAttachmentHandleByName("OceanMask"), GL_READ_ONLY, GL_R8UI);
        BindTextureUnit(2, waterFrameBuffer.GetColorAttachmentHandleByName("Lighting"));
        BindTextureUnit(3, GetTextureHandleByName("WaterDUDV"));
        BindTextureUnit(4, quaterSizeFrameBuffer.GetColorAttachmentHandleByName("DownsampledFinalLighting"));

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        glDispatchCompute((gBuffer.GetWidth() + 7) / 8, (gBuffer.GetHeight() + 7) / 8, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT);
    }

    void GaussianBlur() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& miscFullSizeFrameBuffer = GetFrameBuffer("MiscFullSize");

        std::string gBufferName = (Renderer::GetRendererMode() == RendererMode::RE_STYLE) ? "GBufferRE" : "GBuffer";
        OpenGLFrameBuffer& gBuffer = GetFrameBuffer(gBufferName);

        BindShader("GaussianBlur");

        SetUniformVec2("u_direction", glm::vec2(0, 1));
        glBindImageTexture(0, miscFullSizeFrameBuffer.GetColorAttachmentHandleByName("GaussianFinalLightingIntermediate"), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F); // WARNING! you WERE degrading your image quality by down sampling into a texture of lower bit resolution. Find out if this even matters at this point in the frame. But now you're not. But also. This is a shit load of VRAM so think about this.
        glBindTextureUnit(1, gBuffer.GetColorAttachmentHandleByName("Lighting"));
        glDispatchCompute((miscFullSizeFrameBuffer.GetWidth() + 7) / 8, (miscFullSizeFrameBuffer.GetHeight() + 7) / 8, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        SetUniformVec2("u_direction", glm::vec2(1, 0));
        glBindImageTexture(0, miscFullSizeFrameBuffer.GetColorAttachmentHandleByName("GaussianFinalLighting"), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        glBindTextureUnit(1, miscFullSizeFrameBuffer.GetColorAttachmentHandleByName("GaussianFinalLightingIntermediate"));
        glDispatchCompute((miscFullSizeFrameBuffer.GetWidth() + 7) / 8, (miscFullSizeFrameBuffer.GetHeight() + 7) / 8, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    void OceanUnderwaterCompositePass() {
        ProfilerOpenGLZoneFunction();
        if (!LegacyWorld::HasOcean()) return;

        OpenGLFrameBuffer& miscFullSizeFrameBuffer = GetFrameBuffer("MiscFullSize");
        OpenGLFrameBuffer& waterFrameBuffer = GetFrameBuffer("Water");

        std::string gBufferName = (Renderer::GetRendererMode() == RendererMode::RE_STYLE) ? "GBufferRE" : "GBuffer";
        OpenGLFrameBuffer& gBuffer = GetFrameBuffer(gBufferName);

        BindShader("OceanUnderwaterComposite");
        
        SetUniformFloat("u_time", GameOLD::GetTotalTime());
        SetUniformVec2("u_resolution", glm::vec2(gBuffer.GetWidth(), gBuffer.GetHeight()));

        BindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("Lighting"), GL_READ_WRITE, GL_RGBA16F);
        BindImageTexture(1, waterFrameBuffer.GetColorAttachmentHandleByName("OceanFlags"), GL_READ_ONLY, GL_R8UI);
        BindTextureUnit(2, miscFullSizeFrameBuffer.GetColorAttachmentHandleByName("GaussianFinalLighting"));
        BindTextureUnit(3, waterFrameBuffer.GetColorAttachmentHandleByName("Lighting"));
        BindTextureUnit(4, GetTextureHandleByName("WaterDUDV"));

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        glDispatchCompute((gBuffer.GetWidth() + 7) / 8, (gBuffer.GetHeight() + 7) / 8, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT);
    }

    void InitOceanHeightReadback() {
        const GLbitfield storageFlags = GL_MAP_READ_BIT | GL_CLIENT_STORAGE_BIT;
        //const GLbitfield storageFlags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
        glGenBuffers(g_readbackBufferCount, readbackSSBOs);
        for (int i = 0; i < g_readbackBufferCount; ++i) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, readbackSSBOs[i]);
            glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(OceanReadbackData), nullptr, storageFlags);
        }
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void OceanHeightReadback() {
        if (!LegacyWorld::HasOcean()) return;

        OpenGLFrameBuffer* fftFrameBuffer_band0 = GetFrameBufferOLD("FFT_band0");
        OpenGLFrameBuffer* fftFrameBuffer_band1 = GetFrameBufferOLD("FFT_band1");
        OpenGLShader* shader = GetShaderOLD("OceanPositionReadback");

        if (!fftFrameBuffer_band0) return;
        if (!fftFrameBuffer_band1) return;
        if (!shader) return;

        static int frame = 0;
        int idx = frame % g_readbackBufferCount;

        if (readbackSyncs[idx]) {
            GLenum status = glClientWaitSync(readbackSyncs[idx], 0, 0);
            if (status == GL_ALREADY_SIGNALED || status == GL_CONDITION_SATISFIED) {
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, readbackSSBOs[idx]);
                void* ptr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(OceanReadbackData), GL_MAP_READ_BIT);

                if (ptr) {
                    const OceanReadbackData* gpuData = static_cast<const OceanReadbackData*>(ptr);
                    OceanReadbackData& cpuData = Ocean::GetOceanReadBackData();
                    cpuData.heightPlayer0 = gpuData->heightPlayer0;
                    cpuData.heightPlayer1 = gpuData->heightPlayer1;
                    cpuData.heightPlayer2 = gpuData->heightPlayer2;
                    cpuData.heightPlayer3 = gpuData->heightPlayer3;
                    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
                }
                else {
                    GLenum err = glGetError();
                    std::cerr << "ERROR: glMapBufferRange returned null for slot " << idx << ", glGetError()=0x" << std::hex << err << std::dec << "\n";
                    GLint flags = 0;
                    glGetNamedBufferParameteriv(readbackSSBOs[idx], GL_BUFFER_ACCESS_FLAGS, &flags);
                    std::cerr << "      Buffer access flags: 0x" << std::hex << flags << std::dec << "\n";
                    GLint buffer_size = 0;
                    glGetNamedBufferParameteriv(readbackSSBOs[idx], GL_BUFFER_SIZE, &buffer_size);
                    std::cerr << "      Buffer size: " << buffer_size << "\n";
                }
                glDeleteSync(readbackSyncs[idx]);
                readbackSyncs[idx] = 0;
            }
            else if (status == GL_TIMEOUT_EXPIRED) {
                // Not an error for timeout 0, just means not ready
            }
            else if (status == GL_WAIT_FAILED) {
                GLenum err = glGetError();
                std::cerr << "ERROR: glClientWaitSync failed for slot " << idx << ", glGetError()=0x" << std::hex << err << std::dec << "\n";
                glDeleteSync(readbackSyncs[idx]);
                readbackSyncs[idx] = 0;
            }
        }
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, readbackSSBOs[idx]);
        shader->Bind();
        shader->SetFloat("u_oceanOriginY", Ocean::GetOceanOriginY());
        shader->SetInt("u_mode", GetFftDisplayMode());

        for (int i = 0; i < 4; ++i) {
            glm::vec3 position = glm::vec3(0.0f);
            if (GameOLD::GetLocalPlayerCount() > i) {
                position = GameOLD::GetLocalPlayerByIndex(i)->GetFootPosition();
            }
            shader->SetVec3("positionPlayer" + std::to_string(i), position);
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fftFrameBuffer_band0->GetColorAttachmentHandleByName("Displacement"));
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, fftFrameBuffer_band1->GetColorAttachmentHandleByName("Displacement"));
        glDispatchCompute(1, 1, 1);
        readbackSyncs[idx] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        if (!readbackSyncs[idx]) {
            GLenum err = glGetError();
            std::cerr << "ERROR: glFenceSync failed for slot " << idx << ", glGetError()=0x" << std::hex << err << std::dec << "\n";
        }
        ++frame;
    }
}