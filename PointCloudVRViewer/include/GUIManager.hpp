#ifndef GUI_MANAGER_H
#define GUI_MANAGER_H

#include <GLFW/glfw3.h>

#include <imgui/imgui.h>		// GUI Library
#include <imgui/imgui_impl_glfw.h>		
#include <imgui/imgui_impl_opengl3.h>		
#include <string>

#include <vector>
#include <algorithm>


struct StartingConfig
{
	bool vr_mode_enabled = false;
	bool flying_mode_enabled = false;
	bool create_octree_bin = false;
	size_t max_points = 0;
	std::string path = "";
	unsigned int node_resolution = 64;
	float max_lod_distance = 100.0f;
	float max_vram_gb = 1.0f;
	unsigned int wwidth = 800;
	unsigned int wheight = 600;
};

class GUIManager
{
public:
	GUIManager(GLFWwindow* window);
	bool StartMenu(StartingConfig& config);

private:
	GLFWwindow* _window;
};




#endif