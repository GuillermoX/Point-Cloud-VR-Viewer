#include <Shader.hpp>
#include <Model.hpp>

#include <VRManager.hpp>
#include <SceneManager.hpp>


int main()
{
    SceneManager scene_manager(800, 600, true);

    // Build and compile shaders
    Shader ourShader("assets/shaders/shader.vert", "assets/shaders/shader.frag");
    scene_manager.AddShader(ourShader, "main_shader");
    
    // Load Models
    Model ourModel("assets/objects/backpack/backpack.obj");
    scene_manager.AddModel(ourModel, "main_model");
    
    scene_manager.ExecuteRenderLoop();
   
    return 0;
}