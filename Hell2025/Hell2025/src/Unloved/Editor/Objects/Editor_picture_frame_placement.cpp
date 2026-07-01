#include "Hell/Audio.h"
#include "Hell/Input.h"
#include "Hell/Math/Rotation.h"

#include "Legacy/Renderer/Renderer.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Legacy/World/LegacyWorld.h"

#include "Unloved/Editor/Editor.h"
#include "Unloved/ObjectId.h"
#include "Unloved/World/World.h"

namespace Audio = Hell::Audio;
namespace Input = Hell::Input;

namespace Unloved::Editor {

    void UpdatePictureFramePlacement() {
        if (Input::LeftMousePressed()) {
            // Bail if there is no hovered viewport
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(GetHoveredViewportIndex());
            if (!viewport) return;

            // Cast physx ray
            float maxRayDistance = 2000;
            glm::vec3 rayOrigin = GetMouseRayOriginByViewportIndex(GetHoveredViewportIndex());
            glm::vec3 rayDir = GetMouseRayDirectionByViewportIndex(GetHoveredViewportIndex());

            PhysXRayResult rayResult = Hell::Physics::CastPhysXRay(rayOrigin, rayDir, maxRayDistance, true);
            ObjectType objectType = Unloved::GetObjectIdType(rayResult.userData.objectId);

            // Place picture frame
            if (objectType == ObjectType::WALL_SEGMENT) {
                Audio::PlayAudio(AUDIO_SELECT, 1.0f);
                PictureFrameCreateInfo createInfo;
                createInfo.position = rayResult.hitPosition;
                createInfo.rotation = Hell::Math::EulerRotationFromNormal(rayResult.hitNormal);
                Unloved::World::AddPictureFrame(createInfo);
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
