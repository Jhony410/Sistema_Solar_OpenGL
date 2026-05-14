#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

// Clase que encapsula un programa GLSL (vertex + fragment shader).
// Permite cargar desde archivos y establecer uniforms tipados.
class Shader {
public:
    GLuint ID = 0;

    Shader() = default;

    // Carga, compila y enlaza los dos shaders.
    Shader(const char* vertPath, const char* fragPath) {
        std::string vSrc = readFile(vertPath);
        std::string fSrc = readFile(fragPath);

        GLuint vert = compileStage(GL_VERTEX_SHADER,   vSrc.c_str(), vertPath);
        GLuint frag = compileStage(GL_FRAGMENT_SHADER, fSrc.c_str(), fragPath);

        ID = glCreateProgram();
        glAttachShader(ID, vert);
        glAttachShader(ID, frag);
        glLinkProgram(ID);

        GLint ok;
        glGetProgramiv(ID, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[1024];
            glGetProgramInfoLog(ID, sizeof(log), nullptr, log);
            std::cerr << "[Shader] Link error (" << vertPath << " + " << fragPath
                      << "):\n" << log << "\n";
        }

        glDeleteShader(vert);
        glDeleteShader(frag);
    }

    void use() const { glUseProgram(ID); }

    // --- Uniform setters con sobrecarga ---
    void set(const char* n, int   v) const { glUniform1i (loc(n), v); }
    void set(const char* n, float v) const { glUniform1f (loc(n), v); }
    void set(const char* n, bool  v) const { glUniform1i (loc(n), (int)v); }

    void set(const char* n, const glm::vec2& v) const {
        glUniform2fv(loc(n), 1, glm::value_ptr(v));
    }
    void set(const char* n, const glm::vec3& v) const {
        glUniform3fv(loc(n), 1, glm::value_ptr(v));
    }
    void set(const char* n, const glm::vec4& v) const {
        glUniform4fv(loc(n), 1, glm::value_ptr(v));
    }
    void set(const char* n, const glm::mat3& v) const {
        glUniformMatrix3fv(loc(n), 1, GL_FALSE, glm::value_ptr(v));
    }
    void set(const char* n, const glm::mat4& v) const {
        glUniformMatrix4fv(loc(n), 1, GL_FALSE, glm::value_ptr(v));
    }

private:
    GLint loc(const char* name) const {
        return glGetUniformLocation(ID, name);
    }

    std::string readFile(const char* path) const {
        std::ifstream f(path);
        if (!f.is_open()) {
            std::cerr << "[Shader] No se pudo abrir: " << path << "\n";
            return "";
        }
        std::stringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }

    GLuint compileStage(GLenum type, const char* src, const char* name) const {
        GLuint id = glCreateShader(type);
        glShaderSource(id, 1, &src, nullptr);
        glCompileShader(id);

        GLint ok;
        glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[1024];
            glGetShaderInfoLog(id, sizeof(log), nullptr, log);
            std::cerr << "[Shader] Error compilando (" << name << "):\n" << log << "\n";
        }
        return id;
    }
};
