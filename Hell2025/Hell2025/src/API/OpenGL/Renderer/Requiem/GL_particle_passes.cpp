#include "../GL_renderer.h"
#include "Core/Game.h"
#include "Input/Input.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderDataManager.h"
#include "Util/Util.h"
#include "Viewport/ViewportManager.h"
#include "World/World.h"

#include "Hell/GPUTypes.h"

namespace OpenGLRenderer {

    void DispatchTestPass();
    void BubblesPass();

    void UploadAnyNewParticles();
    void UpdateParticles();
    void DrawParticles();

    void ParticlePass() {

        UploadAnyNewParticles();
        UpdateParticles();
        DrawParticles();

        DispatchTestPass();
        BubblesPass();
    }

    void UploadAnyNewParticles() {
        std::vector<BulletTrailParticle>& particles = World::GetBulletTrailParticles();
        if (particles.empty()) return;

        std::vector<GpuParticle> gpuParticles;
        gpuParticles.reserve(particles.size());

        // Make a gpu copy of every cpu particle
        for (BulletTrailParticle& particle : particles) {
            GpuParticle& gpuParticle = gpuParticles.emplace_back();
            gpuParticle.position = glm::vec4(particle.position, 1.0f);
            gpuParticle.velocity = glm::vec4(particle.velocity, 1.0f);
            gpuParticle.rotation = particle.rotation;
            gpuParticle.rotationalVelocity = particle.rotationalVelocity;
            gpuParticle.lifeTime = particle.lifeTime;
        }

        // Cap at max particle size
        if (gpuParticles.size() > MAX_GPU_PARTICLES) {
            gpuParticles.resize(MAX_GPU_PARTICLES);
        }

        // Then clean the cpu particles, you don't need them any more, they're just for spawning
        particles.clear();

        // Now upload these to the gpu
        UploadSSBOStatic("ParticleAdditions", gpuParticles.size() * sizeof(GpuParticle), gpuParticles.data());

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        BindSSBO(5, "ParticleAdditions");
        BindSSBO(6, "ParticlePool");

        BindShader("ParticleAdditions");
        SetUniformInt("u_newParticleCount", gpuParticles.size());

        glDispatchCompute(1, 1, 1);
    }

    void UpdateParticles() {
        BindSSBO(6, "ParticlePool");
        BindSSBO(7, "ParticleActiveIndices");
        BindSSBO(8, "ParticleDrawCommand");

        BindShader("ParticleUpdate");
        SetUniformFloat("u_deltaTime", Game::GetDeltaTime());

        glDispatchCompute(1, 1, 1);
    }

    void DrawParticles() {
        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();

        OpenGLCubemapView& skyboxCubemapView = GetCubemapView("SkyboxNightSky");
        OpenGLFrameBuffer& fbo = GetFrameBuffer("GBufferRE");

        fbo.Bind();
        fbo.SetViewport();
        fbo.DrawBuffers({ "Lighting" });

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.colorMask = true;
        state.depthFunc = GL_GREATER;

        state.blendEnable = true;
        state.blendFuncSrcfactor = GL_SRC_ALPHA;
        state.blendFuncDstfactor = GL_ONE_MINUS_SRC_ALPHA;

        SetRasterizerState(state);

        BindShader("ParticleColor");
        SetUniformFloat("u_time", Game::GetTotalTime());

        BindTextureUnit(0, GetTextureHandleByName("UnderwaterBulletBubble"));

        glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

        BindSSBO(6, "ParticlePool");
        BindSSBO(7, "ParticleActiveIndices");

        BindEmptyVAO();
        BindDrawIndirectBuffer("ParticleDrawCommand");

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGLRenderer::SetViewport(&fbo, viewport);
            SetUniformInt("u_viewportIndex", i);
            SetUniformMat4("u_projectionView", viewportData[i].projectionViewReverseZ);
            SetUniformMat4("u_view", viewportData[i].view);
            SetUniformVec3("u_viewPos", viewportData[i].viewPos);

            //for (int j = 0; j < MAX_GPU_PARTICLES; j++) {
            //    SetUniformInt("u_particleIndex", j);
            //    glDrawArrays(GL_TRIANGLES, 0, 6);
            //
            //}

            glDrawArraysIndirect(GL_TRIANGLES, 0);
        }

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    }










    void DispatchTestPass() {
        static std::vector<glm::vec4> positions;

        if (Input::KeyPressed(HELL_KEY_U)) {
            glm::vec3 origin = glm::vec3(36.0f, 32.5f, 37.0f);
            positions.clear();

            for (int i = 0; i < 20; i++) {
                glm::vec3 pos = origin;
                pos.x -= Util::RandomFloat(-2.0f, 2.0f);
                pos.y -= Util::RandomFloat(-2.0f, 2.0f);
                pos.z -= Util::RandomFloat(-2.0f, 2.0f);
                positions.push_back(glm::vec4(pos, 1.0f));
            }
            UploadSSBOStatic("BubblePositions", positions.size() * sizeof(glm::vec4), positions.data());

            uint64_t count = positions.size();
            UpdateSSBO("BubblePositionCount", sizeof(uint64_t), &count);

            BindShader("BubbleDrawCommandArgs");
            BindSSBO(5, "BubblePositions");
            BindSSBO(6, "BubblePositionCount");
            BindSSBO(7, "BubbleDrawCommand");
            glDispatchCompute(1, 1, 1);
        }
    }


    void BubblesPass() {
        //ProfilerOpenGLZoneFunction();

        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();

        OpenGLCubemapView& skyboxCubemapView = GetCubemapView("SkyboxNightSky");
        OpenGLFrameBuffer& fbo = GetFrameBuffer("GBufferRE");

        fbo.Bind();
        fbo.SetViewport();
        fbo.DrawBuffers({ "Lighting" });

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.colorMask = true;
        state.depthFunc = GL_GREATER;

        state.blendEnable = true;
        state.blendFuncSrcfactor = GL_SRC_ALPHA;
        state.blendFuncDstfactor = GL_ONE_MINUS_SRC_ALPHA;

        SetRasterizerState(state);

        BindShader("Bubbles");
        SetUniformFloat("u_time", Game::GetTotalTime());
        BindTextureUnit(0, skyboxCubemapView.GetHandle());

        glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

        BindSSBO(5, "BubblePositions");

        BindEmptyVAO();
        BindDrawIndirectBuffer("BubbleDrawCommand");

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGLRenderer::SetViewport(&fbo, viewport);
            SetUniformInt("u_viewportIndex", i);
            SetUniformMat4("u_projectionView", viewportData[i].projectionViewReverseZ);
            SetUniformMat4("u_view", viewportData[i].view);
            SetUniformVec3("u_viewPos", viewportData[i].viewPos);

            glDrawArraysIndirect(GL_TRIANGLES, 0);
        }

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    }

}