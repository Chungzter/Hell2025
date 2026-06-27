#include "Editor.h"

#include "Hell/Common/Enum.h"
#include "Hell/UI/UIBackEnd.h"

#include "Unloved/Viewport/ViewportManager.h"

#include "Unloved/Config/Config.h"
#include "Unloved/Debug/Debug.h"
#include "Unloved/Editor/Gizmo.h"

namespace Unloved::Editor {

    void UpdateDebug() {
        if (!IsOpen()) return;

        // you commented this out when you added debug text modes, maybe you want htis in the future

        /*

        for (int i = 0; i < 4; i++) {
            const Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) return;

            //const Camera* camera = Editor::GetCameraByIndex(i);
            const Resolutions& resolutions = Config::GetResolutions();
            int width = resolutions.ui.x * viewport->GetSize().x;
            int height = resolutions.ui.y * viewport->GetSize().y;
            int xLeft = resolutions.ui.x * viewport->GetPosition().x;
            int yTop = resolutions.ui.y * (1.0f - viewport->GetPosition().y - viewport->GetSize().y);

            if (!Debug::IsDebugTextVisible() && viewport->GetSize().x > 0.15f && viewport->GetSize().y > 0.2f) {
                std::string text = "";
                //text += "ProjectionMatrix: \n" + Hell::String::FormatMat4(viewport->GetProjectionMatrix()) + "\n\n";
                //text += "ViewMatrix: \n" + Hell::String::FormatMat4(camera->GetViewMatrix()) + "\n";
                //text += "ViewportMode: " + Hell::Enum::ToString(viewport->GetViewportMode()) + "\n";
                text += Hell::Enum::ToString(Editor::GetCameraViewByIndex(i)) + "\n";
                text += "\n";
            
               // text += "IsActive: " + Hell::String::FormatBool(i == Editor::GetActiveViewportIndex()) + "\n";


               // text += "Gizmo Position: " + Hell::String::FormatVec3(Gizmo::GetPosition()) + "\n";

               // SpaceCoords windowSpaceCoords = viewport->GetWindowSpaceCoords();
               // SpaceCoords gBufferSpaceCoords = viewport->GetGBufferSpaceCoords();
               // 
               // text += "\n";
               // text += "WINDOW SPACE\n";
               // text += " width: " + std::to_string(windowSpaceCoords.width) + "\n";
               // text += " height: " + std::to_string(windowSpaceCoords.height) + "\n";
               // text += " localMouseX: " + std::to_string(windowSpaceCoords.localMouseX) + "\n";
               // text += " localMouseY: " + std::to_string(windowSpaceCoords.localMouseY) + "\n";
               // 
               // text += "\n";
               // text += "GBUFFER SPACE\n";
               // text += " width: " + std::to_string(gBufferSpaceCoords.width) + "\n";
               // text += " height: " + std::to_string(gBufferSpaceCoords.height) + "\n";
               // text += " localMouseX: " + std::to_string(gBufferSpaceCoords.localMouseX) + "\n";
               // text += " localMouseY: " + std::to_string(gBufferSpaceCoords.localMouseY) + "\n";

                UIBackEnd::BlitText(text, "StandardFont", xLeft + 2, yTop + 2, Alignment::TOP_LEFT, 2.0f);
            }
        }*/
    }
}
