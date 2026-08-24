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

#define INIT_CONFIG_PATH "./init.conf"

/**
 * @brief struct used for the starting configuration
 */
struct StartingConfig
{
	bool vr_mode_enabled = true;
	bool flying_mode_enabled = true;
	bool create_octree_bin = false;
	size_t max_points = 0;
	char path[512] = "";
	unsigned int node_resolution = 64;
	float max_lod_distance = 100.0f;
	float max_vram_gb = 1.0f;
	unsigned int wwidth = 800;
	unsigned int wheight = 600;
	float bg_color[3] = { 0.0f, 0.0f, 0.0f };
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
	void DrawDebugHudInfo();
	
	/**
	 * @brief Render app info menu
	 */
	void DrawInfoMenu(bool show_controls);
	
	/**
	 * @brief Initializes HUD frame
	 */
	void InitFrame();

	/**
	 * @brief Renders all drawn elements
	 */
	void Render();
	
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
	
	/**
	 * @brief Loads the previous configuration used and stored in cache file
	 * @details If no cache file config is not modified
	 * @param config: Configuration to be loaded
	 */
	void loadCachedConfig(StartingConfig& config);

	/**
	 * @brief Stores current configuration in cache file for next execution
	 * @param config: Configuration to be cached
	 */
	void storeCachedConfig(StartingConfig& config);
	
	/**
	 * @brief Apply a custom theme to the menu
	 */
	void applyTheme();

	/**
	 * @brief Opens a URL into the default browser of the system
	 * @param url: URL to open in browser
	 */
	void openURL(const char* url);
	
	/**
	 * @brief Draws an underlined string with a url attached
	 * @param str: string to draw
	 * @param url: URL attached to the string
	 */
	void writeUnderlinedLink(std::string str, std::string url);

};




#endif