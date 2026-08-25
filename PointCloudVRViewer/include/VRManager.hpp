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
	bool m_session_running = false;
	
	// Action variables
	XrActionSet m_actionSet{XR_NULL_HANDLE};
	XrAction    m_moveAction{XR_NULL_HANDLE};
	XrPath		m_leftHandPath{XR_NULL_PATH};

	glm::vec3 virtual_position = glm::vec3(0.0f);	// Position used to translate camera depending on user input joystick
	float movement_speed = 2.5f;
	
	/**
	 * @brief VRManager constructor
	 * @details Initializes all OpenXR context
	 * @param glfwWindow: GLFW Window context
	 */
	VRManager(GLFWwindow* glfwWindow);

	/**
	 * @brief Initializes the VR frame
	 * @return uint32_t: Index of swapchain image from m_swapchainImages vector
	 */
	uint32_t InitFrame();
	
	/**
	 * @brief Finishes the OpenXR frame
	 */
	void FinishFrame();

	/**
	 * @brief Get the camera position of an VR eye
	 * @param eyeIndex: Index of the eye (0 -> left, 1 -> right)
	 */
	glm::vec3 GetCameraPosition(int eyeIndex);

	/**
	 * @brief Get view matrix of an VR eye
	 * @param eyeIndex: Index of the eye (0 -> left, 1 -> right)
	 */
	glm::mat4 GetViewMatrix(int eyeIndex);

	/**
	 * @brief Get projection matrix of an VR eye
	 * @param eyeIndex: Index of the eye (0 -> left, 1 -> right)
	 */
	glm::mat4 GetProjectionMatrix(int eyeIndex);

	/**
	 * @brief Processes input from user
	 * @param delta_time: Delta time of current frame
	 * @param flying_enabled: True if camera can fly
	 */
	void ProcessInput(float delta_time, bool flying_enabled);

private:
	XrFrameState frameState;
	XrView views[2];
	
	/**
	 * @brief Syncronices the OpenXR state
	 */
	void syncActions();
};


#endif