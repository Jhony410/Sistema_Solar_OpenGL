#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in float aBrightness;

out float vBrightness;

uniform mat4 view;
uniform mat4 projection;

void main() {
    vBrightness = aBrightness;
    gl_Position  = projection * view * vec4(aPos, 1.0);
    gl_PointSize = mix(1.0, 3.8, aBrightness * aBrightness);
}
