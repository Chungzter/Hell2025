#include "../GL_renderer.h"
#include "../../GL_backend.h"
#include "AssetManagement/AssetManager.h"
#include "BackEnd/Backend.h"
#include "Viewport/ViewportManager.h"
#include "Editor/Editor.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderDataManager.h"
#include "Modelling/Clipping.h"
#include "Modelling/Unused/Modelling.h"
#include "World/World.h"

#include "Ragdoll/RagdollManager.h"
#include "Input/Input.h"
#include "Hell/Logging.h"
#include "Physics/Physics.h"

#include "Types/Mirror.h"
#include "Managers/MirrorManager.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Core/Game.h"

// get me out of here
#include "AssetManagement/AssetManager.h"
// get me out of here

using namespace Hell;

namespace OpenGLRenderer {
    void RenderNonDeformingAnimatedGameObjects();

	void HouseGeometryPass() {
		ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer* gBuffer = GetFrameBufferOLD("GBuffer");
        OpenGLShader* shader = GetShaderOLD("GBuffer");

        if (!gBuffer) return;
        if (!shader) return;

        gBuffer->Bind();
        //gBuffer->DrawBuffers({ "BaseColor", "Normal", "RMA", "Emissive", "VelocityXYOcclusionSubSurface" });
        gBuffer->DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "Emissive", "VelocityXYOcclusionSubSurface" });
        ForceRasterizerState("GeometryPass_Default");
        EditorRasterizerStateOverride();

        shader->Bind();
        shader->SetMat4("u_model", glm::mat4(1));
        shader->SetBool("u_flipNormalMapY", ShouldFlipNormalMapY());
        shader->SetBool("u_alphaDiscard", false);

        //MeshBuffer& houseMeshBuffer = World::GetHouseMeshBuffer();
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

            if (BackEnd::RenderDocFound()) {
                SplitMultiDrawIndirect(shader, drawInfoSet.procedural[i], true, false);
            }
            else {
                MultiDrawIndirect(drawInfoSet.procedural[i]);
            }
        }
    }


    void RenderNonDeformingAnimatedGameObjects() {
        OpenGLFrameBuffer* gBuffer = GetFrameBufferOLD("GBuffer");
        OpenGLShader* shader = GetShaderOLD("GBuffer");

        if (!gBuffer) return;
        if (!shader) return;

        gBuffer->Bind();
        //gBuffer->DrawBuffers({ "BaseColor", "Normal", "RMA", "Emissive", "VelocityXYOcclusionSubSurface" });
        gBuffer->DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "Emissive", "VelocityXYOcclusionSubSurface" });

        shader->Bind();

        //glBindVertexArray(OpenGLBackEnd::GetWeightedVertexDataVAO());
        //glBindBuffer(GL_ARRAY_BUFFER, OpenGLBackEnd::GetWeightedVertexDataVBO());
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGLBackEnd::GetWeightedVertexDataEBO());
        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
        glBindBuffer(GL_ARRAY_BUFFER, OpenGLBackEnd::GetVertexDataVBO());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGLBackEnd::GetVertexDataEBO());

        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();
        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();

        // Default
        shader->SetBool("u_alphaDiscard", false);
        ForceRasterizerState("GeometryPass_Default");
        EditorRasterizerStateOverride();
        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGLRenderer::SetViewport(gBuffer, viewport);

            if (BackEnd::RenderDocFound()) {
                SplitMultiDrawIndirect(shader, drawInfoSet.skinnedNonDeformingStandard[i], true, false);
            }
            else {
                MultiDrawIndirect(drawInfoSet.skinnedNonDeformingStandard[i]);
            }
        }

        // Alpha Discard
        shader->SetBool("u_alphaDiscard", true);
        ForceRasterizerState("GeometryPass_AlphaDiscard");
        EditorRasterizerStateOverride();
        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGLRenderer::SetViewport(gBuffer, viewport);

            if (BackEnd::RenderDocFound()) {
                SplitMultiDrawIndirect(shader, drawInfoSet.skinnedNonDeformingAlphaDiscard[i], true, false);
            }
            else {
                MultiDrawIndirect(drawInfoSet.skinnedNonDeformingAlphaDiscard[i]);
            }

            // Hair
            glDisable(GL_CULL_FACE);
            if (BackEnd::RenderDocFound()) {
                SplitMultiDrawIndirect(shader, drawInfoSet.skinnedNonDeformingHair[i], true, false);
            }
            else {
                MultiDrawIndirect(drawInfoSet.skinnedNonDeformingHair[i]);
            }
        }

        // Blended
        shader->SetBool("u_alphaDiscard", false);
        gBuffer->DrawBuffers({ "BaseColorMetallic" });
        //gBuffer->DrawBuffers({ "BaseColor" });
        ForceRasterizerState("GeometryPass_Blended");
        EditorRasterizerStateOverride();
        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGLRenderer::SetViewport(gBuffer, viewport);

            if (BackEnd::RenderDocFound()) {
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

        OpenGLFrameBuffer* gBuffer = GetFrameBufferOLD("GBuffer");
        OpenGLShader* shader = GetShaderOLD("GBuffer");
        OpenGLShader* editorMeshShader = GetShaderOLD("EditorMesh");
        OpenGLTextureArray* woundMaskArray = GetTextureArray("WoundMasks");

        if (!gBuffer) return;
        if (!shader) return;
        if (!editorMeshShader) return;
        if (!woundMaskArray) return;

        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D_ARRAY, woundMaskArray->GetHandle());


        shader->Bind();
        shader->SetBool("u_flipNormalMapY", ShouldFlipNormalMapY());

        OpenGLFrameBuffer* decalMasksFBO = GetFrameBufferOLD("DecalMasks");

        gBuffer->Bind();
        //gBuffer->DrawBuffers({ "BaseColor", "Normal", "RMA", "Emissive", "VelocityXYOcclusionSubSurface" });
        gBuffer->DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "Emissive", "VelocityXYOcclusionSubSurface" });

        // Default (Non blended)
        shader->SetBool("u_alphaDiscard", false);
        ForceRasterizerState("GeometryPass_Default");
        EditorRasterizerStateOverride();

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGLRenderer::SetViewport(gBuffer, viewport);
                if (BackEnd::RenderDocFound()) {
                    SplitMultiDrawIndirect(shader, drawInfoSet.standard[i], true, false);
                }
                else {
                    MultiDrawIndirect(drawInfoSet.standard[i]);
                }
            }
        }

        // Alpha discard
        shader->SetBool("u_alphaDiscard", true);
        ForceRasterizerState("GeometryPass_AlphaDiscard");
        EditorRasterizerStateOverride();

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGLRenderer::SetViewport(gBuffer, viewport);
                if (BackEnd::RenderDocFound()) {
                    SplitMultiDrawIndirect(shader, drawInfoSet.alphaDiscard[i], true, false);
                }
                else {
                    MultiDrawIndirect(drawInfoSet.alphaDiscard[i]);
                }

                // Hair
                glDisable(GL_CULL_FACE);
                if (BackEnd::RenderDocFound()) {
                    SplitMultiDrawIndirect(shader, drawInfoSet.hair[i], true, false);
                }
                else {
                    MultiDrawIndirect(drawInfoSet.hair[i]);
                }
            }
        }

        // Blended
        shader->SetBool("u_alphaDiscard", false);
        //gBuffer->DrawBuffers({ "BaseColor" });
        gBuffer->DrawBuffers({ "BaseColorMetallic" });
        ForceRasterizerState("GeometryPass_Blended");
        EditorRasterizerStateOverride();

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGLRenderer::SetViewport(gBuffer, viewport);
                if (BackEnd::RenderDocFound()) {
                    SplitMultiDrawIndirect(shader, drawInfoSet.blended[i], true, false);
                }
                else {
                    MultiDrawIndirect(drawInfoSet.blended[i]);
                }
            }
        }








        glBindVertexArray(OpenGLBackEnd::GetSkinnedVertexDataVAO());
        glBindBuffer(GL_ARRAY_BUFFER, OpenGLBackEnd::GetSkinnedVertexDataVBO());
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGLBackEnd::GetWeightedVertexDataEBO());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGLBackEnd::GetVertexDataEBO());

        shader->Bind();
        //gBuffer->DrawBuffers({ "BaseColor", "Normal", "RMA", "Emissive", "VelocityXYOcclusionSubSurface" });
        gBuffer->DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "Emissive", "VelocityXYOcclusionSubSurface" });

        // Skinned mesh (non blended)
        shader->SetBool("u_alphaDiscard", false);
        ForceRasterizerState("GeometryPass_Default");
        EditorRasterizerStateOverride();
        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGLRenderer::SetViewport(gBuffer, viewport);

            if (BackEnd::RenderDocFound()) {
                SplitMultiDrawIndirect(shader, drawInfoSet.skinnedStandard[i], true, true);
            }
            else {
                MultiDrawIndirect(drawInfoSet.skinnedStandard[i]);
            }
        }

        // Skinned mesh (alpha discard)
        shader->SetBool("u_alphaDiscard", true);
        ForceRasterizerState("GeometryPass_AlphaDiscard");
        EditorRasterizerStateOverride();
        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGLRenderer::SetViewport(gBuffer, viewport);

            if (BackEnd::RenderDocFound()) {
                SplitMultiDrawIndirect(shader, drawInfoSet.skinnedAlphaDiscard[i], true, false);
            }
            else {
                MultiDrawIndirect(drawInfoSet.skinnedAlphaDiscard[i]);
            }

            // Hair
            glDisable(GL_CULL_FACE);
            if (BackEnd::RenderDocFound()) {
                SplitMultiDrawIndirect(shader, drawInfoSet.skinnedHair[i], true, false);
            }
            else {
                MultiDrawIndirect(drawInfoSet.skinnedHair[i]);
            }
        }

        // Skinned mesh (alpha blended)
        shader->SetBool("u_alphaDiscard", false);
        gBuffer->DrawBuffers({ "BaseColorMetallic" });
        //gBuffer->DrawBuffers({ "BaseColor" });
        ForceRasterizerState("GeometryPass_Blended");
        EditorRasterizerStateOverride();
        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGLRenderer::SetViewport(gBuffer, viewport);

            if (BackEnd::RenderDocFound()) {
                SplitMultiDrawIndirect(shader, drawInfoSet.skinnedBlended[i], true, true);
            }
            else {
                MultiDrawIndirect(drawInfoSet.skinnedBlended[i]);
            }
        }









        //gBuffer->DrawBuffers({ "BaseColor", "Normal", "RMA", "Emissive", "VelocityXYOcclusionSubSurface" });
        gBuffer->DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "Emissive", "VelocityXYOcclusionSubSurface" });

        OpenGLShader* christmasLightWireShader = GetShaderOLD("ChristmasLightsWire");
        christmasLightWireShader->Bind();
        ForceRasterizerState("GeometryPass_Default");
        EditorRasterizerStateOverride();

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGLRenderer::SetViewport(gBuffer, viewport);

                christmasLightWireShader->SetInt("playerIndex", i);
                christmasLightWireShader->SetMat4("u_projectionView", viewportData[i].projectionViewReverseZ);

                // Draw Christmas light wires
                for (ChristmasLightSet& lights : World::GetChristmasLightSets()) {
                    std::vector<Wire>& wires = lights.GetWires();
                    for (Wire& wire : wires) {
                        MeshBufferOLD& meshBuffer = wire.GetMeshBuffer();
                        OpenGLMeshBufferOLD& glMeshBuffer = meshBuffer.GetGLMeshBuffer();
                        glBindVertexArray(glMeshBuffer.GetVAO());
                        glDrawElements(GL_TRIANGLES, glMeshBuffer.GetIndexCount(), GL_UNSIGNED_INT, 0);
                    }
                }

                // Draw power pole wires
                for (PowerPoleSet& powerPoleSet : World::GetPowerPoleSets()) {
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

        OpenGLShader* ragdollShader = GetShaderOLD("DebugRagdoll");
        ragdollShader->Bind();
        ForceRasterizerState("GeometryPass_Default");
        EditorRasterizerStateOverride();

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGLRenderer::SetViewport(gBuffer, viewport);

                ragdollShader->SetInt("u_playerIndex", i);
                ragdollShader->SetMat4("u_projectionView", viewportData[i].projectionViewReverseZ);

                // Ragdoll
                auto& ragdolls = RagdollManager::GetRagdolls();

                for (auto it = ragdolls.begin(); it != ragdolls.end(); ) {
                    RagdollV2& ragdoll = it->second;

                    if (ragdoll.RenderingEnabled()) {
                        MeshBufferOLD& meshBuffer = ragdoll.GetMeshBuffer();
                        glBindVertexArray(meshBuffer.GetGLMeshBuffer().GetVAO());

                        for (int j = 0; j < meshBuffer.GetMeshCount(); j++) {
                            if (meshBuffer.GetIndices().size() == 0) continue;

                            Mesh* mesh = meshBuffer.GetMeshByIndex(j);
                            glm::mat4 modelMatrix = ragdoll.GetModelMatrixByRigidIndex(j);
                            ragdollShader->SetMat4("u_model", modelMatrix);
                            ragdollShader->SetVec3("u_color", ragdoll.GetMarkerColorByRigidIndex(j));

                            glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
                        }
                    }
                    it++;
                }

                // Door debug
                //for (Door& door : World::GetDoors()) {
                //    MeshBuffer& meshBuffer = door.m_raytracingDoorMesh;
                //    glBindVertexArray(meshBuffer.GetGLMeshBuffer().GetVAO());
                //
                //    for (int j = 0; j < meshBuffer.GetMeshCount(); j++) {
                //        if (meshBuffer.GetIndices().size() == 0) continue;
                //
                //        MeshNode* meshNode = door.GetMeshNodes().GetMeshNodeByMeshName("Door_Hinges");
                //        glm::mat4 modelMatrix = meshNode->worldMatrix;
                //        modelMatrix[3][1] = door.GetDoorModelMatrix()[3][1];
                //
                //        Mesh* mesh = meshBuffer.GetMeshByIndex(j);
                //        ragdollShader->SetMat4("u_model", modelMatrix);
                //        ragdollShader->SetVec3("u_color", glm::vec3(1, 0, 0));
                //
                //        //glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
                //    }
                //}

                //for (DDGIVolume& volume : World::GetDDGIVolumes()) {
                //    MeshBuffer& meshBuffer = volume.m_staticMeshBuffer;
                //    glBindVertexArray(meshBuffer.GetGLMeshBuffer().GetVAO());
                //
                //
                //    for (int j = 0; j < meshBuffer.GetMeshCount(); j++) {
                //        if (meshBuffer.GetIndices().size() == 0) continue;
                //
                //        Mesh* mesh = meshBuffer.GetMeshByIndex(j);
                //        ragdollShader->SetMat4("u_model", glm::mat4(1.0f));
                //        ragdollShader->SetVec3("u_color", glm::vec3(1, 0, 0));
                //
                //        std::cout << meshBuffer.GetVertices().size() << "\n";
                //
                //        glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
                //    }
                //
                //    break;
                //}
            }
        }

        glBindVertexArray(0);

        RenderNonDeformingAnimatedGameObjects();
    }

    void MirrorGeometryPass() {
        ProfilerOpenGLZoneFunction();

        const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();
        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();

        OpenGLFrameBuffer* gBuffer = GetFrameBufferOLD("GBuffer");
        OpenGLFrameBuffer* gBufferBackup = GetFrameBufferOLD("GBufferBackup");
        OpenGLShader* geometryShader = GetShaderOLD("GBuffer");
        OpenGLShader* houseGeometryShader = GetShaderOLD("DebugTextured");
        OpenGLShader* solidColorShader = GetShaderOLD("DebugSolidColor");

        if (!gBuffer) return;
        if (!gBufferBackup) return;
        if (!geometryShader) return;
        if (!houseGeometryShader) return;
        if (!solidColorShader) return;

        // Render the mirror mask
        // - First you copy the depth buffer from the GBuffer so you can render your mirror plane against scene depth
        // - Then you just do a standard stencil buffer mask write for each viewport
        OpenGLRenderer::BlitFrameBufferDepth(gBuffer, gBufferBackup);

        gBuffer->Bind();
        gBuffer->BindDepthAttachmentFrom(*gBufferBackup);

        solidColorShader->Bind();

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

        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGLRenderer::SetViewport(gBuffer, viewport);

                Mirror* mirror = MirrorManager::GetMirrorByObjectId(viewport->GetMirrorId());
                if (!mirror) continue;

                //mirror->DebugDraw();

                Mesh* mesh = AssetManager::GetMeshByIndex(mirror->GetGlobalMeshIndex());
                if (!mesh) continue;

                glm::mat4 modelMatrix = mirror->GetWorldMatrix();

                solidColorShader->SetMat4("u_projectionView", viewportData[i].projectionViewReverseZ);
                solidColorShader->SetMat4("u_model", modelMatrix);

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

        ForceRasterizerState("GeometryPass_Default");

        glEnable(GL_CLIP_DISTANCE0);
        glFrontFace(GL_CW);

        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_EQUAL, 1, 0xFF);
        glStencilMask(0x00);

        geometryShader->Bind();
        geometryShader->SetBool("u_flipNormalMapY", ShouldFlipNormalMapY());

        // Regular geometry
        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGLRenderer::SetViewport(gBuffer, viewport);

                Mirror* mirror = MirrorManager::GetMirrorByObjectId(viewport->GetMirrorId());
                if (!mirror) continue;

                geometryShader->SetBool("u_useMirrorMatrix", true);
                geometryShader->SetMat4("u_mirrorViewMatrix", mirror->GetViewMatrix(i));
                geometryShader->SetVec4("u_mirrorClipPlane", mirror->GetClipPlane(i));

                OpenGLRenderer::SetViewport(gBuffer, viewport);
                if (BackEnd::RenderDocFound()) {
                    SplitMultiDrawIndirect(geometryShader, drawInfoSet.mirrorRenderItems[i], true, false);
                }
                else {
                    MultiDrawIndirect(drawInfoSet.mirrorRenderItems[i]);
                }
            }
        }
        geometryShader->SetBool("u_useMirrorMatrix", false);

        // House geometry
        houseGeometryShader->Bind();
        houseGeometryShader->SetMat4("u_model", glm::mat4(1));
        houseGeometryShader->SetBool("u_flipNormalMapY", ShouldFlipNormalMapY());

        MeshBuffer& meshBuffer = ResourceManager::GetMeshBuffer("Procedural");
        glBindVertexArray(meshBuffer.GetVAO());

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            Mirror* mirror = MirrorManager::GetMirrorByObjectId(viewport->GetMirrorId());
            if (!mirror) continue;

            OpenGLRenderer::SetViewport(gBuffer, viewport);

            houseGeometryShader->SetInt("u_viewportIndex", i);
            houseGeometryShader->SetBool("u_useMirrorMatrix", true);
            houseGeometryShader->SetMat4("u_mirrorViewMatrix", mirror->GetViewMatrix(i));
            houseGeometryShader->SetVec4("u_mirrorClipPlane", mirror->GetClipPlane(i));

            const std::vector<RenderItem>& renderItems = RenderDataManager::GetRenderItemsProcedural();
            for (const RenderItem& renderItem : renderItems) {

                Mesh* mesh = meshBuffer.GetMeshById(renderItem.meshId);
                if (!mesh) continue;

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(renderItem.baseColorTextureIndex)->GetGLTexture().GetHandle());
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(renderItem.normalMapTextureIndex)->GetGLTexture().GetHandle());
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(renderItem.rmaTextureIndex)->GetGLTexture().GetHandle());

                int indexCount = mesh->indexCount;
                int baseVertex = renderItem.baseVertex;
                int baseIndex = renderItem.baseIndex;

                glDrawElementsBaseVertex(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * baseIndex), baseVertex);
            }
        }
        houseGeometryShader->SetBool("u_useMirrorMatrix", false);

        // Clean up
        glStencilMask(0xFF);
        glDisable(GL_CLIP_DISTANCE0);
        glFrontFace(GL_CCW);
        glDisable(GL_STENCIL_TEST);

        gBuffer->BindDepthAttachmentFrom(*gBuffer);
    }
}

