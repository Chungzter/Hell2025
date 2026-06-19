#include "P90MagManager.h"
#include "Core/Game.h"
#include "Renderer/Renderer.h"

#include <Hell/Glm.h>

namespace P90MagManager {
    
    void SubmitMagForRender(const glm::mat4& worldTransform, uint32_t ammoInMag) {
        //DebugDraw::DrawPoint(worldTransform[3], RED);
    }
    
    void SubmitRenderItems() {
        for (int i = 0; i < Game::GetLocalPlayerCount(); i++) {
            Player* player = Game::GetLocalPlayerByIndex(i);
            if (!player) continue;

            if (player->GetSelectedWeaponName() == "P90") {

                //glm::mat4 test = player-> 

                Transform transform;
                transform.position = player->GetCameraPosition();

                SubmitMagForRender(transform.to_mat4(), 50);
            }
        }
        // check every player, see if they have a p90
        // if so submit a mag for rendering also

        // now, also check every "pickup"
        // if it's a p90, submit a mag for rendering also
    }
}