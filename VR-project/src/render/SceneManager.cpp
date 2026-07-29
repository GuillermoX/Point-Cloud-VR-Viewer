#include <SceneManager.hpp>

SceneManager::SceneManager(unsigned int scr_width, unsigned int scr_height, bool vr_enabled)
{
	// Initialize screen info
	_screen.scr_width = scr_width;
	_screen.scr_height = scr_height;
	_screen.lastX = scr_width / 2.0f;
	_screen.lastY = scr_height / 2.0f;
    
    // Initialize scene config
    _scene_config.vr_enabled = vr_enabled;


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
    _window = glfwCreateWindow(scr_width, scr_height, "VR-cloud-point-viewer", NULL, NULL);
    if (_window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
    }
    glfwMakeContextCurrent(_window);
    
    // Bind functions to glfw callbacks
    if (!_scene_config.vr_enabled)
    {
        glfwSetFramebufferSizeCallback(_window, framebuffer_size_callback);
        glfwSetCursorPosCallback(_window, mouse_callback);
        glfwSetScrollCallback(_window, scroll_callback);
    }

    // Tell GLFW to capture our mouse
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: Load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
    }

    // Tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

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


    // Debug ImGui lib context creator
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForOpenGL(_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");     // GLSL version
}

SceneManager::~SceneManager()
{
    glfwTerminate();
}

void SceneManager::AddShader(Shader& shader, std::string shader_id)
{
    shaders.emplace(shader_id, shader);
}


void SceneManager::AddModel(Model& model, std::string model_id)
{
    models.emplace(model_id, model);
}

void SceneManager::AddPointCloud(PointCloud& point_cloud)
{
    this->point_cloud = &point_cloud;
}

void SceneManager::ExecuteRenderLoop()
{
    Shader& currentShader = shaders.at("point_cloud");

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
                // Get OpenGL texture from swap chain
                openxrTexture = _vr_manager_ptr->m_swapchainImages[imageIndex].image;
                glViewport(0, 0, _vr_manager_ptr->m_visorWidth, _vr_manager_ptr->m_visorHeight);

            }

            // -------- PER EYE RENDER ----------
            int num_eyes_render = 1;    // In Screen mode we render just one eye
            if (_scene_config.vr_enabled)
                num_eyes_render = 2;    // In VR mode we render two eyes
            for (int eyeIndex = 0; eyeIndex < num_eyes_render; eyeIndex++)
            {
                currentShader.use();
				if(_scene_config.vr_enabled) 
					glUniform1f(glGetUniformLocation(currentShader.ID, "screenHeight"), _vr_manager_ptr->m_visorHeight);
				else
					glUniform1f(glGetUniformLocation(currentShader.ID, "screenHeight"), _screen.scr_height);

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
                    projection = glm::perspective(glm::radians(_camera_ptr->Zoom), (float)_screen.scr_width / (float)_screen.scr_height, 0.1f, 100.0f);
                    view = _camera_ptr->GetViewMatrix();
                }

                // View/projection transformations
                glUniformMatrix4fv(glGetUniformLocation(currentShader.ID, "projection"), 1, false, glm::value_ptr(projection));
                glUniformMatrix4fv(glGetUniformLocation(currentShader.ID, "view"), 1, false, glm::value_ptr(view));
                
                // Model transformations
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(0.0f, -1.5f, -2.0f)); // Translate in world space
                model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));	// Scale the point cloud
                glUniformMatrix4fv(glGetUniformLocation(currentShader.ID, "model"), 1, false, glm::value_ptr(model));
    
                glm::mat4 vpm = projection * view * model;  // All view-projection-model matrix multiplied
                // Apply inverse model matrix to camera position to be used in point cloud rendering
                cameraPosition = glm::inverse(model) * glm::vec4(cameraPosition, 1.0);
                point_cloud->Draw(vpm, cameraPosition, _timing.deltaTime, currentShader);   // Draw Point Cloud

                if (_scene_config.octree_debug_enabled)     // Debug visualization of the Octree boxes
                    point_cloud->Debug(vpm, cameraPosition);
            }
            
            if (_scene_config.hud_debug_enabled)
                renderDebugHudInfo();
    
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


// Scene configuration
// -------------------
void SceneManager::FlyingMode(bool active)
{
    _scene_config.flying_enabled = active;
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

void SceneManager::renderDebugHudInfo()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.6f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | 
							 ImGuiWindowFlags_AlwaysAutoResize | 
							 ImGuiWindowFlags_NoMove | 
							 ImGuiWindowFlags_NoSavedSettings | 
							 ImGuiWindowFlags_NoFocusOnAppearing | 
							 ImGuiWindowFlags_NoNav;

	ImGui::Begin("HUD", nullptr, flags);
    
    float fps = ImGui::GetIO().Framerate;
    if (fps < 35.0) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
    else if (fps < 55.0) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
    else ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
	ImGui::Text("FPS: %.1f", fps);
    ImGui::PopStyleColor();

    VRAMManager& vram_manager = VRAMManager::GetInstance();
    unsigned int current = vram_manager.GetCurrentMB();
    unsigned int max = vram_manager.GetMaxMB();
    float usage = ((float)current / (float)max) * 100.0f;
    if (usage < 50.0) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
    else if (usage < 80.0) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
    else ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
	ImGui::Text("VRAM: %u/%u MB | %.1f%%", current, max, usage);
    ImGui::PopStyleColor();


    ImGui::End();

	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(1);

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void SceneManager::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Get camera pointer
    Camera* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));
    camera->ProcessMouseScroll(static_cast<float>(yoffset));
}
