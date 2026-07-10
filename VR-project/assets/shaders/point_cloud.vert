#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 PointColor;

// Transformation matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float screenHeight;

void main()
{
	gl_Position = projection * view * model * vec4(aPos, 1.0);
	PointColor = aColor;	// Send color to fragment shader

	float baseSize = 3.0;
	//gl_PointSize = max(baseSize / gl_Position.w, 2.0);
	//gl_PointSize = 3.0;
	// Supongamos que pasas la matriz 'projection' como uniform
	//float pointRadiusInMeters = 0.005; // Cada punto mide 2 cm en el mundo real
	//float screenHeight = 1080.0;     // Resolución vertical de la pantalla del ojo

	// Fórmula matemática para mantener el tamaño físico constante en píxeles
	//gl_PointSize = (pointRadiusInMeters * projection[1][1] * screenHeight) / gl_Position.w;
}
