#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

// Camara FPS libre: WASD para moverse, mouse para rotar, scroll para velocidad.
class Camera {
public:
    glm::vec3 position;
    float yaw;           // grados, horizontal
    float pitch;         // grados, vertical
    float speed;         // unidades/segundo
    float sensitivity;   // grados por pixel
    float fov;           // campo de vision en grados

    Camera(glm::vec3 pos  = glm::vec3(0.0f, 60.0f, 250.0f),
           float    yaw_  = -90.0f,
           float    pitch_= -13.0f)
        : position(pos), yaw(yaw_), pitch(pitch_),
          speed(80.0f), sensitivity(0.08f), fov(45.0f)
    {
        updateVectors();
    }

    glm::mat4 getView() const {
        return glm::lookAt(position, position + front, up);
    }

    void moveForward (float dt) { position += front   * (speed * dt); }
    void moveBackward(float dt) { position -= front   * (speed * dt); }
    void moveLeft    (float dt) { position -= right   * (speed * dt); }
    void moveRight   (float dt) { position += right   * (speed * dt); }
    void moveUp      (float dt) { position += worldUp * (speed * dt); }
    void moveDown    (float dt) { position -= worldUp * (speed * dt); }

    void processMouse(float dx, float dy) {
        yaw   += dx * sensitivity;
        pitch += dy * sensitivity;
        pitch = std::max(-89.0f, std::min(89.0f, pitch));
        updateVectors();
    }

    // Scroll ajusta la velocidad de desplazamiento
    void processScroll(float yoffset) {
        speed += yoffset * 8.0f;
        speed = std::max(5.0f, std::min(800.0f, speed));
    }

    void resetPosition() {
        position  = glm::vec3(0.0f, 60.0f, 250.0f);
        yaw       = -90.0f;
        pitch     = -13.0f;
        updateVectors();
    }

    // Vector frontal (para uso externo si se necesita)
    const glm::vec3& getFront() const { return front; }

private:
    glm::vec3 front  {0.0f,  0.0f, -1.0f};
    glm::vec3 right  {1.0f,  0.0f,  0.0f};
    glm::vec3 up     {0.0f,  1.0f,  0.0f};
    glm::vec3 worldUp{0.0f,  1.0f,  0.0f};

    void updateVectors() {
        float yR = glm::radians(yaw);
        float pR = glm::radians(pitch);
        front.x = cosf(yR) * cosf(pR);
        front.y = sinf(pR);
        front.z = sinf(yR) * cosf(pR);
        front = glm::normalize(front);
        right = glm::normalize(glm::cross(front, worldUp));
        up    = glm::normalize(glm::cross(right, front));
    }
};
