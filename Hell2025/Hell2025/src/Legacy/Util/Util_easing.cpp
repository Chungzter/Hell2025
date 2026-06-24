#include "Util.h"

#include <iostream> // TODO clean up logging
namespace Util {

    float EaseIn(float t, float a) {
        return pow(t, a);
    }

    float EaseOut(float t, float a) {
        return 1.0f - pow(1.0f - t, a);
    }

    float EaseInOut(float t, float a) {
        if (t < 0.5f)
            return 0.5f * pow(t * 2.0f, a);
        else
            return 1.0f - 0.5f * pow((1.0f - t) * 2.0f, a);
    }

    float HermiteEaseInOut(float t) {
        return t * t * (3.0f - 2.0f * t);
    }

    void Usage() {
        static float currentTime = 0;
        float deltaTime = 1.0f / 60.0f;
        currentTime += deltaTime;

        float duration = 1.0f;

        glm::vec3 startPos = glm::vec3(1, 0, 1);
        glm::vec3 endPos = glm::vec3(1, 0, 3);
        float t = currentTime / duration;   // normalized time [0..1]
        float easedT = EaseOut(t, 3.0f);
        easedT = glm::clamp(easedT, 0.0f, 1.0f);

        glm::vec3 pos = glm::mix(startPos, endPos, easedT);
        std::cout << easedT << " " << pos << "\n";
        
        //DebugDraw::DrawPoint(pos, RED);
        //
        //if (Input::KeyPressed(HELL_KEY_SPACE)) {
        //    currentTime = 0;
        //}
    }
}