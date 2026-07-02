#include "Unloved/Render/API/OpenGL/GL_renderer.h"

#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Config/Config.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/DDGI/GlobalIllumination.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Systems/Ocean/Ocean.h"

#include "World/LegacyWorld.h"
#include "Unloved/Render/Renderer.h"

#include "res/shaders/common/gl_fixed_bindings.glsl"

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
        OpenGLShadowCubeMapArray* lowResShadowMaps = OpenGL::ResourceManager::GetShadowCubeMapArrayPtr("LowRes");
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
        OpenGL::SetUniformBool("u_sampleProbes", Unloved::Renderer::GetCurrentRendererSettings().enableIrradianceProbeSampling);

        if (Unloved::LegacyWorld::HasOcean()) {
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

        glBindTextureUnit(4, gBuffer->GetColorAttachmentHandleByName("BaseColorMetallic"));
        glBindTextureUnit(5, gBuffer->GetColorAttachmentHandleByName("NormalXYRoughnessMisc"));
        glBindTextureUnit(6, gBuffer->GetColorAttachmentHandleByName("VelocityXYOcclusionSubSurface"));
        glBindTextureUnit(7, gBuffer->GetDepthAttachmentHandle());
        glBindTextureUnit(8, gBuffer->GetColorAttachmentHandleByName("Emissive"));
        glBindTextureUnit(9, GetTextureHandleByName("Flashlight2"));
        glBindTextureUnit(TEX_IDX_SHADOW_MAP_FLASHLIGHT, flashLightShadowMapsFBO->GetDepthTextureHandle());

        glBindTextureUnit(TEX_IDX_SHADOW_MAP_HI_RES, hiResShadowMaps->GetDepthTexture());
        glBindTextureUnit(TEX_IDX_SHADOW_MAP_LOW_RES, lowResShadowMaps->GetDepthTexture());

        OpenGLShadowMapArray* shadowMapArray = OpenGL::ResourceManager::GetShadowMapArrayPtr("MoonlightCSM");
        glBindTextureUnit(TEX_IDX_SHADOW_MAP_CSM, shadowMapArray->GetDepthTexture());

        glBindTextureUnit(10, indirectDiffuseFbo->GetColorAttachmentHandleByName("Color"));

        OpenGL::BindSSBO(7, "TileChristmasLights");
        OpenGL::BindSSBO(8, "ChristmasLightInstances");
        OpenGL::BindSSBO(9, "ChristmasLightIndices");

        glBindImageTexture(0, gBuffer->GetColorAttachmentHandleByName("Lighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);

        OpenGL::DispatchCompute(gBuffer->GetWidth() / TILE_SIZE, gBuffer->GetHeight() / TILE_SIZE, 1);
    }
}
