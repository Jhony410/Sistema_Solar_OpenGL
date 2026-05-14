#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <cmath>

// Representa un cuerpo celeste: planeta, luna o el Sol.
// Almacena parametros orbitales y de rotacion; calcula posicion mundial y matriz model.
struct Planet {
    std::string  name;

    // --- Geometria visual ---
    float        radius;         // radio visual (unidades de escena)

    // --- Parametros orbitales ---
    float        orbitRadius;    // distancia al centro de orbita (0 para el Sol)
    float        orbitSpeed;     // velocidad angular orbital (rad/s simulado)
    float        orbitAngle;     // angulo actual en orbita (rad)

    // --- Rotacion axial ---
    float        rotationSpeed;  // velocidad de rotacion propia (rad/s, negativo = retrograda)
    float        rotAngle;       // angulo actual de rotacion
    float        axialTilt;      // inclinacion del eje (grados)

    // --- Textura OpenGL ---
    unsigned int texture = 0;

    // --- Jerarquia: null = orbita el Sol, no-null = orbita ese planeta (Luna) ---
    Planet*      parent = nullptr;

    // Avanza la simulacion un paso de tiempo dt (segundos simulados)
    void update(float dt) {
        orbitAngle += orbitSpeed    * dt;
        rotAngle   += rotationSpeed * dt;
    }

    // Posicion en el espacio mundial (tiene en cuenta el padre)
    glm::vec3 worldPos() const {
        glm::vec3 base(0.0f);
        if (parent)
            base = parent->worldPos();

        return base + glm::vec3(
            orbitRadius * cosf(orbitAngle),
            0.0f,
            orbitRadius * sinf(orbitAngle)
        );
    }

    // Matriz model completa: traslacion, inclinacion axial, rotacion propia, escala
    glm::mat4 modelMatrix() const {
        glm::mat4 m(1.0f);
        m = glm::translate(m, worldPos());
        m = glm::rotate(m, glm::radians(axialTilt), glm::vec3(0.0f, 0.0f, 1.0f));
        m = glm::rotate(m, rotAngle,                glm::vec3(0.0f, 1.0f, 0.0f));
        m = glm::scale (m, glm::vec3(radius));
        return m;
    }
};
