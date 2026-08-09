#include <CLI/CLI.hpp>

#include <Shader.hpp>

#include <VRManager.hpp>
#include <SceneManager.hpp>
#include <PointCloud.hpp>


int main(int argc, char** argv)
{
    // Get CLI parameters
    CLI::App app{"Point Cloud VR Viewer"};
    bool vr_mode_enabled = false;
    bool flying_mode_enabled = false;
    bool create_octree_bin = false;
    size_t max_points = 0;
    std::string path = "";
    unsigned int node_resolution = 64;
    MAX_LOD_DISTANCE = 100.0f;
    float max_vram_gb = 1.0f;
    unsigned int wwidth = 800;
    unsigned int wheight = 600;

    app.add_flag("-v,--vr", vr_mode_enabled, "VR Mode enabled");
    app.add_flag("-f,--flying", flying_mode_enabled, "Flying mode enabled");
    CLI::Option* opt_binary = app.add_flag("-b,--binary-octree", create_octree_bin, "Create Binary Octree");
    app.add_option("-p,--points", max_points, "Create Binary Octree with specified max number of points")
        ->needs(opt_binary);
    app.add_option("-r,--resolution-box", node_resolution, "Resolution of node box")
        ->needs(opt_binary);
    app.add_option("-c,--cloud-path", path, "Path of point cloud files")
        ->required();
    app.add_option("-d,--distance", MAX_LOD_DISTANCE, "Maximum LOD distance (distance where LOD is minimum)");
    app.add_option("-g,--gpumem", max_vram_gb, "Maximum memory to use by GPU (GBytes)");
    app.add_option("-H,--wheight", wheight, "Window Height (non VR mode)");
    app.add_option("-W,--wwidth", wwidth, "Window Width (non VR mode)");

    CLI11_PARSE(app, argc, argv);

    MAX_VRAM_BYTES = max_vram_gb * GB;

    SceneManager scene_manager(wwidth, wheight, vr_mode_enabled, flying_mode_enabled);

    // Build and compile shaders
    Shader ourShader("assets/shaders/point_cloud.vert", "assets/shaders/point_cloud.frag");
    scene_manager.AddShader(ourShader);    

    // Load Cloud Point
    bool point_cloud_created;
    PointCloud pointCloud(path, node_resolution, create_octree_bin, max_points, point_cloud_created);
    if (!point_cloud_created)
        return 1;
    scene_manager.AddPointCloud(pointCloud);
    
    scene_manager.ExecuteRenderLoop();
   
    return 0;
}