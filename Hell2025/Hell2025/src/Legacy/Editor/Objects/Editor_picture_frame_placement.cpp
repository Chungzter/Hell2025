#include "Editor/Editor.h"
#include "Hell/Audio.h"
namespace Audio = Hell::Audio;
#include "Renderer/Renderer.h"
#include "World/LegacyWorld.h"
#include "Viewport/ViewportManager.h"
#include "Util.h"
#include "Hell/Input.h"
namespace Input = Hell::Input;


namespace Editor {
    void UpdatePictureFramePlacement() {
        if (Input::LeftMousePressed()) {
            // Bail if there is no hovered viewport
            Viewport* viewport = ViewportManager::GetViewportByIndex(GetHoveredViewportIndex());
            if (!viewport) return;

            // Cast physx ray
            float maxRayDistance = 2000;
            glm::vec3 rayOrigin = GetMouseRayOriginByViewportIndex(GetHoveredViewportIndex());
            glm::vec3 rayDir = GetMouseRayDirectionByViewportIndex(GetHoveredViewportIndex());

            PhysXRayResult rayResult = Physics::CastPhysXRay(rayOrigin, rayDir, maxRayDistance, true);
            ObjectType objectType = UniqueID::GetType(rayResult.userData.objectId);

            // Place picture frame
            if (objectType == ObjectType::WALL_SEGMENT) {
                Audio::PlayAudio(AUDIO_SELECT, 1.0f);
                PictureFrameCreateInfo createInfo;
                createInfo.position = rayResult.hitPosition;
                createInfo.rotation = Util::EulerRotationFromNormal(rayResult.hitNormal);
                LegacyWorld::AddPictureFrame(createInfo);
                ExitObjectPlacement();
            }
        }

        // Exit placement
        if (Input::RightMouseDown()) {
            Audio::PlayAudio(AUDIO_SELECT, 1.0f);
            ExitObjectPlacement();
        }
    }
}