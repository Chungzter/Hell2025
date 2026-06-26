#include "../GL_renderer.h"
#include "Core/GameOLD.h"
#include "GlobalIllumination/GlobalIllumination.h"
#include "World/LegacyWorld.h"
#include "Renderer/Renderer.h"
#include "Ocean/Ocean.h"

#include "Config/Config.h"

#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"

namespace OpenGLRenderer {

    void ComputeViewspaceDepth() {
        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLFrameBuffer* fullSizeFBO = OpenGL::ResourceManager::GetFrameBufferPtr("MiscFullSize");
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("ViewspaceDepth");

        if (!gBuffer) return;
        if (!fullSizeFBO) return;
        if (!shader) return;

        OpenGL::BindShader("ViewspaceDepth");
        OpenGL::BindImageTexture(0, fullSizeFBO->GetColorAttachmentHandleByName("ViewspaceDepth"), GL_WRITE_ONLY, GL_R32F);
        OpenGL::BindTextureUnit(1, gBuffer->GetDepthAttachmentHandle());

        OpenGL::DispatchCompute((gBuffer->GetWidth() + 7) / 8, (gBuffer->GetHeight() + 7) / 8, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    }

    void LightingPass() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLFrameBuffer* finalImageFBO = OpenGL::ResourceManager::GetFrameBufferPtr("FinalImage");
        OpenGLShadowMap* flashLightShadowMapsFBO = OpenGL::ResourceManager::GetShadowMapPtr("FlashlightShadowMaps");
        OpenGLFrameBuffer* indirectDiffuseFbo = OpenGL::ResourceManager::GetFrameBufferPtr("IndirectDiffuse");
        OpenGLShadowCubeMapArray* hiResShadowMaps = OpenGL::ResourceManager::GetShadowCubeMapArrayPtr("HiRes");
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("Lighting");
        OpenGLFrameBuffer* miscFullSizeFBO = OpenGL::ResourceManager::GetFrameBufferPtr("MiscFullSize");

        if (!gBuffer) return;
        if (!miscFullSizeFBO) return;
        if (!indirectDiffuseFbo) return;
        if (!finalImageFBO) return;
        if (!shader) return;

        IESProfile* iesProfile = Hell::ResourceManager::GetIESProfilePtr("Lamp0");
        if (!iesProfile) return;

        OpenGL::BindShader("Lighting");

        OpenGL::SetUniformFloat("u_viewportWidth", gBuffer->GetWidth());
        OpenGL::SetUniformFloat("u_viewportHeight", gBuffer->GetHeight());
        OpenGL::SetUniformInt("u_tileXCount", gBuffer->GetWidth() / TILE_SIZE);
        OpenGL::SetUniformInt("u_tileYCount", gBuffer->GetHeight() / TILE_SIZE);
        OpenGL::SetUniformBool("u_sampleProbes", Renderer::GetCurrentRendererSettings().enableIrradianceProbeSampling);

        if (LegacyWorld::HasOcean()) {
            OpenGL::SetUniformFloat("u_oceanHeight", Ocean::GetOceanOriginY());
        }
        else {
            OpenGL::SetUniformFloat("u_oceanHeight", -1000);
        }

        // Warning this CSM shit is p1 only atm, especially cause of hardcoded FULL SCREEN viewport dimensions

        float viewportWidth = gBuffer->GetWidth();
        float viewportHeight = gBuffer->GetHeight();

        std::vector<float>& cascadeLevels = GetShadowCascadeLevels();
        OpenGL::SetUniformFloat("u_cascadeFarPlane", 256.0f); // ???
        OpenGL::SetUniformFloat("u_cascadePlaneDistances[0]", cascadeLevels[0]);
        OpenGL::SetUniformFloat("u_cascadePlaneDistances[1]", cascadeLevels[1]);
        OpenGL::SetUniformFloat("u_cascadePlaneDistances[2]", cascadeLevels[2]);
        OpenGL::SetUniformFloat("u_cascadePlaneDistances[3]", cascadeLevels[3]);

        OpenGL::SetUniformVec2("u_viewportSize", glm::vec2(viewportWidth, viewportHeight));

        glBindTextureUnit(0, gBuffer->GetColorAttachmentHandleByName("BaseColorMetallic"));
        glBindTextureUnit(1, gBuffer->GetColorAttachmentHandleByName("NormalXYRoughnessMisc"));
        glBindTextureUnit(2, gBuffer->GetColorAttachmentHandleByName("VelocityXYOcclusionSubSurface"));
        glBindTextureUnit(3, gBuffer->GetDepthAttachmentHandle());
        glBindTextureUnit(6, gBuffer->GetColorAttachmentHandleByName("Emissive"));
        glBindTextureUnit(7, GetTextureHandleByName("Flashlight2"));
        glBindTextureUnit(8, flashLightShadowMapsFBO->GetDepthTextureHandle());

        glActiveTexture(GL_TEXTURE9);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, hiResShadowMaps->GetDepthTexture());

        OpenGLShadowMapArray* shadowMapArray = OpenGL::ResourceManager::GetShadowMapArrayPtr("MoonlightCSM");
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_2D_ARRAY, shadowMapArray->GetDepthTexture());

        glBindTextureUnit(11, indirectDiffuseFbo->GetColorAttachmentHandleByName("Color"));

        OpenGL::BindSSBO(7, "TileChristmasLights");
        OpenGL::BindSSBO(8, "ChristmasLightInstances");
        OpenGL::BindSSBO(9, "ChristmasLightIndices");

        glBindImageTexture(0, gBuffer->GetColorAttachmentHandleByName("Lighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);

        OpenGL::DispatchCompute(gBuffer->GetWidth() / TILE_SIZE, gBuffer->GetHeight() / TILE_SIZE, 1);
    }
}
