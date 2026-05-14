#pragma once
#include <glad/glad.h>
#include <vector>
#include <cmath>

// Genera una esfera UV con posicion, normal y coordenadas de textura.
// Todos los planetas comparten la misma geometria de esfera; el tamano
// se controla via la matriz model (escala uniforme).
class Sphere {
public:
    GLuint VAO = 0, VBO = 0, EBO = 0;
    int    indexCount = 0;

    // stacks = divisiones latitudinales, sectors = divisiones longitudinales
    Sphere(int stacks = 64, int sectors = 64) {
        std::vector<float>        verts;
        std::vector<unsigned int> inds;

        const float PI = 3.14159265358979f;

        for (int i = 0; i <= stacks; ++i) {
            float phi = PI * (-0.5f + (float)i / stacks);
            float y   = sinf(phi);
            float r   = cosf(phi);

            for (int j = 0; j <= sectors; ++j) {
                float theta = 2.0f * PI * (float)j / sectors;
                float x     = r * cosf(theta);
                float z     = r * sinf(theta);

                // Posicion (radio unitario)
                verts.push_back(x);
                verts.push_back(y);
                verts.push_back(z);

                // Normal (igual a la posicion en una esfera unitaria)
                verts.push_back(x);
                verts.push_back(y);
                verts.push_back(z);

                // Coordenadas UV
                // v = i/stacks (norte en top) correcto con stbi_flip=true
                verts.push_back((float)j / sectors);
                verts.push_back((float)i / stacks);
            }
        }

        for (int i = 0; i < stacks; ++i) {
            for (int j = 0; j < sectors; ++j) {
                unsigned int p0 = i * (sectors + 1) + j;
                unsigned int p1 = p0 + (sectors + 1);

                inds.push_back(p0);     inds.push_back(p1);     inds.push_back(p0 + 1);
                inds.push_back(p0 + 1); inds.push_back(p1);     inds.push_back(p1 + 1);
            }
        }

        indexCount = (int)inds.size();

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER,
            (GLsizeiptr)(verts.size() * sizeof(float)), verts.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            (GLsizeiptr)(inds.size() * sizeof(unsigned int)), inds.data(), GL_STATIC_DRAW);

        // location 0: posicion (3 floats)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // location 1: normal (3 floats)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
            (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // location 2: UV (2 floats)
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
            (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
    }

    ~Sphere() {
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
        if (EBO) glDeleteBuffers(1, &EBO);
    }

    void draw() const {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }
};
