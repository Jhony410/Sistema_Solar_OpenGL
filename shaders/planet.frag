#version 330 core

in vec3 vFragPos;
in vec3 vNormal;
in vec2 vUV;

out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec3  uLightPos;
uniform vec3  uViewPos;
uniform vec3  uLightColor;
uniform float uAmbient;
uniform float uSpecularStr;
uniform float uShininess;

// Destello de emergencia: luz desde la camara para iluminar zonas oscuras
uniform int   uFlashlight;      // 0 = apagado, 1 = encendido
uniform vec3  uFlashlightPos;   // posicion de la camara

void main() {
    vec4 texel = texture(uTexture, vUV);
    vec3 norm  = normalize(vNormal);

    // --- Iluminacion del Sol (Blinn-Phong) ---
    float effectiveAmbient = uFlashlight != 0 ? max(uAmbient, 0.45) : uAmbient;
    vec3 ambient  = effectiveAmbient * uLightColor;

    vec3 lightDir = normalize(uLightPos - vFragPos);
    float diff    = max(dot(norm, lightDir), 0.0);
    vec3 diffuse  = diff * uLightColor;

    vec3 viewDir  = normalize(uViewPos - vFragPos);
    vec3 halfDir  = normalize(lightDir + viewDir);
    float spec    = pow(max(dot(norm, halfDir), 0.0), uShininess);
    vec3 specular = uSpecularStr * spec * uLightColor;

    vec3 result = (ambient + diffuse + specular) * texel.rgb;

    // --- Destello de emergencia: punto de luz en la camara ---
    if (uFlashlight != 0) {
        float dist   = length(uFlashlightPos - vFragPos);
        vec3  fDir   = normalize(uFlashlightPos - vFragPos);
        float fDiff  = max(dot(norm, fDir), 0.0);
        float att    = 1.0 / (1.0 + 0.04 * dist + 0.0003 * dist * dist);
        // Luz ligeramente azul-blanca, como una linterna espacial
        vec3 flash   = fDiff * att * vec3(0.80, 0.88, 1.0) * 6.0;
        result      += flash * texel.rgb;
    }

    FragColor = vec4(result, texel.a);
}
