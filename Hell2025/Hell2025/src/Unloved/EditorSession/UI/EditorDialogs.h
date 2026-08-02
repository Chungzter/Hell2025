#pragma once

#include "Unloved/EditorSession/EditorSessionTypes.h"

#include <string>

namespace Unloved::EditorSession::Dialog {

    void Open(const std::string& message);
    void Close();
    void Render();

    bool IsOpen();
}

namespace Unloved::EditorSession::FileDialog {

    void Open(EditorSessionMode mode, const std::string& selectedFileName);
    void New(EditorSessionMode mode);
    void Close();
    void Render();

    bool IsOpen();
    bool IsNewFileOpen();
    std::string ConsumeSelectedFile();
    std::string ConsumeNewFileName();
}
