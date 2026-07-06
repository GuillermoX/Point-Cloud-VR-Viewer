#ifndef VRMANAGER_H
#define VRMANAGER_H

#include <windows.h>
#include <vector>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

// Tell OpenXR we are in Windows and Using OpenGL
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_OPENGL
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

// Needed to use types HDC and HGLRC in constructor declaration
#define GLFW_EXPOSE_NATIVE_WIN32 
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>



class VRManager
{
public:
	// Main values of OpenXR
	XrInstance m_instance;       // Represents conection with OpenXR
	XrSystemId m_systemId;       // ID of VR Headset (Quest 3)
	XrSession  m_session;        // Current session
	XrSpace    m_appSpace;       // Coordinates origin
	XrSwapchain m_swapchain;     // Where OpenGL paints
	std::vector<XrSwapchainImageOpenGLKHR> m_swapchainImages;
	uint32_t m_visorHeight;
	uint32_t m_visorWidth;
	
	VRManager(GLFWwindow* glfwWindow);
	uint32_t InitFrame();
	void GetViewProjectionMatrices();
	void FinishFrame();
	glm::vec3 GetCameraPosition(int eyeIndex);
	glm::mat4 GetViewMatrix(int eyeIndex);
	glm::mat4 GetProjectionMatrix(int eyeIndex);

private:
	XrFrameState frameState;
	XrView views[2];
};



#endif