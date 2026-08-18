#ifndef GUI_MANAGER_H
#define GUI_MANAGER_H

#include <GLFW/glfw3.h>

#include <imgui/imgui.h>		// GUI Library
#include <imgui/imgui_impl_glfw.h>		
#include <imgui/imgui_impl_opengl3.h>		
#include <string>

#include <vector>
#include <algorithm>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>	// Windows API
#include <commdlg.h>	// File explorer API

#pragma comment(lib, "Comdlg32.lib")

#include "VRAMManager.hpp"

/**
 * @brief struct used for the starting configuration
 */
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
	/**
	 * @brief Renders the starting menu
	 * @param config: Configuration struct to be filled with the menu options
	 * @return true if engine should be started, false if user closed the window
	 */
	bool StartMenu(StartingConfig& config);


	/**
	 * @brief Renders debug info in a HUD
	 */
	void RenderDebugHudInfo();
	
	/**
	 * @brief Singleton GUIManager instance initialization
	 * @param window: GLFW window context
	 */
	static GUIManager* Init(GLFWwindow* window)
	{
		if (gui_manager == nullptr)
			gui_manager = new GUIManager(window);

		return gui_manager;
	}
	
	/**
	 * @brief Singleton GUIManager instance getter
	 */
	static GUIManager* GetInstance() { return gui_manager; }

private:
	GLFWwindow* _window;
	inline static GUIManager* gui_manager;

	/**
	 * @breif GUIManager constructor
	 * @details Creates the ImGui context and links it to the OpenGL context
	 * @param window GLFW windows context
	 */
	GUIManager(GLFWwindow* window);		// Private constructor
	~GUIManager() { delete gui_manager; };
	GUIManager(const GUIManager&) = delete;	// Delete copy constructor
	GUIManager& operator=(const GUIManager&) = delete;	// Delete asignation operator
	
	/**
	 * @brief Opens a file explorer window to select a point cloud file
	 */
	std::string openFileDialog();

	void applyTheme();

};




#endif