#include "Camera.h"

#include <algorithm>
#include <glm/ext.hpp>

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/quaternion_float.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/trigonometric.hpp"

Camera::Camera(glm::mat4 const& projection, glm::vec2 const& pitch_limits) {
    m_projection = projection;
    m_angles = glm::vec3(0.0f);
    m_position = glm::vec3(0.0f);
    m_pitch_limits = glm::radians(pitch_limits);
    m_model = glm::identity<glm::mat4>();
}

void Camera::set_position(glm::vec3 const& new_position) {
    m_position = new_position;
}

void Camera::update_position(glm::vec3 const& delta) {
    m_position += delta;
}

void Camera::set_pitch(float new_pitch) {
    m_angles.x = glm::radians(new_pitch);
    m_angles.x = std::clamp(m_angles.x, m_pitch_limits.x, m_pitch_limits.y);
}

void Camera::update_pitch(float delta) {
    m_angles.x += glm::radians(delta);
    m_angles.x = std::clamp(m_angles.x, m_pitch_limits.x, m_pitch_limits.y);
}

float Camera::get_pitch() {
    return glm::degrees(m_angles.x);
}

void Camera::set_yaw(float new_yaw) {
    m_angles.y = glm::radians(new_yaw);
}

void Camera::update_yaw(float delta) {
    m_angles.y += glm::radians(delta);
}

float Camera::get_yaw() {
    return glm::degrees(m_angles.y);
}

void Camera::set_roll(float new_roll) {
    m_angles.z = glm::radians(new_roll);
}

void Camera::update_roll(float delta) {
    m_angles.z += glm::radians(delta);
}

void Camera::set_model(glm::mat4 const& model_matrix) {
    m_model = model_matrix;
}

glm::vec3 const& Camera::position() const {
    return m_position;
}

glm::vec3 const& Camera::forward() const {
    return m_forward;
}

glm::vec3 const& Camera::right() const {
    return m_right;
}

glm::mat4 const& Camera::vp() const {
    return m_vp;
}

glm::mat4 const& Camera::model() const {
    return m_model;
}

void Camera::update() {
    constexpr glm::vec3 forward(0.0f, 0.0f, 1.0f);
    constexpr glm::vec3 right(-1.0f, 0.0f, 0.0f);
    glm::quat const rotation = glm::quat(m_angles);

    m_forward = rotation * forward;
    m_right = rotation * right;

    m_vp = m_projection * glm::lookAt(m_position, m_position + m_forward, glm::vec3(0.0f, 1.0f, 0.0f));
}
