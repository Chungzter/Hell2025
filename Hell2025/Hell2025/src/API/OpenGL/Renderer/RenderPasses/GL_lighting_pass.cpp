#include "../GL_renderer.h"
#include "AssetManagement/AssetManager.h"
#include "Core/Game.h"
#include "GlobalIllumination/GlobalIllumination.h"
#include "World/World.h"
#include "Renderer/Renderer.h"
#include "Ocean/Ocean.h"

#include "Config/Config.h"

#include <Hell/Logging.h>

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
        BindTextureUnit(1, gBuffer->GetColorAttachmentHandleByName("WorldPosition"));
        BindTextureUnit(2, fullSizeFBO->GetColorAttachmentHandleByName("ViewportIndex"));

        glDispatchCompute((gBuffer->GetWidth() + 7) / 8, (gBuffer->GetHeight() + 7) / 8, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    }

    void LightingPass() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer* gBuffer = GetFrameBufferOLD("GBuffer");
        OpenGLFrameBuffer* finalImageFBO = GetFrameBufferOLD("FinalImage");
        OpenGLShadowMap* flashLightShadowMapsFBO = GetShadowMap("FlashlightShadowMaps");
        OpenGLFrameBuffer* indirectDiffuseFbo = GetFrameBufferOLD("IndirectDiffuse");
        OpenGLShadowCubeMapArray* hiResShadowMaps = GetShadowCubeMapArray("HiRes");
        OpenGLShader* shader = GetShaderOLD("Lighting");
        OpenGLFrameBuffer* miscFullSizeFBO = GetFrameBufferOLD("MiscFullSize");

        if (!gBuffer) return;
        if (!miscFullSizeFBO) return;
        if (!indirectDiffuseFbo) return;
        if (!finalImageFBO) return;
        if (!shader) return;

        IESProfile* iesProfile = AssetManager::GetIESProfileByName("Lamp0");
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

        shader->SetVec3("u_moonlightDir", Game::GetMoonlightDirection());
        shader->SetFloat("farPlane", Config::GetFarPlane());
        shader->SetVec2("u_viewportSize", glm::vec2(viewportWidth, viewportHeight));
        shader->SetInt("cascadeCount", cascadeLevels.size() + 1);
        for (size_t i = 0; i < cascadeLevels.size(); ++i) {
            shader->SetFloat("u_cascadePlaneDistances[" + std::to_string(i) + "]", cascadeLevels[i]);
        }

        glBindTextureUnit(0, gBuffer->GetColorAttachmentHandleByName("BaseColor"));
        glBindTextureUnit(1, gBuffer->GetColorAttachmentHandleByName("Normal"));
        glBindTextureUnit(2, gBuffer->GetColorAttachmentHandleByName("RMA"));
        glBindTextureUnit(3, gBuffer->GetDepthAttachmentHandle());
        glBindTextureUnit(4, gBuffer->GetColorAttachmentHandleByName("WorldPosition"));
        //glBindTextureUnit(5, miscFullSizeFBO->GetColorAttachmentHandleByName("ViewportIndex"));
        glBindTextureUnit(6, gBuffer->GetColorAttachmentHandleByName("Emissive"));
        glBindTextureUnit(7, AssetManager::GetTextureByName("Flashlight2")->GetGLTexture().GetHandle());
        glBindTextureUnit(8, flashLightShadowMapsFBO->GetDepthTextureHandle());

        glActiveTexture(GL_TEXTURE9);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, hiResShadowMaps->GetDepthTexture());

        OpenGLShadowMapArray* shadowMapArray = GetShadowMapArray("MoonlightCSM");
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_2D_ARRAY, shadowMapArray->GetDepthTexture());

        glBindTextureUnit(11, indirectDiffuseFbo->GetColorAttachmentHandleByName("Color"));

        BindSSBO("TileChristmasLights", 7);
        BindSSBO("ChristmasLightInstances", 8);
        BindSSBO("ChristmasLightIndices", 9);

        glBindImageTexture(0, gBuffer->GetColorAttachmentHandleByName("FinalLighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);

        glDispatchCompute(gBuffer->GetWidth() / TILE_SIZE, gBuffer->GetHeight() / TILE_SIZE, 1);
    }
}