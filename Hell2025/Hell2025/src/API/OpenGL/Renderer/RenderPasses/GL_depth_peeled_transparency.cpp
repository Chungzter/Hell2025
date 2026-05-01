#include "API/OpenGL/Renderer/GL_renderer.h"
#include "API/OpenGL/GL_backend.h"
#include "AssetManagement/AssetManager.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderDataManager.h"
#include "Viewport/ViewportManager.h"

#include <Hell/Logging.h>

// remove me
#include "Core/Game.h"
#include "Input/Input.h"
#include "Physics/Physics.h"
#include "World/World.h"
#include "Util/Util.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <algorithm>
#include <execution>
// remove me


/*
"DepthPeeledTransparency"] = OpenGLFrameBuffer("DepthPeeledTransparency", resolutions.gBuffer);
"DepthPeeledTransparency"].CreateAttachment("Color", GL_RGBA8);
"DepthPeeledTransparency"].CreateAttachment("ViewspaceDepthPrevious", GL_R32F);
"DepthPeeledTransparency"].CreateAttachment("Composite", GL_R32F);
"DepthPeeledTransparency"].CreateAttachment("Composite", GL_RGBA8);
"DecalPainting"].CreateDepthAttachment(GL_DEPTH32F_STENCIL8);
*/

namespace OpenGLRenderer {

	void P90MagColor();
	void P90MagComposite();


	void DepthPeeledTransparencyPass() {
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		P90MagColor();
	}

	void P90MagColor() {
		ProfilerOpenGLZoneFunction();

		const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();
		const std::vector<RenderItem>& renderItems = RenderDataManager::GetNonDeformingSkinnedMeshRenderItemsDepthPeeledTransparent();

		OpenGLFrameBuffer* gBuffer = GetFrameBuffer("GBuffer");
		OpenGLFrameBuffer* depthPeeledTransparencyFbo = GetFrameBuffer("DepthPeeledTransparency");
		OpenGLFrameBuffer* miscFullSizeFbo = GetFrameBuffer("MiscFullSize"); // Has gbuffer viewspace depth in here
		OpenGLShader* depthPeelDepthShader = GetShader("DepthPeeledTransparencyDepth");
		OpenGLShader* depthPeelColorShader = GetShader("DepthPeeledTransparencyColor");

		if (!gBuffer) return;
		if (!depthPeeledTransparencyFbo) return;
		if (!miscFullSizeFbo) return;
		if (!depthPeelDepthShader) return;
		if (!depthPeelColorShader) return;

		// Begin by copying the gbuffer viewspace depth into the depth peeling fbo.
		BlitFrameBuffer(miscFullSizeFbo, depthPeeledTransparencyFbo, "ViewspaceDepth", "ViewspaceDepthPrevious", GL_COLOR_BUFFER_BIT, GL_NEAREST);

		depthPeeledTransparencyFbo->Bind();
		depthPeeledTransparencyFbo->ClearAttachment("Composite", 0.0f, 0.0f, 0.0f, 0.0f);
		depthPeeledTransparencyFbo->DrawBuffers({ "Composite", });


		ForceRasterizerState("GeometryPass_Default");

		glBindVertexArray(OpenGLBackEnd::GetWeightedVertexDataVAO());
		glBindBuffer(GL_ARRAY_BUFFER, OpenGLBackEnd::GetWeightedVertexDataVBO());
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGLBackEnd::GetWeightedVertexDataEBO());

