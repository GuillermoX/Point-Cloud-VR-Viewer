#include <CLI/CLI.hpp>

#include <Shader.hpp>

#include <VRManager.hpp>
#include <SceneManager.hpp>
#include <PointCloud.hpp>
#include <GUIManager.hpp>


int main(int argc, char** argv)
{
	StartingConfig config;

    SceneManager scene_manager(config.wwidth, config.wheight);
	GUIManager* gui_manager = GUIManager::Init(scene_manager.GetGLFWwindow());	

    if (argc > 1)	// If CLI parameters specified
    {
		std::string temp_path;	// Temporal path string to avoid problems with raw char[] config path
		// Get CLI parameters
		CLI::App app{"Point Cloud VR Viewer"};
	
		app.add_flag("-v,--vr", config.vr_mode_enabled, "VR Mode enabled");
		app.add_flag("-f,--flying", config.flying_mode_enabled, "Flying mode enabled");
		CLI::Option* opt_binary = app.add_flag("-b,--binary-octree", config.create_octree_bin, "Create Binary Octree");
		app.add_option("-p,--points", config.max_points, "Create Binary Octree with specified max number of points")
			->needs(opt_binary);
		app.add_option("-r,--resolution-box", config.node_resolution, "Resolution of node box (number of points per dimension)")
			->needs(opt_binary);
		app.add_option("-c,--cloud-path", temp_path, "Path of point cloud files")
			->required();
		app.add_option("-d,--distance", config.max_lod_distance, "Maximum LOD distance (distance where LOD is minimum)");
		app.add_option("-g,--gpumem", config.max_vram_gb, "Maximum memory to use by GPU (GBytes)");
		app.add_option("-H,--wheight", config.wheight, "Window Height (non VR mode)");
		app.add_option("-W,--wwidth", config.wwidth, "Window Width (non VR mode)");

		CLI11_PARSE(app, argc, argv);

		snprintf(config.path, sizeof(config.path), "%s", temp_path.c_str());	// Copy temporal string to char[] path
    }
	else
	{
		if (!gui_manager->StartMenu(config))
			return 0;	// If exit menu without starting engine exit program
	}

    MAX_VRAM_BYTES = config.max_vram_gb * GB;
	MAX_LOD_DISTANCE = config.max_lod_distance;

	scene_manager.ChangeWindowResolution(config.wwidth, config.wheight);
	scene_manager.SetBackgroundColor(config.bg_color);
	scene_manager.SetVREnabled(config.vr_mode_enabled);
	scene_manager.SetFlyingEnabled(config.flying_mode_enabled);

    // Build and compile shaders
    Shader shader("assets/shaders/point_cloud.vert", "assets/shaders/point_cloud.frag");
    scene_manager.AddShader(shader);    

    // Load Cloud Point
    bool point_cloud_created;
    PointCloud pointCloud(config.path, config.node_resolution, config.create_octree_bin, config.max_points, point_cloud_created);
    if (!point_cloud_created)
        return 1;
    scene_manager.AddPointCloud(pointCloud);
    
    scene_manager.ExecuteRenderLoop();
   
    return 0;
}