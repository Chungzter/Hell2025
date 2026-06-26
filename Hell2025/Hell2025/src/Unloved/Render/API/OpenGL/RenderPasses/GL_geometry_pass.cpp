#include "../GL_renderer.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Hell/Backend/BackEnd.h"
#include "Viewport/ViewportManager.h"
#include "Editor/Editor.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderDataManager.h"
#include "Modelling/Clipping.h"
#include "Modelling/Unused/Modelling.h"
#include "World/LegacyWorld.h"

#include "Hell/Logging.h"
#include "Hell/Physics/Physics.h"

#include "Types/Mirror.h"
#include "Managers/MirrorManager.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Core/GameOLD.h"

// get me out of here

using namespace Hell;

namespace OpenGLRenderer {
    void RenderNonDeformingAnimatedGameObjects();

	void HouseGeometryPass() {
		ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("GBuffer");

        if (!gBuffer) return;
        if (!shader) return;

        gBuffer->Bind();
        //gBuffer->DrawBuffers({ "BaseColor", "Normal", "RMA", "Emissive", "VelocityXYOcclusionSubSurface" });
        gBuffer->DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "Emissive", "VelocityXYOcclusionSubSurface" });
        OpenGLRasterizerStateManager::ForceRasterizerState("GeometryPass_Default");
        EditorRasterizerStateOverride();

        OpenGL::BindShader("GBuffer");
        OpenGL::SetUniformMat4("u_model", glm::mat4(1));
        OpenGL::SetUniformBool("u_flipNormalMapY", ShouldFlipNormalMapY());
        OpenGL::SetUniformBool("u_alphaDiscard", false);

        //MeshBuffer& houseMeshBuffer = LegacyWorld::GetHouseMeshBuffer();
        //OpenGLMeshBuffer& glHouseMeshBuffer = houseMeshBuffer.GetGLMeshBuffer();
        //glBindVertexArray(glHouseMeshBuffer.GetVAO());

        MeshBuffer& meshBuffer = ResourceManager::GetMeshBuffer("Procedural");
        glBindVertexArray(meshBuffer.GetVAO());

        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();
        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGLRenderer::SetViewport(gBuffer, viewport);

            if (Hell::BackEnd::RenderDocFound()) {
                SplitMultiDrawIndirect(shader, drawInfoSet.procedural[i], true, false);
            }
            else {
                MultiDrawIndirect(drawInfoSet.procedural[i]);
            }
        }
    }


    void RenderNonDeformingAnimatedGameObjects() {
        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("GBuffer");

        if (!gBuffer) return;
        if (!shader) return;

        gBuffer->Bind();
        //gBuffer->DrawBuffers({ "BaseColor", "Normal", "RMA", "Emissive", "VelocityXYOcclusionSubSurface" });
        gBuffer->DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "Emissive", "VelocityXYOcclusionSubSurface" });

        OpenGL::BindShader("GBuffer");

        //glBindVertexArray(OpenGL::BackEnd::GetWeightedVertexDataVAO());
        //glBindBuffer(GL_ARRAY_BUFFER, OpenGL::BackEnd::GetWeightedVertexDataVBO());
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGL::BackEnd::GetWeightedVertexDataEBO());
        MeshBuffer& meshBuffer = ResourceManager::GetMeshBuffer("AssetGeometry");
        glBindVertexArray(meshBuffer.GetVAO());
        glBindBuffer(GL_ARRAY_BUFFER, meshBuffer.GetVBO());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshBuffer.GetEBO());

        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();
        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();

        // Default
        OpenGL::SetUniformBool("u_alphaDiscard", false);
        OpenGLRasterizerStateManager::ForceRasterizerState("GeometryPass_Default");
        EditorRasterizerStateOverride();
        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGLRenderer::SetViewport(gBuffer, viewport);

            if (Hell::BackEnd::RenderDocFound()) {
                SplitMultiDrawIndirect(shader, drawInfoSet.skinnedNonDeformingStandard[i], true, false);
            }
            else {
                MultiDrawIndirect(drawInfoSet.skinnedNonDeformingStandard[i]);
            }
        }

        // Alpha Discard
        OpenGL::SetUniformBool("u_alphaDiscard", true);
        OpenGLRasterizerStateManager::ForceRasterizerState("GeometryPass_AlphaDiscard");
        EditorRasterizerStateOverride();
        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGLRenderer::SetViewport(gBuffer, viewport);

            if (Hell::BackEnd::RenderDocFound()) {
                SplitMultiDrawIndirect(shader, drawInfoSet.skinnedNonDeformingAlphaDiscard[i], true, false);
            }
            else {
                MultiDrawIndirect(drawInfoSet.skinnedNonDeformingAlphaDiscard[i]);
            }

            // Hair
            glDisable(GL_CULL_FACE);
            if (Hell::BackEnd::RenderDocFound()) {
                SplitMultiDrawIndirect(shader, drawInfoSet.skinnedNonDeformingHair[i], true, false);
            }
            else {
                MultiDrawIndirect(drawInfoSet.skinnedNonDeformingHair[i]);
            }
        }

        // Blended
        OpenGL::SetUniformBool("u_alphaDiscard", false);
        gBuffer->DrawBuffers({ "BaseColorMetallic" });
        //gBuffer->DrawBuffers({ "BaseColor" });
        OpenGLRasterizerStateManager::ForceRasterizerState("GeometryPass_Blended");
        EditorRasterizerStateOverride();
        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGLRenderer::SetViewport(gBuffer, viewport);

            if (Hell::BackEnd::RenderDocFound()) {
                SplitMultiDrawIndirect(shader, drawInfoSet.skinnedNonDeformingBlended[i], true, false);
            }
            else {
                MultiDrawIndirect(drawInfoSet.skinnedNonDeformingBlended[i]);
            }
        }
    }


    void GeometryPass() {
        ProfilerOpenGLZoneFunction();

        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();
        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();

        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("GBuffer");
        OpenGLShader* editorMeshShader = OpenGL::ResourceManager::GetShaderPtr("EditorMesh");
        OpenGLTextureArray* woundMaskArray = OpenGL::ResourceManager::GetTextureArrayPtr("WoundMasks");

        if (!gBuffer) return;
        if (!shader) return;
        if (!editorMeshShader) return;
        if (!woundMaskArray) return;

        {
            MeshBuffer& meshBuffer = ResourceManager::GetMeshBuffer("AssetGeometry");
            glBindVertexArray(meshBuffer.GetVAO());
        }
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D_ARRAY, woundMaskArray->GetHandle());


        OpenGL::BindShader("GBuffer");
        OpenGL::SetUniformBool("u_flipNormalMapY", ShouldFlipNormalMapY());

        OpenGLFrameBuffer* decalMasksFBO = OpenGL::ResourceManager::GetFrameBufferPtr("DecalMasks");

        gBuffer->Bind();
        //gBuffer->DrawBuffers({ "BaseColor", "Normal", "RMA", "Emissive", "VelocityXYOcclusionSubSurface" });
        gBuffer->DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "Emissive", "VelocityXYOcclusionSubSurface" });

        // Default (Non blended)
        OpenGL::SetUniformBool("u_alphaDiscard", false);
        OpenGLRasterizerStateManager::ForceRasterizerState("GeometryPass_Default");
        EditorRasterizerStateOverride();

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGLRenderer::SetViewport(gBuffer, viewport);
                if (Hell::BackEnd::RenderDocFound()) {
                    SplitMultiDrawIndirect(shader, drawInfoSet.standard[i], true, false);
                }
                else {
                    MultiDrawIndirect(drawInfoSet.standard[i]);
                }
            }
        }

        // Alpha discard
        OpenGL::SetUniformBool("u_alphaDiscard", true);
        OpenGLRasterizerStateManager::ForceRasterizerState("GeometryPass_AlphaDiscard");
        EditorRasterizerStateOverride();

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGLRenderer::SetViewport(gBuffer, viewport);
                if (Hell::BackEnd::RenderDocFound()) {
                    SplitMultiDrawIndirect(shader, drawInfoSet.alphaDiscard[i], true, false);
                }
                else {
                    MultiDrawIndirect(drawInfoSet.alphaDiscard[i]);
                }

                // Hair
                glDisable(GL_CULL_FACE);
                if (Hell::BackEnd::RenderDocFound()) {
                    SplitMultiDrawIndirect(shader, drawInfoSet.hair[i], true, false);
                }
                else {
                    MultiDrawIndirect(drawInfoSet.hair[i]);
                }
            }
        }

        // Blended
        OpenGL::SetUniformBool("u_alphaDiscard", false);
        //gBuffer->DrawBuffers({ "BaseColor" });
        gBuffer->DrawBuffers({ "BaseColorMetallic" });
        OpenGLRasterizerStateManager::ForceRasterizerState("GeometryPass_Blended");
        EditorRasterizerStateOverride();

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGLRenderer::SetViewport(gBuffer, viewport);
                if (Hell::BackEnd::RenderDocFound()) {
                    SplitMultiDrawIndirect(shader, drawInfoSet.blended[i], true, false);
                }
                else {
                    MultiDrawIndirect(drawInfoSet.blended[i]);
                }
            }
        }


        glBindVertexArray(OpenGL::BackEnd::GetSkinnedVertexDataVAO());
        glBindBuffer(GL_ARRAY_BUFFER, OpenGL::BackEnd::GetSkinnedVertexDataVBO());
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGL::BackEnd::GetWeightedVertexDataEBO());
        {
            MeshBuffer& meshBuffer = ResourceManager::GetMeshBuffer("AssetGeometry");
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshBuffer.GetEBO());
        }

        OpenGL::BindShader("GBuffer");
        //gBuffer->DrawBuffers({ "BaseColor", "Normal", "RMA", "Emissive", "VelocityXYOcclusionSubSurface" });
        gBuffer->DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "Emissive", "VelocityXYOcclusionSubSurface" });

        // Skinned mesh (non blended)
        OpenGL::SetUniformBool("u_alphaDiscard", false);
        OpenGLRasterizerStateManager::ForceRasterizerState("GeometryPass_Default");
        EditorRasterizerStateOverride();
        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGLRenderer::SetViewport(gBuffer, viewport);

            if (Hell::BackEnd::RenderDocFound()) {
                SplitMultiDrawIndirect(shader, drawInfoSet.skinnedStandard[i], true, true);
            }
            else {
                MultiDrawIndirect(drawInfoSet.skinnedStandard[i]);
            }
        }

        // Skinned mesh (alpha discard)
        OpenGL::SetUniformBool("u_alphaDiscard", true);
        OpenGLRasterizerStateManager::ForceRasterizerState("GeometryPass_AlphaDiscard");
        EditorRasterizerStateOverride();
        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGLRenderer::SetViewport(gBuffer, viewport);

            if (Hell::BackEnd::RenderDocFound()) {
                SplitMultiDrawIndirect(shader, drawInfoSet.skinnedAlphaDiscard[i], true, false);
            }
            else {
                MultiDrawIndirect(drawInfoSet.skinnedAlphaDiscard[i]);
            }

            // Hair
            glDisable(GL_CULL_FACE);
            if (Hell::BackEnd::RenderDocFound()) {
                SplitMultiDrawIndirect(shader, drawInfoSet.skinnedHair[i], true, false);
            }
            else {
                MultiDrawIndirect(drawInfoSet.skinnedHair[i]);
            }
        }

        // Skinned mesh (alpha blended)
        OpenGL::SetUniformBool("u_alphaDiscard", false);
        gBuffer->DrawBuffers({ "BaseColorMetallic" });
        //gBuffer->DrawBuffers({ "BaseColor" });
        OpenGLRasterizerStateManager::ForceRasterizerState("GeometryPass_Blended");
        EditorRasterizerStateOverride();
        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGLRenderer::SetViewport(gBuffer, viewport);

            if (Hell::BackEnd::RenderDocFound()) {
                SplitMultiDrawIndirect(shader, drawInfoSet.skinnedBlended[i], true, true);
            }
            else {
                MultiDrawIndirect(drawInfoSet.skinnedBlended[i]);
            }
        }


        //gBuffer->DrawBuffers({ "BaseColor", "Normal", "RMA", "Emissive", "VelocityXYOcclusionSubSurface" });
        gBuffer->DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "Emissive", "VelocityXYOcclusionSubSurface" });

        OpenGLShader* christmasLightWireShader = OpenGL::ResourceManager::GetShaderPtr("ChristmasLightsWire");
        OpenGL::BindShader("ChristmasLightsWire");
        OpenGLRasterizerStateManager::ForceRasterizerState("GeometryPass_Default");
        EditorRasterizerStateOverride();

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGLRenderer::SetViewport(gBuffer, viewport);

                OpenGL::SetUniformInt("playerIndex", i);
                OpenGL::SetUniformMat4("u_projectionView", viewportData[i].projectionViewReverseZ);

                // Draw Christmas light wires
                for (ChristmasLightSet& lights : LegacyWorld::GetChristmasLightSets()) {
                    std::vector<Wire>& wires = lights.GetWires();
                    for (Wire& wire : wires) {
                        MeshBufferOLD& meshBuffer = wire.GetMeshBuffer();
                        OpenGLMeshBufferOLD& glMeshBuffer = meshBuffer.GetGLMeshBuffer();
                        glBindVertexArray(glMeshBuffer.GetVAO());
                        glDrawElements(GL_TRIANGLES, glMeshBuffer.GetIndexCount(), GL_UNSIGNED_INT, 0);
                    }
                }

                // Draw power pole wires
                for (PowerPoleSet& powerPoleSet : LegacyWorld::GetPowerPoleSets()) {
                    std::vector<Wire>& wires = powerPoleSet.GetWires();
                    for (Wire& wire : wires) {
                        MeshBufferOLD& meshBuffer = wire.GetMeshBuffer();
                        OpenGLMeshBufferOLD& glMeshBuffer = meshBuffer.GetGLMeshBuffer();
                        glBindVertexArray(glMeshBuffer.GetVAO());
                        glDrawElements(GL_TRIANGLES, glMeshBuffer.GetIndexCount(), GL_UNSIGNED_INT, 0);
                    }
                }
            }
        }

        // Debug draw ragdolls
        if (Renderer::GetCurrentRendererSettings().debugDrawRagdolls) {
            OpenGLShader* ragdollShader = OpenGL::ResourceManager::GetShaderPtr("DebugRagdoll");
            OpenGL::BindShader("DebugRagdoll");
            OpenGLRasterizerStateManager::ForceRasterizerState("GeometryPass_Default");
            EditorRasterizerStateOverride();

            MeshBuffer& physicsDebugGeometry = ResourceManager::GetMeshBuffer("PhysicsDebugGeometry");
            if (physicsDebugGeometry.GetMeshCount() > 0) {
                for (int i = 0; i < 4; i++) {
                    Viewport* viewport = ViewportManager::GetViewportByIndex(i);
                    if (viewport->IsVisible()) {
                        OpenGLRenderer::SetViewport(gBuffer, viewport);

                        Player* player = GameOLD::GetLocalPlayerByIndex(i);
                        if (!player) continue;

                        OpenGL::SetUniformInt("u_playerIndex", i);
                        OpenGL::SetUniformMat4("u_projectionView", viewportData[i].projectionViewReverseZ);

                        glBindVertexArray(physicsDebugGeometry.GetVAO());

                        // Ragdoll
                        auto& ragdolls = Hell::Physics::GetRagdolls();

                        for (auto it = ragdolls.begin(); it != ragdolls.end(); ) {
                            Ragdoll& ragdoll = it->second;

                            // Dont render current viewports ragdoll. It blocks the screen.
                            bool skipRendering = (player->GetRagdollId() == it->first);

                            if (!skipRendering) {
                                for (uint32_t rigidIndex = 0; rigidIndex < ragdoll.m_pxRigidDynamics.size(); rigidIndex++) {
                                    const uint32_t meshId = ragdoll.GetMarkerDebugMeshIdByRigidIndex(rigidIndex);
                                    if (meshId == 0) continue;

                                    Mesh* mesh = physicsDebugGeometry.GetMeshById(meshId);
                                    if (!mesh) continue;

                                    glm::mat4 modelMatrix = ragdoll.GetModelMatrixByRigidIndex(rigidIndex);
                                    OpenGL::SetUniformMat4("u_model", modelMatrix);
                                    OpenGL::SetUniformVec3("u_color", ragdoll.GetMarkerColorByRigidIndex(rigidIndex));

                                    glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
                                }
                            }
                            it++;
                        }
                    }
                }
            }
        }

        glBindVertexArray(0);

        RenderNonDeformingAnimatedGameObjects();
    }

    void MirrorGeometryPass() {
        ProfilerOpenGLZoneFunction();

        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();
        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();

        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLFrameBuffer* gBufferBackup = OpenGL::ResourceManager::GetFrameBufferPtr("GBufferBackup");
        OpenGLShader* geometryShader = OpenGL::ResourceManager::GetShaderPtr("GBuffer");
        OpenGLShader* houseGeometryShader = OpenGL::ResourceManager::GetShaderPtr("DebugTextured");
        OpenGLShader* solidColorShader = OpenGL::ResourceManager::GetShaderPtr("DebugSolidColor");

        if (!gBuffer) return;
        if (!gBufferBackup) return;
        if (!geometryShader) return;
        if (!houseGeometryShader) return;
        if (!solidColorShader) return;

        // Render the mirror mask
        // - First you copy the depth buffer from the GBuffer so you can render your mirror plane against scene depth
        // - Then you just do a standard stencil buffer mask write for each viewport
        OpenGL::BlitFrameBufferDepth(gBuffer, gBufferBackup);

        gBuffer->Bind();
        gBuffer->BindDepthAttachmentFrom(*gBufferBackup);

        OpenGL::BindShader("DebugSolidColor");

        //gBuffer->DrawBuffer("BaseColor");

        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE); // Test depth, but don't write it
        glDepthFunc(GL_GEQUAL);

        glEnable(GL_STENCIL_TEST);
        glStencilMask(0xFF);
        glClearStencil(0);
        glClear(GL_STENCIL_BUFFER_BIT);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

        //glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        //glDisable(GL_DEPTH_TEST);

        MeshBuffer& meshBufferAssets = ResourceManager::GetMeshBuffer("AssetGeometry");
        MeshBuffer& meshBufferProcedural = ResourceManager::GetMeshBuffer("Procedural");

        glBindVertexArray(meshBufferAssets.GetVAO());

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGLRenderer::SetViewport(gBuffer, viewport);

                Mirror* mirror = MirrorManager::GetMirrorByObjectId(viewport->GetMirrorId());
                if (!mirror) continue;

                //mirror->DebugDraw();

                Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(mirror->GetGlobalMeshIndex());
                if (!mesh) continue;

                glm::mat4 modelMatrix = mirror->GetWorldMatrix();

                OpenGL::SetUniformMat4("u_projectionView", viewportData[i].projectionViewReverseZ);
                OpenGL::SetUniformMat4("u_model", modelMatrix);

                glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (GLvoid*)(mesh->baseIndex * sizeof(GLuint)), mesh->baseVertex);
            }
        }

        glDepthMask(GL_TRUE);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        gBuffer->Bind();
        //gBuffer->DrawBuffers({ "BaseColor", "Normal", "RMA", "Emissive" });
        gBuffer->DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "Emissive", "VelocityXYOcclusionSubSurface" });

        // Clear the depth buffer so that the mirror world has a clean depth state to test against
        gBuffer->ClearDepthAttachment(0.0);

        OpenGLRasterizerStateManager::ForceRasterizerState("GeometryPass_Default");

        glEnable(GL_CLIP_DISTANCE0);
        glFrontFace(GL_CW);

        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_EQUAL, 1, 0xFF);
        glStencilMask(0x00);

        OpenGL::BindShader("GBuffer");
        OpenGL::SetUniformBool("u_flipNormalMapY", ShouldFlipNormalMapY());

        // Regular geometry
        glBindVertexArray(meshBufferAssets.GetVAO());

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGLRenderer::SetViewport(gBuffer, viewport);

                Mirror* mirror = MirrorManager::GetMirrorByObjectId(viewport->GetMirrorId());
                if (!mirror) continue;

                OpenGL::SetUniformBool("u_useMirrorMatrix", true);
                OpenGL::SetUniformMat4("u_mirrorViewMatrix", mirror->GetViewMatrix(i));
                OpenGL::SetUniformVec4("u_mirrorClipPlane", mirror->GetClipPlane(i));

                OpenGLRenderer::SetViewport(gBuffer, viewport);
                if (Hell::BackEnd::RenderDocFound()) {
                    SplitMultiDrawIndirect(geometryShader, drawInfoSet.mirrorRenderItems[i], true, false);
                }
                else {
                    MultiDrawIndirect(drawInfoSet.mirrorRenderItems[i]);
                }
            }
        }
        OpenGL::SetUniformBool("u_useMirrorMatrix", false);

        // House geometry
        OpenGL::BindShader("DebugTextured");
        OpenGL::SetUniformMat4("u_model", glm::mat4(1));
        OpenGL::SetUniformBool("u_flipNormalMapY", ShouldFlipNormalMapY());

        glBindVertexArray(meshBufferProcedural.GetVAO());

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            Mirror* mirror = MirrorManager::GetMirrorByObjectId(viewport->GetMirrorId());
            if (!mirror) continue;

            OpenGLRenderer::SetViewport(gBuffer, viewport);

            OpenGL::SetUniformInt("u_viewportIndex", i);
            OpenGL::SetUniformBool("u_useMirrorMatrix", true);
            OpenGL::SetUniformMat4("u_mirrorViewMatrix", mirror->GetViewMatrix(i));
            OpenGL::SetUniformVec4("u_mirrorClipPlane", mirror->GetClipPlane(i));

            const std::vector<RenderItem>& renderItems = RenderDataManager::GetRenderItemsProcedural();
            for (const RenderItem& renderItem : renderItems) {

                Mesh* mesh = meshBufferProcedural.GetMeshById(renderItem.meshId);
                if (!mesh) continue;

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.baseColorTextureIndex)->GetGLTexture().GetHandle());
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.normalMapTextureIndex)->GetGLTexture().GetHandle());
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.rmaTextureIndex)->GetGLTexture().GetHandle());

                int indexCount = mesh->indexCount;
                int baseVertex = renderItem.baseVertex;
                int baseIndex = renderItem.baseIndex;

                glDrawElementsBaseVertex(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * baseIndex), baseVertex);
            }
        }
        OpenGL::SetUniformBool("u_useMirrorMatrix", false);

        // Clean up
        glStencilMask(0xFF);
        glDisable(GL_CLIP_DISTANCE0);
        glFrontFace(GL_CCW);
        glDisable(GL_STENCIL_TEST);

        gBuffer->BindDepthAttachmentFrom(*gBuffer);
    }
}
