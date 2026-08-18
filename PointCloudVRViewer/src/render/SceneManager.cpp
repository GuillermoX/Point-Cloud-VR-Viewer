#include <SceneManager.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

SceneManager::SceneManager(unsigned int scr_width, unsigned int scr_height) :
    _screen{ scr_width, scr_height, scr_width / 2.0f, scr_height / 2.0f },      // Screen initalization
    _scene_config{false, true},                                     // Scene config initialization
    _vr_FBO{0}, _vr_depth_buffer{0}                                             // No values until VR initialization
{ 

    // GLFW and GLAD initialization
    // ----------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    if(_scene_config.vr_enabled)
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Needed by some VR runtimes

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Glfw window creation
    _window = glfwCreateWindow(scr_width, scr_height, "Point Cloud VR Viewer", NULL, NULL);
    if (_window == NULL)
    {
        std::cout << "[SCENE MANAGER | ERROR] Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(_window);


    // Load window icon
    GLFWimage images[3];
	images[0].pixels = stbi_load("assets/icons/icon_16.png", &images[0].width, &images[0].height, 0, 4);
	images[1].pixels = stbi_load("assets/icons/icon_32.png", &images[1].width, &images[1].height, 0, 4);
	images[2].pixels = stbi_load("assets/icons/icon_48.png", &images[2].width, &images[2].height, 0, 4);

	if (images[0].pixels && images[1].pixels && images[2].pixels) 
	{
		// Add icon to window
		glfwSetWindowIcon(_window, 3, images);
	} 

    for (int i = 0; i < 3; i++) 
	{
		if (images[i].pixels) 
		{
			stbi_image_free(images[i].pixels);
		}
	}
   
    // Bind functions to glfw callbacks
    if (!_scene_config.vr_enabled)
    {
        glfwSetFramebufferSizeCallback(_window, framebuffer_size_callback);
        glfwSetCursorPosCallback(_window, mouse_callback);
    }

    // glad: Load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "[SCENE MANAGER | ERROR] Failed to initialize GLAD" << std::endl;
        return;
    }

    // Configure global opengl state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);         // Alpha blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Make sure point size is enabled
	glEnable(GL_PROGRAM_POINT_SIZE);
    

    if (_scene_config.vr_enabled)
    {    
        std::cout << "INFO: ----- VR IS ENABLED -----"  << std::endl;
        std::cout << "GPU in use with OpenGL: " << glGetString(GL_RENDERER) << std::endl;
        _vr_manager_ptr = std::make_unique<VRManager>(_window);     // Create VR manager
        // Create FBO
        glGenFramebuffers(1, &_vr_FBO);

        // Create Depth Buffer (Z-Buffer), same dimensions as OpenXR visor values
        glGenRenderbuffers(1, &_vr_depth_buffer);
        glBindRenderbuffer(GL_RENDERBUFFER, _vr_depth_buffer);

        // Since using Texture Array (layers per eye), the Depth Buffer also has to be an array
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, 0, GL_DEPTH_COMPONENT24, _vr_manager_ptr->m_visorWidth, _vr_manager_ptr->m_visorHeight);

        // Unbind FBO to avoid interferences
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    else    // Screen Mode Setup 
    { 
        _camera_ptr = std::make_unique<Camera>(glm::vec3(0.0f, 0.0f, 3.0f));    // Create non VR Camera object
    }
 
    glViewport(0, 0, _screen.scr_width, _screen.scr_height);

    // Add this object to GLFW User Pointer to be used in callbacks
    glfwSetWindowUserPointer(_window, this);


}

SceneManager::~SceneManager()
{
    glfwTerminate();
}


void SceneManager::ChangeWindowResolution(unsigned int width, unsigned int height)
{
    glfwSetWindowSize(_window, width, height);
    _screen.scr_height = height;
    _screen.scr_width = width;
}


void SceneManager::AddShader(Shader& shader)
{
    _shader = &shader;
}


