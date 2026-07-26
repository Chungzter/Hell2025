#include "EditorCamera.h"

#include <algorithm>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Unloved::EditorSession {

    EditorCamera::EditorCamera() {
        LookAt(glm::vec3(0.0f), glm::normalize(glm::vec3(-1.0f, -0.75f, -1.0f)), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    void EditorCamera::LookAt(const glm::vec3& pivot, const glm::vec3& forward, const glm::vec3& up) {
        const glm::vec3 normalizedForward = glm::normalize(forward);
        const glm::vec3 normalizedRight = glm::normalize(glm::cross(normalizedForward, up));
        const glm::vec3 normalizedUp = glm::normalize(glm::cross(normalizedRight, normalizedForward));

        glm::mat3 rotation(1.0f);
        rotation[0] = normalizedRight;
        rotation[1] = normalizedUp;
        rotation[2] = -normalizedForward;

        m_pivot = pivot;
        m_orientation = glm::normalize(glm::quat_cast(rotation));
        UpdateViewMatrix();
    }

    void EditorCamera::SetPivot(const glm::vec3& pivot) {
        m_pivot = pivot;
        UpdateViewMatrix();
    }

    void EditorCamera::SetOrientation(const glm::quat& orientation) {
        m_orientation = glm::normalize(orientation);
        UpdateViewMatrix();
    }

    void EditorCamera::SetDistance(float distance) {
        m_distance = std::max(distance, 0.01f);
        UpdateViewMatrix();
    }

    void EditorCamera::UpdateViewMatrix() {
        m_right = glm::normalize(m_orientation * glm::vec3(1.0f, 0.0f, 0.0f));
        m_up = glm::normalize(m_orientation * glm::vec3(0.0f, 1.0f, 0.0f));
        m_forward = glm::normalize(m_orientation * glm::vec3(0.0f, 0.0f, -1.0f));
        m_position = m_pivot - m_forward * m_distance;
        m_viewMatrix = glm::lookAt(m_position, m_pivot, m_up);
    }

    const glm::mat4& EditorCamera::GetViewMatrix() const {
        return m_viewMatrix;
    }

    const glm::vec3& EditorCamera::GetPivot() const {
        return m_pivot;
    }

    const glm::vec3& EditorCamera::GetPosition() const {
        return m_position;
    }

    const glm::vec3& EditorCamera::GetForward() const {
        return m_forward;
    }

    const glm::vec3& EditorCamera::GetUp() const {
        return m_up;
    }

    const glm::vec3& EditorCamera::GetRight() const {
        return m_right;
    }

    const glm::quat& EditorCamera::GetOrientation() const {
        return m_orientation;
    }

    float EditorCamera::GetDistance() const {
        return m_distance;
    }
}
