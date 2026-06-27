#pragma once

#include <glm/vec3.hpp>

namespace Unloved {

    struct GPULight {
        float posX;
        float posY;
        float posZ;
        float colorR;

        float colorG;
        float colorB;
        float strength;
        float radius;

        int lightIndex;
        int shadowMapDirty = 1; // true or false
        int useIes = 0;         // true or false
        int iesIndex;

        float iesVScale;
        float iesVBias;
        float iesHScale;
        float iesHBias;

        glm::vec3 forward;
        float iesMaxIntensity;

        glm::vec3 right;
        float iesExposure;

        glm::vec3 up;
        int padding0;

        int iesTextureIndex;
        int isDirtyForRaytracing = 0; // true or false
        int padding1;
        int padding2;
    };
}