void SceneManager::AddPointCloud(PointCloud& point_cloud)
{
    this->point_cloud = &point_cloud;
}

void SceneManager::ExecuteRenderLoop()
{
    // Tell GLFW to capture our mouse
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (_shader == nullptr)
    {
        std::cout << "[SCENE MANAGER | ERROR] No shader specified before executing render loop." << std::endl;
        return;
    }
    // ----------- RENDER LOOP ------------
    while (!glfwWindowShouldClose(_window))
    {
        // Per-frame time logic
        float currentFrame = static_cast<float>(glfwGetTime());
        _timing.deltaTime = currentFrame - _timing.lastFrame;
        _timing.lastFrame = currentFrame;


        // Always Process Input
        glfwPollEvents();       // glfw: Poll IO events (keys pressed/released, mouse moved etc.)
        processInput();


        
        // Always render when VR not enabled OR when VR enabled and Session Running
        // ------------------------------------------------------------------------
        if (!_scene_config.vr_enabled || _vr_manager_ptr->m_session_running)
        {
            // Initialize frame if VR mode ON
            uint32_t imageIndex;
            GLuint openxrTexture;
            if (_scene_config.vr_enabled && _vr_manager_ptr->m_session_running)
            {
                imageIndex = _vr_manager_ptr->InitFrame();
                // Get OpenXR texture from swap chain
                openxrTexture = _vr_manager_ptr->m_swapchainImages[imageIndex].image;
                glViewport(0, 0, _vr_manager_ptr->m_visorWidth, _vr_manager_ptr->m_visorHeight);

            }

            // -------- PER EYE RENDER ----------
            int num_eyes_render = 1;    // In Screen mode we render just one eye
            if (_scene_config.vr_enabled)
                num_eyes_render = 2;    // In VR mode we render two eyes
            for (int eyeIndex = 0; eyeIndex < num_eyes_render; eyeIndex++)
            {
                _shader->use();
				if(_scene_config.vr_enabled) 
					glUniform1f(glGetUniformLocation(_shader->ID, "screenHeight"), _vr_manager_ptr->m_visorHeight);
				else
					glUniform1f(glGetUniformLocation(_shader->ID, "screenHeight"), _screen.scr_height);

                if (_scene_config.vr_enabled)
                {
                    // Bind VR FrameBuffer
                    glBindFramebuffer(GL_FRAMEBUFFER, _vr_FBO);

                    // Bind VR Depth FrameBuffer
                    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _vr_depth_buffer);

                    // Bind OpenXR texture to the VR FBO (eyeIndex indicates texture layer: 0 -> left eye / 1 -> right eye)
                    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, openxrTexture, 0, eyeIndex);
                }

                // Clean screen for current eye
                glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


                // Obtain camera data necessary to render
                // --------------------------------------
                glm::vec3 cameraPosition;
                glm::mat4 projection;
                glm::mat4 view;
                if (_scene_config.vr_enabled)   // If VR enabled camera data obtained from VR Manager
                {
                    cameraPosition = _vr_manager_ptr->GetCameraPosition(eyeIndex);
                    projection = _vr_manager_ptr->GetProjectionMatrix(eyeIndex);
                    view = _vr_manager_ptr->GetViewMatrix(eyeIndex);
                }
                else                            // If VR disabled camera data obtained from Camera object
                {
                    cameraPosition = _camera_ptr->Position;
                    projection = glm::perspective(glm::radians(_camera_ptr->Zoom), (float)_screen.scr_width / (float)_screen.scr_height, 0.1f, 300.0f);
                    view = _camera_ptr->GetViewMatrix();
                }

                // View/projection transformations
                glUniformMatrix4fv(glGetUniformLocation(_shader->ID, "projection"), 1, false, glm::value_ptr(projection));
                glUniformMatrix4fv(glGetUniformLocation(_shader->ID, "view"), 1, false, glm::value_ptr(view));
                
                // Model transformations
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(0.0f, -1.5f, -2.0f)); // Translate in world space
                model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));	// Scale the point cloud
                glUniformMatrix4fv(glGetUniformLocation(_shader->ID, "model"), 1, false, glm::value_ptr(model));
    
                glm::mat4 vpm = projection * view * model;  // All view-projection-model matrix multiplied
                // Apply inverse model matrix to camera position to be used in point cloud rendering
                cameraPosition = glm::inverse(model) * glm::vec4(cameraPosition, 1.0);
                point_cloud->Draw(vpm, cameraPosition, _timing.deltaTime, *_shader);   // Draw Point Cloud

                if (_scene_config.octree_debug_enabled)     // Debug visualization of the Octree boxes
                    point_cloud->Debug(vpm, cameraPosition);
            }
            
            if (_scene_config.hud_debug_enabled)
                GUIManager::GetInstance()->RenderDebugHudInfo();
    
            // Finish the frame in VR mode or screen mode
            if (_scene_config.vr_enabled)
                _vr_manager_ptr->FinishFrame();
            else
                glfwSwapBuffers(_window);
        }
        else
        {
            // Sleep the CPU when VR session fininshed sleeping (googles off)
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

    }

}





