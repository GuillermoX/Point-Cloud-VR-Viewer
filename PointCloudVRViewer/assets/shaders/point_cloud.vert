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

	float pointRadiusInMeters = 0.005; // Each point is 0.5 cm in real life

	// Keeps the same constant physical size in pixels 
	gl_PointSize = (pointRadiusInMeters * projection[1][1] * screenHeight) / gl_Position.w;
}
