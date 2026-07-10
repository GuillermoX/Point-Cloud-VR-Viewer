#version 330 core
out vec4 FragColor;

in vec3 PointColor; // Comes from Vertex Shader

void main() {
    // Calcular la distancia desde el centro del punto (0.5, 0.5)
    /*vec2 centerOffset = gl_PointCoord - vec2(0.5, 0.5);
    float distanceSquared = dot(centerOffset, centerOffset);

    // Si la distancia al cuadrado es mayor que 0.25 (radio 0.5 al cuadrado),
    // significa que el píxel cae fuera del círculo, así que lo destruimos.
    if (distanceSquared > 0.25) {
        discard;
    }*/

    // Si cae dentro, lo pintamos con su color real
    FragColor = vec4(PointColor, 1.0);
}
