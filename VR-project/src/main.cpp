#include <extern/CLI/CLI.hpp>

#include <Shader.hpp>
#include <Model.hpp>

#include <VRManager.hpp>
#include <SceneManager.hpp>


int main(int argc, char** argv)
{
    // Get CLI parameters
    CLI::App app{"VR Point Cloud Viewer"};
    bool vr_mode_enabled = false;
    bool flying_mode_enabled = false;

    app.add_flag("-v,--vr", vr_mode_enabled, "VR Mode enabled");
    app.add_flag("-f,--flying", flying_mode_enabled, "Flying mode enabled");

    CLI11_PARSE(app, argc, argv);

    SceneManager scene_manager(800, 600, vr_mode_enabled);
    scene_manager.FlyingMode(flying_mode_enabled);

    // Build and compile shaders
    Shader ourShader("assets/shaders/shader.vert", "assets/shaders/shader.frag");
    scene_manager.AddShader(ourShader, "main_shader");
    
    // Load Models
    Model ourModel("assets/objects/backpack/backpack.obj");
    scene_manager.AddModel(ourModel, "main_model");
    
    scene_manager.ExecuteRenderLoop();
   
    return 0;
}