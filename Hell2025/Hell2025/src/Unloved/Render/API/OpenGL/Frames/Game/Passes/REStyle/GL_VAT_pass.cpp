#include "Hell/Debug/DebugDraw.h"
#include "Hell/Common/Color.h"
#include "Hell/Input/Input.h"
#include "Hell/Input/keycodes.h"
#include "Hell/Logging.h"
#include "Hell/Math/LocalFrame.h"
#include "Hell/Math/Transform.h"
#include "Hell/Render/API/OpenGL/GL_rasterizer_state_manager.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Time.h"

#include "Unloved/Debug/Debug.h"
#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Objects/Renderables/PointAnimationInstance.h"
#include "Unloved/Objects/Renderables/VATInstance.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Viewport/ViewportManager.h"

#include "res/shaders/common/OpenGL/GL_binding_indices.glsl"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace OpenGL::Renderer {

    struct TestParticle {

        TestParticle() = default;

        TestParticle(const glm::vec3& position, const glm::vec3& velocity) {
            m_position = position;
            m_velocity = velocity;
        }

        void Update(float deltaTime) {
            m_velocity.y += m_gravity * deltaTime;
            m_position += m_velocity * deltaTime;
        }

        glm::vec3 m_position = glm::vec3(0.0f);
        glm::vec3 m_velocity = glm::vec3(0.0f);
        float m_gravity = -9.8f;
    };

    void VATPass() {
        ProfilerOpenGLZoneFunction();

        static VATInstance vatInstance;
        static std::vector<TestParticle> particles;
        static bool mirror = false;
        static bool superDebugMode = false;

        glm::vec3 position = glm::vec3(36.25f, 32.60f, 37.56f);
        Hell::DebugDraw::DrawPoint(position, YELLOW);
    
        // Model matrix
        Hell::Transform transform;
        transform.position = position;
        transform.scale = glm::vec3(0.1f);
        glm::mat4 modelMatrix = transform.ToMat4();

        // Toggle super debug mode
        if (Hell::Input::KeyPressed(HELL_KEY_BACKSPACE)) {
            superDebugMode = !superDebugMode;
        }

        // Super debug mode
        if (superDebugMode) {
            Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(0);
            if (!player) return;

            glm::vec3 pos = player->GetInteractHitPosition();
            glm::vec3 normal = player->GetInteractHitNormal();

            normal.y = 0.0f;
            normal = glm::normalize(normal);

            Hell::LocalFrame localFrame = Hell::LocalFrame(normal);
            Hell::QuatTransform transform = Hell::QuatTransform(pos, localFrame, glm::vec3(0.05f));
            modelMatrix = transform.ToMat4();

            Hell::DebugDraw::DrawPoint(pos, YELLOW);
            Hell::DebugDraw::DrawLocalFrame(pos, localFrame, 0.1f);

            // Spawn particle
            if (Hell::Input::KeyDown(HELL_KEY_UP)) {
                glm::vec3 vel =
                    localFrame.right * Hell::Random::Float(-1.0f, 1.0f) +
                    localFrame.up * Hell::Random::Float(0.1f, 1.0f) +
                    localFrame.forward * Hell::Random::Float(0.0f, 0.5f);

                particles.push_back(TestParticle(pos, vel));
            }
        }

        if (Hell::Input::KeyPressed(HELL_KEY_RIGHT)) {
            constexpr float testPlaybackSpeed = 5.0f;

            VATInstanceCreateInfo createInfo;
            createInfo.resourceName = "Blood19";
            createInfo.playbackSpeed = testPlaybackSpeed;
            createInfo.loop = false;
            vatInstance.Init(createInfo);

            particles.clear();
        }

        if (Hell::Input::KeyPressed(HELL_KEY_LEFT)) {
            constexpr float testPlaybackSpeed = 5.0f;

            VATInstanceCreateInfo createInfo;
            createInfo.resourceName = "Blood20";
            createInfo.playbackSpeed = testPlaybackSpeed;
            createInfo.loop = false;
            vatInstance.Init(createInfo);

            particles.clear();
        }

        if (Hell::Input::KeyPressed(HELL_KEY_DOWN)) {
            constexpr float testPlaybackSpeed = 5.0f;

            VATInstanceCreateInfo createInfo;
            createInfo.resourceName = "Blood22";
            createInfo.playbackSpeed = testPlaybackSpeed;
            createInfo.loop = false;
            vatInstance.Init(createInfo);

            particles.clear();
        }

        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        if (!gBuffer) return;

        gBuffer->Bind();
        gBuffer->DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "VelocityXYOcclusionSubSurface" });

        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");

        Hell::Vat* vat = Hell::ResourceManager::GetVATPtr(vatInstance.GetResourceName());
        if (!vat) return;

        const Hell::VATMetadata& metadata = vat->GetMetadata();

        Model* model = Hell::ResourceManager::GetModelById(vat->GetModelId());
        if (!model) return;
        if (model->GetMeshIndices().empty()) return;

        Mesh* mesh = meshBuffer.GetMeshById(model->GetMeshIndices()[0]);
        if (!mesh) return;

        BindShader("VAT");

        if (!vatInstance.HasValidTextureIndices()) return;
        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        SetUniformInt("u_positionTextureIndex", vatInstance.GetPositionTextureIndex());
        SetUniformInt("u_rotationTextureIndex", vatInstance.GetRotationTextureIndex());
        SetUniformInt("u_lookupTextureIndex", vatInstance.GetLookupTextureIndex());

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = true;
        state.depthFunc = GL_GREATER;

        OpenGL::RasterizerStateManager::SetRasterizerState(state);

        const glm::mat4 inverseModelMatrix = glm::inverse(modelMatrix);
        
        const float deltaTime = Hell::Time::DeltaTime();
        vatInstance.Update(deltaTime, modelMatrix);

        glBindVertexArray(OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(gBuffer, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);

            SetUniformMat4("u_modelMatrix", modelMatrix);
            SetUniformMat4("u_inverseModelMatrix", inverseModelMatrix);
            SetUniformFloat("u_time", vatInstance.GetCurrentTime());
            SetUniformFloat("u_fps", vatInstance.GetFPS());
            SetUniformInt("u_frameCount", vatInstance.GetFrameCount());
            SetUniformVec3("u_boundsMin", metadata.boundsMin);
            SetUniformVec3("u_boundsMax", metadata.boundsMax);
            SetUniformBool("u_mirror", mirror);

            glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
        }

        for (int i = 0; i < particles.size(); i++) {
            particles[i].Update(Hell::Time::DeltaTime());

            const glm::vec3& point = particles[i].m_position;
            Hell::DebugDraw::DrawPoint(point, glm::vec4(Hell::Color::Random(i), 1.0f));
        }
    }
}
