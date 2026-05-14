#version 330 core

in float vBrightness;
out vec4 FragColor;

void main() {
    // Punto circular suavizado
    vec2  coord = gl_PointCoord - vec2(0.5);
    float dist  = length(coord);
    if (dist > 0.5) discard;

    float alpha = (1.0 - smoothstep(0.0, 0.5, dist)) * vBrightness;

    // Mezcla entre blanco azulado (estrellas jovenes) y blanco calido (estrellas viejas)
    vec3 col = mix(vec3(0.75, 0.85, 1.0), vec3(1.0, 0.95, 0.85), vBrightness);

    FragColor = vec4(col, alpha);
}
