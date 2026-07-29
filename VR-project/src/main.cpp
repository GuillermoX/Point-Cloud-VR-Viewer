#include <extern/CLI/CLI.hpp>

#include <Shader.hpp>
#include <Model.hpp>

#include <VRManager.hpp>
#include <SceneManager.hpp>
#include <PointCloud.hpp>


int main(int argc, char** argv)
{
    // Get CLI parameters
    CLI::App app{"VR Point Cloud Viewer"};
    bool vr_mode_enabled = false;
    bool flying_mode_enabled = false;
    bool create_octree_bin = false;
    size_t max_points = 0;
    std::string path = "";
    unsigned int node_resolution = 64;
    MAX_LOD_DISTANCE = 100;

    app.add_flag("-v,--vr", vr_mode_enabled, "VR Mode enabled");
    app.add_flag("-f,--flying", flying_mode_enabled, "Flying mode enabled");
    CLI::Option* opt_binary = app.add_flag("-b,--binary-octree", create_octree_bin, "Create Binary Octree");
    app.add_option("-p,--points", max_points, "Create Binary Octree with specified max number of points")
        ->needs(opt_binary);
    app.add_option("-r,--resolution-box", node_resolution, "Resolution of node box")
        ->needs(opt_binary);
    app.add_option("-c,--cloud", path, "Path of point cloud files")
        ->required();
    app.add_option("-d,--distance", MAX_LOD_DISTANCE, "Create Binary Octree with specified max number of points");

    CLI11_PARSE(app, argc, argv);


    SceneManager scene_manager(800, 600, vr_mode_enabled);
    scene_manager.FlyingMode(flying_mode_enabled);

    // Build and compile shaders
    Shader ourShader("assets/shaders/point_cloud.vert", "assets/shaders/point_cloud.frag");
    scene_manager.AddShader(ourShader, "point_cloud");    

    // Load Cloud Point
    PointCloud pointCloud(path, 64, create_octree_bin, max_points);
    scene_manager.AddPointCloud(pointCloud);
    
    scene_manager.ExecuteRenderLoop();
   
    return 0;
}