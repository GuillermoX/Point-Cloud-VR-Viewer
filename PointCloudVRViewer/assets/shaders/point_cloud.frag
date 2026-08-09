#version 330 core
out vec4 FragColor;

in vec3 PointColor; // Comes from Vertex Shader

uniform float fadeAlpha;

void main() {
    // Coordenadas respecto al centro del punto
    vec2 circCoord = gl_PointCoord - vec2(0.5);
    float dist = length(circCoord);

    // Si está fuera del círculo, lo borramos
    if (dist > 0.45) {
        discard;
    }

    FragColor = vec4(PointColor, fadeAlpha);

}
