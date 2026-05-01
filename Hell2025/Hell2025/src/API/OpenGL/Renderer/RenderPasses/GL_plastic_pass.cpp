#include "../GL_renderer.h"
#include "../../GL_backend.h"
#include "AssetManagement/AssetManager.h"
#include "BackEnd/Backend.h"
#include "Viewport/ViewportManager.h"
#include "Editor/Editor.h"
#include "Renderer/RenderDataManager.h"
#include "Modelling/Clipping.h"
#include "Modelling/Unused/Modelling.h"
#include "World/World.h"

#include "Ragdoll/RagdollManager.h"
#include "Input/Input.h"
#include <Hell/Logging.h>
#include "Physics/Physics.h"

#include "Types/Mirror.h"
#include "Managers/MirrorManager.h"

#include "Core/Game.h"

// get me out of here
#include "AssetManagement/AssetManager.h"
// get me out of here

namespace OpenGLRenderer {


	void PlasticPass() {
		ProfilerOpenGLZoneFunction();

		const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();
		const std::vector<RenderItem>& renderItems = RenderDataManager::GetPlasticRenderItems();

		OpenGLFrameBuffer* gBuffer = GetFrameBuffer("GBuffer");
		OpenGLShader* shader = GetShader("Plastic");
        OpenGLShadowCubeMapArray* hiResShadowMaps = GetShadowCubeMapArray("HiRes");

        if (!gBuffer) return;
        if (!shader) return;
        if (!hiResShadowMaps) return;

		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		BlitFrameBuffer(gBuffer, gBuffer, "FinalLighting", "FinalLightingCopy", GL_COLOR_BUFFER_BIT, GL_NEAREST);

		gBuffer->Bind();
		gBuffer->DrawBuffers({ "FinalLighting" });

		shader->Bind();
        shader->SetInt("u_tileXCount", GetTileCountX());

        BindSSBO("TileLights", 5);

		ForceRasterizerState("GeometryPass_Default");

		glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());

		// Fill the death butter
		glEnable(GL_DEPTH_TEST);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

		for (int i = 0; i < 4; i++) {
			Viewport* viewport = ViewportManager::GetViewportByIndex(i);
			if (!viewport->IsVisible()) continue;

			OpenGLRenderer::SetViewport(gBuffer, viewport);

			shader->Bind();
			shader->SetMat4("u_projectionView", viewportData[i].projectionView);
			shader->SetMat4("u_view", viewportData[i].view);

			glDepthFunc(GL_LESS);

			for (const RenderItem& renderItem : renderItems) {
				Mesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
				if (!mesh) continue;

				shader->SetMat4("u_model", renderItem.modelMatrix);
				glDrawElementsBaseVertex(GL_TRIANGLES,  mesh->indexCount, GL_UNSIGNED_INT, (GLvoid*)(mesh->baseIndex * sizeof(GLuint)), mesh->baseVertex);
			}
		}

		// Bind plastic material
		// It'd be great if you didn't have to blend in these hacky plastic material properties
		// and could derive the result you want directly from the source material
		Material* material = AssetManager::GetMaterialByName("Plastic");
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(material->m_basecolor)->GetGLTexture().GetHandle());
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(material->m_normal)->GetGLTexture().GetHandle());
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(material->m_rma)->GetGLTexture().GetHandle());

		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, gBuffer->GetColorAttachmentHandleByName("FinalLightingCopy"));
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, gBuffer->GetDepthAttachmentHandle());

        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, hiResShadowMaps->GetDepthTexture());

		// Now render color
		glEnable(GL_DEPTH_TEST);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		for (int i = 0; i < 4; i++) {
			Viewport* viewport = ViewportManager::GetViewportByIndex(i);
			if (!viewport->IsVisible()) continue;

			OpenGLRenderer::SetViewport(gBuffer, viewport);

			shader->Bind();
            shader->SetMat4("u_projectionView", viewportData[i].projectionView);
            shader->SetMat4("u_view", viewportData[i].view);
            shader->SetVec3("u_viewPos", viewportData[i].viewPos);

			glDepthFunc(GL_EQUAL);

			for (const RenderItem& renderItem : renderItems) {
				Mesh* mesh = AssetManager::GetMeshByIndex(renderItem.meshIndex);
				if (!mesh) continue;

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(renderItem.baseColorTextureIndex)->GetGLTexture().GetHandle());
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(renderItem.normalMapTextureIndex)->GetGLTexture().GetHandle());
				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(renderItem.rmaTextureIndex)->GetGLTexture().GetHandle());

				shader->SetMat4("u_model", renderItem.modelMatrix);
				shader->SetMat4("u_inverseModel", renderItem.inverseModelMatrix);

				glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (GLvoid*)(mesh->baseIndex * sizeof(GLuint)), mesh->baseVertex);
			}
		}




		// Clean up
		glDepthFunc(GL_LESS);
	}





	void PlasticPass2() {

		ProfilerOpenGLZoneFunction();

		const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();
		const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();

		OpenGLFrameBuffer* gBuffer = GetFrameBuffer("GBuffer");
		OpenGLShader* shader = GetShader("Plastic");

		if (!gBuffer) return;
		if (!shader) return;

		glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());

		gBuffer->Bind();
		gBuffer->DrawBuffers({ "FinalLighting" });

		shader->Bind();

		ForceRasterizerState("GeometryPass_Default");
		EditorRasterizerStateOverride();

		glDisable(GL_DEPTH_TEST);

		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); // THIS IS IMPORTANT. Some other pass disabled this. Classic OpenGL state machine.

		// PLASTIC TEMPORARYILY RENDERER HERE FOR TESTING
		for (int i = 0; i < 4; i++) {
			Viewport* viewport = ViewportManager::GetViewportByIndex(i);
			if (viewport->IsVisible()) {
				OpenGLRenderer::SetViewport(gBuffer, viewport);
				if (BackEnd::RenderDocFound()) {
					SplitMultiDrawIndirect(shader, drawInfoSet.plastic[i], true, false);
				}
				else {
					MultiDrawIndirect(drawInfoSet.plastic[i]);
				}
			}
		}

		glBindVertexArray(0);

	}
}

