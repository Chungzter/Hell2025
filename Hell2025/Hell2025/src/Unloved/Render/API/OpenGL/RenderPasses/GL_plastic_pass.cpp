#include "../GL_renderer.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Hell/Backend/BackEnd.h"
#include "Viewport/ViewportManager.h"
#include "Editor/Editor.h"
#include "Renderer/RenderDataManager.h"
#include "Modelling/Clipping.h"
#include "Modelling/Unused/Modelling.h"
#include "World/LegacyWorld.h"

#include "Hell/Logging.h"
#include "Hell/Physics/Physics.h"

#include "Types/Mirror.h"
#include "Managers/MirrorManager.h"

#include "Core/GameOLD.h"\

// get me out of here
#include "Hell/ResourceManagement/ResourceManager.h"
// get me out of here

namespace OpenGLRenderer {


	void PlasticPass() {
		ProfilerOpenGLZoneFunction();

		const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();
		const std::vector<RenderItem>& renderItems = RenderDataManager::GetRenderItemsPlastic();

        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLFrameBuffer* miscFullSizeFbo = OpenGL::ResourceManager::GetFrameBufferPtr("MiscFullSize");
		OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("Plastic");
        OpenGLShadowCubeMapArray* hiResShadowMaps = OpenGL::ResourceManager::GetShadowCubeMapArrayPtr("HiRes");

        if (!gBuffer) return;
        if (!shader) return;
        if (!hiResShadowMaps) return;

		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		OpenGL::BlitFrameBuffer(gBuffer, miscFullSizeFbo, "Lighting", "FinalLightingCopy", GL_COLOR_BUFFER_BIT, GL_NEAREST);

		gBuffer->Bind();
		gBuffer->DrawBuffers({ "Lighting" });

		OpenGL::BindShader("Plastic");
        OpenGL::SetUniformInt("u_tileXCount", GetTileCountX());

        OpenGL::BindSSBO(5, "TileLights");

		OpenGLRasterizerStateManager::ForceRasterizerState("GeometryPass_Default");

		glBindVertexArray(Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());

		// Fill the death butter
		glEnable(GL_DEPTH_TEST);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

		for (int i = 0; i < 4; i++) {
			Viewport* viewport = ViewportManager::GetViewportByIndex(i);
			if (!viewport->IsVisible()) continue;

			OpenGLRenderer::SetViewport(gBuffer, viewport);

			OpenGL::BindShader("Plastic");
			OpenGL::SetUniformMat4("u_projectionView", viewportData[i].projectionViewReverseZ);
			OpenGL::SetUniformMat4("u_view", viewportData[i].view);

			glDepthFunc(GL_GREATER);

			for (const RenderItem& renderItem : renderItems) {
				Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(renderItem.meshId);
				if (!mesh) continue;

				OpenGL::SetUniformMat4("u_model", renderItem.modelMatrix);
				glDrawElementsBaseVertex(GL_TRIANGLES,  mesh->indexCount, GL_UNSIGNED_INT, (GLvoid*)(mesh->baseIndex * sizeof(GLuint)), mesh->baseVertex);
			}
		}

		// Bind plastic material
		// It'd be great if you didn't have to blend in these hacky plastic material properties
		// and could derive the result you want directly from the source material
		Material* material = Hell::ResourceManager::GetMaterialByName("Plastic");
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(material->m_basecolor)->GetGLTexture().GetHandle());
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(material->m_normal)->GetGLTexture().GetHandle());
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(material->m_rma)->GetGLTexture().GetHandle());

		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, miscFullSizeFbo->GetColorAttachmentHandleByName("FinalLightingCopy"));
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

			OpenGL::BindShader("Plastic");
            OpenGL::SetUniformMat4("u_projectionView", viewportData[i].projectionViewReverseZ);
            OpenGL::SetUniformMat4("u_view", viewportData[i].view);
            OpenGL::SetUniformVec3("u_viewPos", viewportData[i].viewPos);

			glDepthFunc(GL_EQUAL);

			for (const RenderItem& renderItem : renderItems) {
				Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(renderItem.meshId);
				if (!mesh) continue;

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.baseColorTextureIndex)->GetGLTexture().GetHandle());
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.normalMapTextureIndex)->GetGLTexture().GetHandle());
				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItem.rmaTextureIndex)->GetGLTexture().GetHandle());

				OpenGL::SetUniformMat4("u_model", renderItem.modelMatrix);
				OpenGL::SetUniformMat4("u_inverseModel", renderItem.inverseModelMatrix);

				glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (GLvoid*)(mesh->baseIndex * sizeof(GLuint)), mesh->baseVertex);
			}
		}


		// Clean up
		glDepthFunc(GL_GREATER);
	}


	void PlasticPass2() {

		ProfilerOpenGLZoneFunction();

		const DrawCommandsSet& drawInfoSet = RenderDataManager::GetDrawInfoSet();
		const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();

		OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
		OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("Plastic");

		if (!gBuffer) return;
		if (!shader) return;

		glBindVertexArray(Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());

		gBuffer->Bind();
		gBuffer->DrawBuffers({ "Lighting" });

		OpenGL::BindShader("Plastic");

		OpenGLRasterizerStateManager::ForceRasterizerState("GeometryPass_Default");
		EditorRasterizerStateOverride();

		glDisable(GL_DEPTH_TEST);

		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); // THIS IS IMPORTANT. Some other pass disabled this. Classic OpenGL state machine.

		// PLASTIC TEMPORARYILY RENDERER HERE FOR TESTING
		for (int i = 0; i < 4; i++) {
			Viewport* viewport = ViewportManager::GetViewportByIndex(i);
			if (viewport->IsVisible()) {
				OpenGLRenderer::SetViewport(gBuffer, viewport);
				if (Hell::BackEnd::RenderDocFound()) {
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

