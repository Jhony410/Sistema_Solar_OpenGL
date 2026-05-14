#version 330 core

in vec2 vUV;
in vec3 vFragPos;
in vec3 vNormal;

out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec3 uLightPos;
uniform vec3 uLightColor;
uniform float uAmbient;

void main() {
    vec4 texel = texture(uTexture, vUV);
    if (texel.a < 0.03) discard;

    vec3 norm     = normalize(vNormal);
    vec3 lightDir = normalize(uLightPos - vFragPos);

    // Doble cara: el anillo se ilumina desde arriba y abajo
    float diff = max(abs(dot(norm, lightDir)), 0.15);

    vec3 result = (uAmbient + diff * (1.0 - uAmbient)) * uLightColor * texel.rgb;
    FragColor   = vec4(result, texel.a * 0.88);
}
