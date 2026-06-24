#include "../GL_renderer.h"
#include "Core/GameOLD.h"
#include "GlobalIllumination/GlobalIllumination.h"
#include "World/World.h"
#include "Renderer/Renderer.h"
#include "Ocean/Ocean.h"

#include "Config/Config.h"

#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"

namespace OpenGLRenderer {

    void ComputeViewspaceDepth() {
        OpenGLFrameBuffer* gBuffer = GetFrameBufferOLD("GBuffer");
        OpenGLFrameBuffer* fullSizeFBO = GetFrameBufferOLD("MiscFullSize");
        OpenGLShader* shader = GetShaderOLD("ViewspaceDepth");

        if (!gBuffer) return;
        if (!fullSizeFBO) return;
        if (!shader) return;

        shader->Bind();
        BindImageTexture(0, fullSizeFBO->GetColorAttachmentHandleByName("ViewspaceDepth"), GL_WRITE_ONLY, GL_R32F);
        BindTextureUnit(1, gBuffer->GetDepthAttachmentHandle());

        glDispatchCompute((gBuffer->GetWidth() + 7) / 8, (gBuffer->GetHeight() + 7) / 8, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    }

    void LightingPass() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer* gBuffer = GetFrameBufferOLD("GBuffer");
        OpenGLFrameBuffer* finalImageFBO = GetFrameBufferOLD("FinalImage");
        OpenGLShadowMap* flashLightShadowMapsFBO = GetShadowMapOLD("FlashlightShadowMaps");
        OpenGLFrameBuffer* indirectDiffuseFbo = GetFrameBufferOLD("IndirectDiffuse");
        OpenGLShadowCubeMapArray* hiResShadowMaps = GetShadowCubeMapArrayOLD("HiRes");
        OpenGLShader* shader = GetShaderOLD("Lighting");
        OpenGLFrameBuffer* miscFullSizeFBO = GetFrameBufferOLD("MiscFullSize");

        if (!gBuffer) return;
        if (!miscFullSizeFBO) return;
        if (!indirectDiffuseFbo) return;
        if (!finalImageFBO) return;
        if (!shader) return;

        IESProfile* iesProfile = Hell::ResourceManager::GetIESProfilePtr("Lamp0");
        if (!iesProfile) return;

        shader->Bind();

        shader->SetFloat("u_viewportWidth", gBuffer->GetWidth());
        shader->SetFloat("u_viewportHeight", gBuffer->GetHeight());
        shader->SetInt("u_tileXCount", gBuffer->GetWidth() / TILE_SIZE);
        shader->SetInt("u_tileYCount", gBuffer->GetHeight() / TILE_SIZE);
        shader->SetBool("u_sampleProbes", Renderer::GetCurrentRendererSettings().enableIrradianceProbeSampling);

        if (World::HasOcean()) {
            shader->SetFloat("u_oceanHeight", Ocean::GetOceanOriginY());
        }
        else {
            shader->SetFloat("u_oceanHeight", -1000);
        }

        // Warning this CSM shit is p1 only atm, especially cause of hardcoded FULL SCREEN viewport dimensions

        float viewportWidth = gBuffer->GetWidth();
        float viewportHeight = gBuffer->GetHeight();

        std::vector<float>& cascadeLevels = GetShadowCascadeLevels();
        shader->SetFloat("u_cascadeFarPlane", 256.0f); // ???
        shader->SetFloat("u_cascadePlaneDistances[0]", cascadeLevels[0]);
        shader->SetFloat("u_cascadePlaneDistances[1]", cascadeLevels[1]);
        shader->SetFloat("u_cascadePlaneDistances[2]", cascadeLevels[2]);
        shader->SetFloat("u_cascadePlaneDistances[3]", cascadeLevels[3]);

        shader->SetVec2("u_viewportSize", glm::vec2(viewportWidth, viewportHeight));

        glBindTextureUnit(0, gBuffer->GetColorAttachmentHandleByName("BaseColorMetallic"));
        glBindTextureUnit(1, gBuffer->GetColorAttachmentHandleByName("NormalXYRoughnessMisc"));
        glBindTextureUnit(2, gBuffer->GetColorAttachmentHandleByName("VelocityXYOcclusionSubSurface"));
        glBindTextureUnit(3, gBuffer->GetDepthAttachmentHandle());
        glBindTextureUnit(6, gBuffer->GetColorAttachmentHandleByName("Emissive"));
        glBindTextureUnit(7, GetTextureHandleByName("Flashlight2"));
        glBindTextureUnit(8, flashLightShadowMapsFBO->GetDepthTextureHandle());

        glActiveTexture(GL_TEXTURE9);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, hiResShadowMaps->GetDepthTexture());

        OpenGLShadowMapArray* shadowMapArray = GetShadowMapArrayOLD("MoonlightCSM");
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_2D_ARRAY, shadowMapArray->GetDepthTexture());

        glBindTextureUnit(11, indirectDiffuseFbo->GetColorAttachmentHandleByName("Color"));

        BindSSBO(7, "TileChristmasLights");
        BindSSBO(8, "ChristmasLightInstances");
        BindSSBO(9, "ChristmasLightIndices");

        glBindImageTexture(0, gBuffer->GetColorAttachmentHandleByName("Lighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);

        glDispatchCompute(gBuffer->GetWidth() / TILE_SIZE, gBuffer->GetHeight() / TILE_SIZE, 1);
    }
}
