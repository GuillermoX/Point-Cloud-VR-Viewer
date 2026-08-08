#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H

#include <glad/glad.h>

#include <unordered_map>
#include <string>
#include <memory>
#include <thread>
#include <chrono>

#define GLFW_EXPOSE_NATIVE_WIN32 
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#define STB_IMAGE_IMPLEMENTATION

#include <glm/gtc/type_ptr.hpp>

#include <extern/imgui/imgui.h>		// GUI Library
#include <extern/imgui/imgui_impl_glfw.h>		
#include <extern/imgui/imgui_impl_opengl3.h>		


#include "Camera.hpp"
#include "Shader.hpp"
#include "PointCloud.hpp"
#include "VRManager.hpp"

struct screen_manager_t
{
	unsigned int scr_width = 800;
	unsigned int scr_height = 600;

	float lastX;     // position first mouse coordinates to screen center
	float lastY;
	bool firstMouse = true;
};

struct timing_t
{
	float deltaTime = 0.0f;
	float lastFrame = 0.0f;
};

struct scene_config_t
{
	bool vr_enabled;
	bool flying_enabled = false;
	bool octree_debug_enabled = false;
	bool hud_debug_enabled = false;
};

/**
 * @brief Used to check if in previous frame a key was pressed
 */
struct key_control_t
{
	bool octree_debug_key = false;
	bool hud_debug_key = false;
};


/**
 * @brief Manages the scene elements and the render loop
 */
class SceneManager
{
public:
	PointCloud* point_cloud;								// Point Cloud object
	
	/**
	 * @brief SceneManager constructor
	 * @param scr_width: Screen Width
	 * @param scr_height: Screen Height
	 * @param vr_enabled: Specify if the scene is rendered in VR
	 * @param flying_mode: Specify if the camera can fly
	 */
	SceneManager(unsigned int scr_width, unsigned int scr_height, bool vr_enabled = false, bool flying_mode = true);

	/**
	 * @brief SceneManager destructor
	 * @details Destroys GLFW context with all resources in it
	 */
	~SceneManager();

	/**
	 * @brief Adds a Shader to the SceneManager
	 * @param shader: shader to be used to render
	 */
	void AddShader(Shader& shader);

	/**
	 * @brief Adds a PointCloud to the SceneManager
	 * @pram point_cloud: point cloud to be rendered
	 */
	void AddPointCloud(PointCloud& point_cloud);
	
	/**
	 * @brief Renders all elements in scene 
	 * @details Executes the loop until the exit key is pressed.
	 *          If VR is enabled it renders two times per frame (one render per eye)
	 */
	void ExecuteRenderLoop();

private:
	screen_manager_t _screen;							// Screen Manager
	timing_t _timing;									// Timing information
	scene_config_t _scene_config;						// Scene configuration
	key_control_t _keys_state;							// Key states

	std::unique_ptr<Camera> _camera_ptr;				// Non VR camera object
	std::unique_ptr<VRManager> _vr_manager_ptr;			// VR manager (includes camera managment and functions)

	GLFWwindow* _window;								// GLFW window context
	Shader* _shader;										// Shader used to render

	GLuint _vr_FBO;										// VR frame buffer (two layers, one per eye)
	GLuint _vr_depth_buffer;							// VR depth buffer
	
	/**
	 * @brief Process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
	 * @details If VR is enabled it processes joystick movement. If not, it processes input from keyboard.
	 */
	void processInput();

	/**
	 * @brief Process debug option keys
	 * @param key: Key to be checked as pressed
	 * @param option: Boolean option to be changed if key is pressed
	 * @param debug_key: Variable to know the state of the key the last frame
	 */
	void processDebugOption(unsigned int key, bool& option, bool& debug_key);

	/**
	 * @brief Renders debug info in a HUD handled by ImGui library
	 */
	void renderDebugHudInfo();
	
	/**
	 * @brief GLFW Framebuffer Size Callback Function
	 * @details It executes when screen is resized
	 * @param window: Window GLFW context
	 * @param width: Screen width
	 * @param height: Screen height
	 */
	static void framebuffer_size_callback(GLFWwindow* window, int width, int height);

	/**
	 * @brief GLFW Mouse Callback Function
	 * @details It executes when mouse is moved
	 * @param window: Window GLFW context
	 * @param xpos: mouse X position
	 * @param ypos: mouse Y position
	 */
	static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
};

#endif