#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 PointColor;

// Transformation matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * vec4(aPos, 0.0);
	PointColor = aColor;	// Send color to fragment shader

	float baseSize = 20.0;
	//gl_PointSize = baseSize / glPosition.w;
}
