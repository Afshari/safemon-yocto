#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Safemon {

    class Camera
    {
    public:
        Camera();

        void LookAt(const glm::vec3& pos, const glm::vec3& target);
        void SetPreset(int preset, const glm::vec3& target);

        void HandleMouseView(float dx, float dy);
        void HandleMouseOrbit(float dx, float dy, const glm::vec3& center);
        void HandleMouseTranslate(float dx, float dy);
        void HandleWheel(float direction);
        void HandleKeyW() { m_pos += m_forward * m_speed; }
        void HandleKeyS() { m_pos -= m_forward * m_speed; }
        void HandleKeyA() { m_pos -= m_right * m_speed; }
        void HandleKeyD() { m_pos += m_right * m_speed; }
        void HandleKeyQ() { m_pos += m_up * m_speed; }
        void HandleKeyE() { m_pos -= m_up * m_speed; }
        void SpeedUp()   { m_speed *= 1.2f; }
        void SpeedDown() { m_speed *= 0.8f; }

        glm::mat4 GetViewMatrix()       const;
        glm::mat4 GetProjectionMatrix(float aspect) const;

        const glm::vec3& GetPos()     const { return m_pos; }
        const glm::vec3& GetForward() const { return m_forward; }
        const glm::vec3& GetRight()   const { return m_right; }
        const glm::vec3& GetUp()      const { return m_up; }
        float             GetSpeed()   const { return m_speed; }

    private:
        glm::vec3 Rotate(const glm::vec3& axis, float angle, const glm::vec3& v) const;

        glm::vec3 m_pos     = { 0.0f, 2.0f, 6.0f };
        glm::vec3 m_forward = { 0.0f, 0.0f, -1.0f };
        glm::vec3 m_up      = { 0.0f, 1.0f,  0.0f };
        glm::vec3 m_right   = { 1.0f, 0.0f,  0.0f };
        float     m_speed   = 0.03f;
        float     m_fov     = 40.0f;
        float     m_near    = 0.01f;
        float     m_far     = 1000.0f;
    };

} // namespace Safemon