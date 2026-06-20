#include "camera.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Safemon {

    Camera::Camera()
    {
        m_right = glm::normalize(glm::cross(m_forward, m_up));
    }

    glm::vec3 Camera::Rotate(
        const glm::vec3& axis,
        float            angle,
        const glm::vec3& v) const
    {
        glm::quat q = glm::angleAxis(angle, glm::normalize(axis));
        return glm::rotate(q, v);
    }

    void Camera::LookAt(const glm::vec3& pos, const glm::vec3& target)
    {
        m_pos = pos;
        m_forward = glm::normalize(target - pos);
        m_up = glm::vec3(0.0f, 1.0f, 0.0f);
        m_right = glm::normalize(glm::cross(m_forward, m_up));
        m_up = glm::cross(m_right, m_forward);
    }

    void Camera::SetPreset(int preset, const glm::vec3& target)
    {
        glm::vec3 to_target = glm::normalize(target - m_pos);

        switch (preset)
        {
        case 1: m_pos = target - to_target * 1.5f; break;
        case 2: m_pos = target - to_target * 3.0f; break;
        case 3: m_pos = target - to_target * 5.0f; break;
        case 4: m_pos = target - to_target * 8.0f; break;
        default: break;
        }

        LookAt(m_pos, target);
    }

    void Camera::HandleMouseView(float dx, float dy)
    {
        float scale = 0.005f;
        m_forward = Rotate(m_up, -dx * scale, m_forward);
        m_forward = Rotate(m_right, -dy * scale, m_forward);
        m_forward = glm::normalize(m_forward);

        m_right = glm::cross(m_forward, m_up);
        m_right = glm::vec3(m_right.x, 0.0f, m_right.z);
        m_right = glm::normalize(m_right);

        m_up = glm::cross(m_right, m_forward);
        m_up = glm::normalize(m_up);
        m_forward = glm::cross(m_up, m_right);
    }

    void Camera::HandleMouseOrbit(float dx, float dy, const glm::vec3& center)
    {
        glm::vec3 offset = m_pos - center;
        glm::vec3 local = {
            glm::dot(m_right,   offset),
            glm::dot(m_forward, offset),
            glm::dot(m_up,      offset)
        };

        float scale = 0.01f;
        m_forward = Rotate(m_up, -dx * scale, m_forward);
        m_forward = Rotate(m_right, -dy * scale, m_forward);
        m_up = Rotate(m_up, -dx * scale, m_up);
        m_up = Rotate(m_right, -dy * scale, m_up);

        m_right = glm::cross(m_forward, m_up);
        m_right = glm::vec3(m_right.x, 0.0f, m_right.z);
        m_right = glm::normalize(m_right);

        m_up = glm::cross(m_right, m_forward);
        m_up = glm::normalize(m_up);
        m_forward = glm::cross(m_up, m_right);

        m_pos = center
            + m_right * local.x
            + m_forward * local.y
            + m_up * local.z;
    }

    void Camera::HandleMouseTranslate(float dx, float dy)
    {
        float scale = glm::length(m_pos) * 0.001f;
        m_pos -= m_right * scale * dx;
        m_pos += m_up * scale * dy;
    }

    void Camera::HandleWheel(float direction)
    {
        m_pos += m_forward * direction * m_speed;
    }

    glm::mat4 Camera::GetViewMatrix() const
    {
        return glm::lookAt(m_pos, m_pos + m_forward, m_up);
    }

    glm::mat4 Camera::GetProjectionMatrix(float aspect) const
    {
        return glm::perspective(glm::radians(m_fov), aspect, m_near, m_far);
    }

} // namespace Safemon