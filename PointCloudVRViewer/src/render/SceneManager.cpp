#include <SceneManager.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#include <chrono>
#include <thread>
#pragma comment(lib, "winmm.lib")

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
    
    // Add this object to GLFW User Pointer to be used in callbacks
    glfwSetWindowUserPointer(_window, this);



}

SceneManager::~SceneManager()
{
    glfwTerminate();
}


void SceneManager::InitCamera()
{

    if (_scene_config.vr_enabled)
    {    
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Needed by some VR runtimes

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

        glfwSwapInterval(0);    // Disable V-Sync to avoid glfwSwapBuffer from blocking CPU
    }
    else    // Screen Mode Setup 
    { 
        _camera_ptr = std::make_unique<Camera>(glm::vec3(0.0f, 0.0f, 3.0f), _scene_config.movement_speed);    // Create non VR Camera object

        // Bind functions to glfw callbacks
        glfwSetFramebufferSizeCallback(_window, framebuffer_size_callback);
        glViewport(0, 0, _screen.scr_width, _screen.scr_height);
    }
 
    
}


void SceneManager::ChangeWindowResolution(unsigned int width, unsigned int height)
{
    glfwSetWindowSize(_window, width, height);
    _screen.scr_height = height;
    _screen.scr_width = width;
}

void SceneManager::SetBackgroundColor(float color[3])
{
    _scene_config.bg_color[0] = color[0];
    _scene_config.bg_color[1] = color[1];
    _scene_config.bg_color[2] = color[2];
}

void SceneManager::SetMovementSpeed(float speed)
{
    if (_scene_config.vr_enabled)
        _vr_manager_ptr->movement_speed = speed;
    else
        _camera_ptr->MovementSpeed = speed;
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
    timeBeginPeriod(1);     // Force Windows to work with time precision of 1.0 ms instead of 15.6 ms

    // Tell GLFW to capture our mouse
    if (!_scene_config.vr_enabled)
    {
        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetCursorPosCallback(_window, mouse_callback);  // Only activate callback function once the engine is started (not in menu)
    }
    else
        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

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

        auto frame_start = std::chrono::high_resolution_clock::now();    // Time at start of frame


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
                glClearColor(_scene_config.bg_color[0], _scene_config.bg_color[1], _scene_config.bg_color[2], 1.0f);
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
                    projection = glm::perspective(glm::radians(_camera_ptr->Zoom), (float)_screen.scr_width / (float)_screen.scr_height, 0.1f, MAX_LOD_DISTANCE*1.5f);
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
            


    
            // Finish the frame in VR mode or screen mode
            if (_scene_config.vr_enabled)
                _vr_manager_ptr->FinishFrame();
            else
            {
				// HUD Rendering
                GUIManager* gui_manager = GUIManager::GetInstance();
				gui_manager->InitFrame();

				if (_scene_config.hud_debug_enabled)
					gui_manager->DrawDebugHudInfo();

				gui_manager->DrawInfoMenu(_scene_config.hud_info_enabled);
				gui_manager->Render();
                
                // Sleep thread to limit FPS
				auto frame_end = std::chrono::high_resolution_clock::now();         // Time at the end of frame
				std::chrono::duration<double> elapsed = frame_end - frame_start;    // Elapsed time in frame

				if (elapsed < _timing.frame_target_time) {  // If elapsed time smaller than target time
					std::this_thread::sleep_for(std::chrono::duration<double>(_timing.frame_target_time - elapsed));
				} 
            }


            glfwSwapBuffers(_window);
        }
        else
        {
            // Sleep the CPU when VR session fininshed sleeping (googles off)
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

    }
    timeEndPeriod(1);   // Restore windows defaults time precision

}





void SceneManager::processInput()
{

    // General input keys
    if (glfwGetKey(_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(_window, true);    
    processKeyOption(GLFW_KEY_B, _scene_config.octree_debug_enabled, _keys_state.octree_debug_key);
    processKeyOption(GLFW_KEY_H, _scene_config.hud_debug_enabled, _keys_state.hud_debug_key);
    processKeyOption(GLFW_KEY_K, _scene_config.hud_info_enabled, _keys_state.hud_info_key);



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

        // Cursor state handling
		bool cursor_key_pressed = glfwGetKey(_window, GLFW_KEY_C) == GLFW_PRESS;
		if (cursor_key_pressed && !_keys_state.cursor_enabled_key)
		{
			_screen.cursor_enabled = !_screen.cursor_enabled;
			setCursor(_screen.cursor_enabled);
		}
		_keys_state.cursor_enabled_key = cursor_key_pressed;

    }

}

void SceneManager::processKeyOption(unsigned int key, bool& option, bool& debug_key)
{ 
    bool debug_key_pressed = glfwGetKey(_window, key) == GLFW_PRESS;
    if (debug_key_pressed && !debug_key)
        option = !option;
    debug_key = debug_key_pressed;
}


void SceneManager::setCursor(bool enable)
{
    if (enable)
    {
        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        _screen.firstMouse = true;
        glfwSetCursorPosCallback(_window, NULL);  // Only activate callback function once the engine is started (not in menu)
    }
    else
    {
        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetCursorPosCallback(_window, mouse_callback);  // Only activate callback function once the engine is started (not in menu)
    }

}


// glfw: Whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void SceneManager::framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
    SceneManager* scene = static_cast<SceneManager*>(glfwGetWindowUserPointer(window));
    scene->_screen.scr_width = width;
    scene->_screen.scr_height = height;

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