		for (int i = 0; i < 4; i++) {
			Viewport* viewport = ViewportManager::GetViewportByIndex(i);
			if (!viewport->IsVisible()) continue;

			OpenGLRenderer::SetViewport(gBuffer, viewport);

			depthPeelDepthShader->Bind();
			depthPeelDepthShader->SetMat4("u_projectionView", viewportData[i].projectionView);
			depthPeelDepthShader->SetMat4("u_view", viewportData[i].view);

			depthPeelColorShader->Bind();
			depthPeelColorShader->SetMat4("u_projectionView", viewportData[i].projectionView);
			depthPeelColorShader->SetMat4("u_view", viewportData[i].view);

			// PEEL
			static int peelCount = 1;
			int maxPeelCount = 10;

			//glDisable(GL_CULL_FACE);

			if (Input::KeyPressed(HELL_KEY_LEFT)) {
				peelCount--;
				peelCount = std::clamp(peelCount, 1, maxPeelCount);
				std::cout << "peelCount: " << peelCount << "\n";
			}
			if (Input::KeyPressed(HELL_KEY_RIGHT)) {
				peelCount++;
				peelCount = std::clamp(peelCount, 1, maxPeelCount);
				std::cout << "peelCount: " << peelCount << "\n";
			}

			for (int j = 0; j < peelCount; j++) {

				// Fill the depth buffer of this peel layer
				{
					BlitFrameBufferDepth(gBuffer, depthPeeledTransparencyFbo);

					depthPeeledTransparencyFbo->Bind();
					depthPeeledTransparencyFbo->ClearAttachment("ViewspaceDepth", 0.0f);
					depthPeeledTransparencyFbo->DrawBuffers({ "ViewspaceDepth" });

					depthPeelDepthShader->Bind();
					BindImageTexture(0, depthPeeledTransparencyFbo->GetColorAttachmentHandleByName("ViewspaceDepthPrevious"), GL_READ_ONLY, GL_R32F);

					glDepthFunc(GL_LESS);

					for (const RenderItem& renderItem : renderItems) {
						SkinnedMesh* mesh = AssetManager::GetSkinnedMeshByIndex(renderItem.meshIndex);
						if (!mesh) continue;

						depthPeelDepthShader->SetMat4("u_model", renderItem.modelMatrix);
						glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (GLvoid*)(mesh->baseIndex * sizeof(GLuint)), mesh->baseVertexGlobal);

					}
				}
				// Re-render GL_EQUAL against the depth buffer
				{
					glDepthFunc(GL_EQUAL);


					depthPeeledTransparencyFbo->Bind();
					depthPeeledTransparencyFbo->ClearAttachment("Color", 0.0f);
					depthPeeledTransparencyFbo->DrawBuffers({ "Color", "ViewspaceDepthPrevious"});


					depthPeelColorShader->Bind();
					BindImageTexture(4, depthPeeledTransparencyFbo->GetColorAttachmentHandleByName("ViewspaceDepth"), GL_READ_ONLY, GL_R32F);

					Material* material = AssetManager::GetMaterialByName("Plastic");
					glActiveTexture(GL_TEXTURE3);
					glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(material->m_basecolor)->GetGLTexture().GetHandle());
					glActiveTexture(GL_TEXTURE4);
					glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(material->m_normal)->GetGLTexture().GetHandle());
					glActiveTexture(GL_TEXTURE5);
					glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(material->m_rma)->GetGLTexture().GetHandle());


					OpenGLFrameBuffer* gBuffer = GetFrameBuffer("GBuffer");
					glActiveTexture(GL_TEXTURE6);
					glBindTexture(GL_TEXTURE_2D, gBuffer->GetColorAttachmentHandleByName("FinalLighting"));
					glActiveTexture(GL_TEXTURE7);
					glBindTexture(GL_TEXTURE_2D, gBuffer->GetDepthAttachmentHandle());

					for (const RenderItem& renderItem : renderItems) {
						SkinnedMesh* mesh = AssetManager::GetSkinnedMeshByIndex(renderItem.meshIndex);
						if (!mesh) continue;

						glActiveTexture(GL_TEXTURE0);
						glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(renderItem.baseColorTextureIndex)->GetGLTexture().GetHandle());
						glActiveTexture(GL_TEXTURE1);
						glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(renderItem.normalMapTextureIndex)->GetGLTexture().GetHandle());
						glActiveTexture(GL_TEXTURE2);
						glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(renderItem.rmaTextureIndex)->GetGLTexture().GetHandle());

						depthPeelColorShader->SetMat4("u_model", renderItem.modelMatrix);
						depthPeelColorShader->SetMat4("u_inverseModel", renderItem.inverseModelMatrix);
						glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (GLvoid*)(mesh->baseIndex * sizeof(GLuint)), mesh->baseVertexGlobal);

					}
				}

				P90MagComposite();
			}
		}
	}

	void
		P90MagComposite() {

		OpenGLFrameBuffer* depthPeeledTransparencyFbo = GetFrameBuffer("DepthPeeledTransparency");
		OpenGLFrameBuffer* gBuffer = GetFrameBuffer("GBuffer");
		OpenGLShader* shader = GetShader("DepthPeeledTransparencyComposite");

		if (!depthPeeledTransparencyFbo) return;
		if (!gBuffer) return;
		if (!shader) return;

		shader->Bind();
		BindImageTexture(0, gBuffer->GetColorAttachmentHandleByName("FinalLighting"), GL_READ_WRITE, GL_RGBA16F);
		BindImageTexture(1 , depthPeeledTransparencyFbo->GetColorAttachmentHandleByName("Color"), GL_READ_ONLY, GL_RGBA16F);

		int width = gBuffer->GetWidth();
		int height = gBuffer->GetHeight();

		glDispatchCompute((width + 15) / 16, (height + 15) / 16, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}
}

