#pragma once
#include <glm/glm.hpp>

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float3.hpp"

class Camera {
public:
    Camera(glm::mat4 const& projection, glm::vec2 const& pitch_limits);
    ~Camera() = default;

    void set_position(glm::vec3 const& new_position);
    void update_position(glm::vec3 const& delta);

    void set_pitch(float new_pitch);
    void update_pitch(float delta);
    float get_pitch();

    void set_yaw(float new_yaw);
    void update_yaw(float delta);
    float get_yaw();

    void set_roll(float new_roll);
    void update_roll(float delta);

    void set_model(glm::mat4 const& model_matrix);

    glm::vec3 const& position() const;

    glm::vec3 const& forward() const;
    glm::vec3 const& right() const;
    glm::mat4 const& vp() const;

    glm::mat4 const& model() const;

    void update();

private:
    glm::vec3 m_forward;
    glm::vec3 m_right;
    glm::mat4 m_vp;
    glm::mat4 m_model;

    glm::mat4 m_projection;
    glm::vec3 m_position;
    glm::vec3 m_angles;
    glm::vec2 m_pitch_limits;
};
