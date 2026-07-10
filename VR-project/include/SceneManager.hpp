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


#include "Camera.hpp"
#include "Shader.hpp"
#include "Model.hpp"
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
	bool wire_frame_enabled = false;
	bool flying_enabled = false;
};


class SceneManager
{
public:
	std::unordered_map<std::string, Shader> shaders;
	std::unordered_map<std::string, Model> models;
	std::shared_ptr<PointCloud> point_cloud;

	SceneManager(unsigned int scr_width, unsigned int scr_height, bool vr_enabled = false);
	~SceneManager();

	/**
	* @brief Adds a Model to the SceneManager
	* @return 0 -> added model correctly
			  -1 -> error while adding shader
	*/
	unsigned char AddShader(Shader& shader, std::string shader_id);

	/**
	* @brief Adds a Model to the SceneManager
	* @return 0 -> added model correctly
			  -1 -> error while adding model
	*/
	unsigned char AddModel(Model& model, std::string model_id);

	unsigned char AddPointCloud(PointCloud& point_cloud);

	void ExecuteRenderLoop();

	bool SceneIsValid();

	// Scene configuration manager
	void FlyingMode(bool active);

private:
	screen_manager_t _screen;
	timing_t _timing;
	scene_config_t _scene_config;
	bool _scene_valid = true;

	std::unique_ptr<Camera> _camera_ptr;
	std::unique_ptr<VRManager> _vr_manager_ptr;

	GLFWwindow* _window;

	GLuint _vr_FBO;
	GLuint _vr_depth_buffer;

	void processInput();
	
	// GLFW Callback functions
	static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
	static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
	static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
};




#endif