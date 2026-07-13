#version 330 core
layout (location = 0) in vec3 aPos;

// Transformation matrices
uniform mat4 vpm;
uniform mat4 insideModel;

void main()
{
	gl_Position = vpm * insideModel * vec4(aPos, 1.0);
}
