#pragma once

namespace Unloved::GameAudio {
    void BeginFrame();
    void Update();

    void PlayGlockEquipAudio();
    void PlayGlassHitAudio();
    void PlayFootstepIndoorAudio();
    void PlayFootstepOutdoorAudio();
    void TryPlayFleshImpactAudio();
}
