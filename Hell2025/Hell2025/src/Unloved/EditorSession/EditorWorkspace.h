#pragma once

#include "EditorSessionTypes.h"

namespace Unloved::EditorSession::Workspace {

    bool Open(EditorSessionMode mode);
    void Close();
    void Save();
    void Discard();

    bool HasMode();
    EditorSessionMode GetMode();
}
