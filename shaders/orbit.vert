#version 330 core

layout(location = 0) in vec3 aPos;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 uCenter;   // centro de la orbita (origen del sol, o posicion del planeta padre)

void main() {
    vec3 worldPos = aPos + uCenter;
    gl_Position = projection * view * vec4(worldPos, 1.0);
}
