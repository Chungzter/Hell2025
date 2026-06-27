#include "Util.h"
#include "Unloved/Common/Constants.h"

namespace Util {

    const AnimatedNode* FindAnimatedNode(Animation* animation, const char* NodeName) {
        for (unsigned int i = 0; i < animation->m_animatedNodes.size(); i++) {
            const AnimatedNode* animatedNode = &animation->m_animatedNodes[i];

            if (animatedNode->m_nodeName == NodeName) {
                return animatedNode;
            }
        }
        return nullptr;
    }

    int FindAnimatedNodeIndex(float AnimationTime, const AnimatedNode* animatedNode) {
        // bail if current animation time is earlier than the this nodes first keyframe time
        if (AnimationTime < animatedNode->m_nodeKeys[0].timeStamp)
            return -1; // you WERE returning -1 here

        for (unsigned int i = 1; i < animatedNode->m_nodeKeys.size(); i++) {
            if (AnimationTime < animatedNode->m_nodeKeys[i].timeStamp)
                return i - 1;
        }
        return (int)animatedNode->m_nodeKeys.size() - 1;
    }

    void CalcInterpolatedPosition(glm::vec3& Out, float AnimationTime, const AnimatedNode* animatedNode) {
        int Index = FindAnimatedNodeIndex(AnimationTime, animatedNode);
        int NextIndex = (Index + 1);

        // Is next frame out of range?
        if (NextIndex == animatedNode->m_nodeKeys.size()) {
            Out = animatedNode->m_nodeKeys[Index].positon;
            return;
        }

        // Nothing to report
        if (Index == -1 || animatedNode->m_nodeKeys.size() == 1) {
            Out = animatedNode->m_nodeKeys[0].positon;
            return;
        }
        float DeltaTime = animatedNode->m_nodeKeys[NextIndex].timeStamp - animatedNode->m_nodeKeys[Index].timeStamp;
        float Factor = (AnimationTime - animatedNode->m_nodeKeys[Index].timeStamp) / DeltaTime;

        glm::vec3 start = animatedNode->m_nodeKeys[Index].positon;
        glm::vec3 end = animatedNode->m_nodeKeys[NextIndex].positon;
        glm::vec3 delta = end - start;
        Out = start + Factor * delta;
    }

    void CalcInterpolatedScale(glm::vec3& Out, float AnimationTime, const AnimatedNode* animatedNode) {
        int Index = FindAnimatedNodeIndex(AnimationTime, animatedNode);
        int NextIndex = (Index + 1);

        // Is next frame out of range?
        if (NextIndex == animatedNode->m_nodeKeys.size()) {
            Out = animatedNode->m_nodeKeys[Index].scale;
            return;
        }

        // Nothing to report
        if (Index == -1 || animatedNode->m_nodeKeys.size() == 1) {
            Out = animatedNode->m_nodeKeys[0].scale;
            return;
        }
        float DeltaTime = animatedNode->m_nodeKeys[NextIndex].timeStamp - animatedNode->m_nodeKeys[Index].timeStamp;
        float Factor = (AnimationTime - animatedNode->m_nodeKeys[Index].timeStamp) / DeltaTime;

        glm::vec3 start = animatedNode->m_nodeKeys[Index].scale;
        glm::vec3 end = animatedNode->m_nodeKeys[NextIndex].scale;
        glm::vec3 delta = end - start;
        Out = start + Factor * delta;
    }

    void CalcInterpolatedRotation(glm::quat& Out, float AnimationTime, const AnimatedNode* animatedNode) {
        int Index = FindAnimatedNodeIndex(AnimationTime, animatedNode);
        int NextIndex = (Index + 1);

        // Is next frame out of range?
        if (NextIndex == animatedNode->m_nodeKeys.size()) {
            Out = animatedNode->m_nodeKeys[Index].rotation;
            return;
        }

        // Nothing to report
        if (Index == -1 || animatedNode->m_nodeKeys.size() == 1) {
            Out = animatedNode->m_nodeKeys[0].rotation;
            return;
        }
        float DeltaTime = animatedNode->m_nodeKeys[NextIndex].timeStamp - animatedNode->m_nodeKeys[Index].timeStamp;
        float Factor = (AnimationTime - animatedNode->m_nodeKeys[Index].timeStamp) / DeltaTime;

        const glm::quat& StartRotationQ = animatedNode->m_nodeKeys[Index].rotation;
        const glm::quat& EndRotationQ = animatedNode->m_nodeKeys[NextIndex].rotation;

        Util::InterpolateQuaternion(Out, StartRotationQ, EndRotationQ, Factor);
        Out = glm::normalize(Out);
    }

    glm::mat4 Mat4InitScaleTransform(float ScaleX, float ScaleY, float ScaleZ) {
        return glm::scale(glm::mat4(1.0), glm::vec3(ScaleX, ScaleY, ScaleZ));
    }

    glm::mat4 Mat4InitTranslationTransform(float x, float y, float z) {
        return  glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
    }

    glm::mat4 Mat4InitRotateTransform(float RotateX, float RotateY, float RotateZ) {
        glm::mat4 rx = glm::mat4(1);
        glm::mat4 ry = glm::mat4(1);
        glm::mat4 rz = glm::mat4(1);

        const float x = glm::radians(RotateX);
        const float y = glm::radians(RotateY);
        const float z = glm::radians(RotateZ);

        rx[0][0] = 1.0f; rx[0][1] = 0.0f; rx[0][2] = 0.0f; rx[0][3] = 0.0f;
        rx[1][0] = 0.0f; rx[1][1] = cosf(x); rx[1][2] = -sinf(x); rx[1][3] = 0.0f;
        rx[2][0] = 0.0f; rx[2][1] = sinf(x); rx[2][2] = cosf(x); rx[2][3] = 0.0f;
        rx[3][0] = 0.0f; rx[3][1] = 0.0f; rx[3][2] = 0.0f; rx[3][3] = 1.0f;

        ry[0][0] = cosf(y); ry[0][1] = 0.0f; ry[0][2] = -sinf(y); ry[0][3] = 0.0f;
        ry[1][0] = 0.0f; ry[1][1] = 1.0f; ry[1][2] = 0.0f; ry[1][3] = 0.0f;
        ry[2][0] = sinf(y); ry[2][1] = 0.0f; ry[2][2] = cosf(y); ry[2][3] = 0.0f;
        ry[3][0] = 0.0f; ry[3][1] = 0.0f; ry[3][2] = 0.0f; ry[3][3] = 1.0f;

        rz[0][0] = cosf(z); rz[0][1] = -sinf(z); rz[0][2] = 0.0f; rz[0][3] = 0.0f;
        rz[1][0] = sinf(z); rz[1][1] = cosf(z); rz[1][2] = 0.0f; rz[1][3] = 0.0f;
        rz[2][0] = 0.0f; rz[2][1] = 0.0f; rz[2][2] = 1.0f; rz[2][3] = 0.0f;
        rz[3][0] = 0.0f; rz[3][1] = 0.0f; rz[3][2] = 0.0f; rz[3][3] = 1.0f;

        return rz * ry * rx;
    }

}
