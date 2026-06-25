#pragma once

namespace Hell::Time {
    void Init();
    void Update();

    float RawDeltaTime();
    float DeltaTime();
    float MaxDeltaTime();
    float FixedDeltaTime();
    float FixedAlpha();
    float TotalTime();

    int FixedStepsConsumedThisFrame();
    int MaxFixedStepsPerFrame();
    bool ConsumeFixedStep();
}
