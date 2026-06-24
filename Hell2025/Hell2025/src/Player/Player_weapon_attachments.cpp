#include "Player.h"
#include "Bible/Bible.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderDataManager.h"
#include "Viewport/ViewportManager.h"
#include "Util.h"

#include "Input/Input.h"


void Player::UpdateWeaponAttachments() {
	AnimatedGameObject* viewWeapon = GetViewWeaponAnimatedGameObject();
	SkinnedModel* skinnedModel = viewWeapon->GetSkinnedModel();

	if (!viewWeapon || Util::IsNaN(viewWeapon->GetModelMatrix())) {
		return;
	}

    return;

	if (viewWeapon->GetSkinnedModel()->GetName() == "Glock") {
		{
			glm::mat4 globalBlendedNodeTransform = viewWeapon->GetGlobalBlendedNodeTransfrom("Sight");
			glm::mat4 boneOffset = skinnedModel->GetBoneOffset("Sight");
			glm::mat4 modelMatrix = viewWeapon->GetModelMatrix();

			glm::mat4 finalMatrix = modelMatrix * globalBlendedNodeTransform * boneOffset;
			m_redDot.Update(finalMatrix);
			m_redDot.DrawWorldspaceAABBs(YELLOW);

			//const glm::mat4 modelMatrix = viewWeapon->GetBoneWorldMatrixWithBoneOffset("Sight");
			//m_redDot.Update(modelMatrix);
			RenderDataManager::SubmitRenderItems(m_redDot.GetRenderItems());
		}
		{
			const glm::mat4 modelMatrix = viewWeapon->GetBoneWorldMatrixWithBoneOffset("Suppressor");
            m_supressor.Update(modelMatrix);
            m_supressor.DrawWorldspaceAABBs(YELLOW);

            RenderDataManager::SubmitRenderItems(m_supressor.GetRenderItems());
		}
	}


    return;



    if (viewWeapon->GetSkinnedModel()->GetName() == "P90") {
        Transform offset;
        offset.position = glm::vec3(0.0f, 0.0f, -1.000003f);
        glm::mat4 offsetMatrix = offset.to_mat4();

        glm::mat4 globalBlendedNodeTransform = viewWeapon->GetGlobalBlendedNodeTransfrom("Magazine2");
        glm::mat4 boneOffset = skinnedModel->GetBoneOffset("Magazine2");
        glm::mat4 modelMatrix = viewWeapon->GetModelMatrix();
        glm::mat4 finalMatrix = modelMatrix * globalBlendedNodeTransform * boneOffset * offsetMatrix;
        //glm::mat4 finalMatrix = modelMatrix * globalBlendedNodeTransform * boneOffset;

        //DebugDraw::DrawPoint((modelMatrix * globalBlendedNodeTransform)[3], BLUE);
        //DebugDraw::DrawPoint((modelMatrix * globalBlendedNodeTransform * boneOffset)[3], WHITE);
        //DebugDraw::DrawPoint((modelMatrix * globalBlendedNodeTransform * boneOffset)[3] * offsetMatrix, ORANGE);

        m_p90MagMeshNodes.Update(finalMatrix);
        RenderDataManager::SubmitRenderItems(m_p90MagMeshNodes.GetRenderItems());



        //m_p90MagTest.DrawWorldspaceAABBs(GREEN);
    }
}