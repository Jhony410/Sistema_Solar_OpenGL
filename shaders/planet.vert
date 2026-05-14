#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

out vec3 vFragPos;
out vec3 vNormal;
out vec2 vUV;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    vFragPos  = worldPos.xyz;
    vNormal   = normalMatrix * aNormal;
    vUV       = aUV;
    gl_Position = projection * view * worldPos;
}
