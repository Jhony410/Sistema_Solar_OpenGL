#version 330 core

in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform float uTime;

void main() {
    vec4 texel = texture(uTexture, vUV);

    // Pulso suave que simula la actividad solar
    float pulse = 1.0 + 0.04 * sin(uTime * 1.8) + 0.02 * sin(uTime * 3.5);

    // Halo anaranjado en los bordes del sol
    vec2 uv = vUV * 2.0 - 1.0;
    float rim = 1.0 - length(uv) * 0.5;
    rim = clamp(rim, 0.0, 1.0);

    vec3 glow = texel.rgb * pulse;
    glow += vec3(0.3, 0.1, 0.0) * (1.0 - rim) * 0.3;

    FragColor = vec4(glow, 1.0);
}
