#include "Physics.h"
#include "Core/GameOLD.h"

namespace Physics {

    std::vector<PxRigidActor*> GetIgnoreList(RaycastIgnoreFlags flags) {
        std::vector<PxRigidActor*> ignoreList;
        ignoreList.reserve(8);

        int count = GameOLD::GetLocalPlayerCount();
        for (int i = 0; i < count; ++i) {
            if (auto* player = GameOLD::GetLocalPlayerByIndex(i)) {

                // Player character controllers
                if ((flags & RaycastIgnoreFlags::PLAYER_CHARACTER_CONTROLLERS) != RaycastIgnoreFlags::NONE) {
                    ignoreList.push_back(player->GetCharacterControllerActor());
                }

                // Player ragdolls
                if ((flags & RaycastIgnoreFlags::PLAYER_RAGDOLLS) != RaycastIgnoreFlags::NONE) {
                    auto ragdollActors = GetRagdollPxRigidActors(player->GetRagdollId());
                    ignoreList.insert(ignoreList.end(), ragdollActors.begin(), ragdollActors.end());
                }
            }
        }

        return ignoreList;
    }
}