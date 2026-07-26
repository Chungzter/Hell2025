#pragma once

#include "EditorSessionTypes.h"

namespace Unloved::EditorSession {

    void Init();
    void Open();
    void Open(EditorSessionMode mode);
    void Close();
    void SetActive(bool active);
    void Update();
    void UpdateViewportInput();
    void Render();

    bool IsActive();
    bool IsInactive();
    bool HasMode();
    EditorSessionMode GetMode();
    bool WantsMouseCapture();
    bool WantsKeyboardCapture();
}