void SceneManager::processInput()
{
    if (glfwGetKey(_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(_window, true);    

    processDebugOption(GLFW_KEY_B, _scene_config.octree_debug_enabled, _keys_state.octree_debug_key);
    processDebugOption(GLFW_KEY_H, _scene_config.hud_debug_enabled, _keys_state.hud_debug_key);


    if (_scene_config.vr_enabled)   // If VR mode delegate Input Processing to VR Manager
    {
        _vr_manager_ptr->ProcessInput(_timing.deltaTime, _scene_config.flying_enabled);
    }
    else    // Otherwise handle keyboard input
    {
        // Get camera pointer
        if (glfwGetKey(_window, GLFW_KEY_W) == GLFW_PRESS)
            _camera_ptr->ProcessKeyboard(Camera::FORWARD, _timing.deltaTime, _scene_config.flying_enabled);
        if (glfwGetKey(_window, GLFW_KEY_S) == GLFW_PRESS)
            _camera_ptr->ProcessKeyboard(Camera::BACKWARD, _timing.deltaTime, _scene_config.flying_enabled);
        if (glfwGetKey(_window, GLFW_KEY_A) == GLFW_PRESS)
            _camera_ptr->ProcessKeyboard(Camera::LEFT, _timing.deltaTime, _scene_config.flying_enabled);
        if (glfwGetKey(_window, GLFW_KEY_D) == GLFW_PRESS)
            _camera_ptr->ProcessKeyboard(Camera::RIGHT, _timing.deltaTime, _scene_config.flying_enabled);

    }

}

void SceneManager::processDebugOption(unsigned int key, bool& option, bool& debug_key)
{ 
    bool debug_key_pressed = glfwGetKey(_window, key) == GLFW_PRESS;
    if (debug_key_pressed && !debug_key)
        option = !option;
    debug_key = debug_key_pressed;
}



// glfw: Whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void SceneManager::framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: Whenever the mouse moves, this callback is called
// -------------------------------------------------------
void SceneManager::mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    
    SceneManager* scene = static_cast<SceneManager*>(glfwGetWindowUserPointer(window));
    if (scene->_screen.firstMouse)
    {
        scene->_screen.lastX = xpos;
        scene->_screen.lastY = ypos;
        scene->_screen.firstMouse = false;
    }

    float xoffset = xpos - scene->_screen.lastX;
    float yoffset = scene->_screen.lastY - ypos; // reversed since y-coordinates go from bottom to top

    scene->_screen.lastX = xpos;
    scene->_screen.lastY = ypos;

    // Get camera pointer
    scene->_camera_ptr->ProcessMouseMovement(xoffset, yoffset);
}
