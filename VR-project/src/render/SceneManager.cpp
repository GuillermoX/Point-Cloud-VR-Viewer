#include <SceneManager.hpp>

SceneManager::SceneManager(unsigned int scr_width, unsigned int scr_height, bool vr_enabled)
{


	// Initialize screen info
	// ----------------------
	_screen.scr_width = scr_width;
	_screen.scr_height = scr_height;
	_screen.lastX = scr_width / 2.0f;
	_screen.lastY = scr_height / 2.0f;
    
    // Initialize scene config
    _scene_config.vr_enabled = vr_enabled;


    // GLFW and GLAD initialization
    // ----------------------------

    // glfw: Initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    if(_scene_config.vr_enabled)
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Requerido por algunos Runtimes VR

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Glfw window creation
    // --------------------

    _window = glfwCreateWindow(scr_width, scr_height, "VR-cloud-point-viewer", NULL, NULL);
    if (_window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        _scene_valid = false;
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
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        _scene_valid = false;
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
        _vr_manager_ptr = std::make_unique<VRManager>(_window);
        // 1. Crear el FBO "cascarón"
        // Create FBO
        glGenFramebuffers(1, &_vr_FBO);

        // 2. Crear un Depth Buffer (Z-Buffer) para que tus puntos se tapen correctamente entre sí en VR
        // El tamaño tiene que ser exactamente el ancho y alto del visor que te dio OpenXR
        // Create Depth Buffer (Z-Buffer), same dimensions as OpenXR visor values
        glGenRenderbuffers(1, &_vr_depth_buffer);
        glBindRenderbuffer(GL_RENDERBUFFER, _vr_depth_buffer);

        // Since using Texture Array (layers per eye), the Depth Buffer also has to be an array
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, 0, GL_DEPTH_COMPONENT24, _vr_manager_ptr->m_visorWidth, _vr_manager_ptr->m_visorHeight);

        // Unbind FBO to avoid interferences
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    else    // ----------- Screen Mode Setup ----------------
    { 
        _camera_ptr = std::make_unique<Camera>(glm::vec3(0.0f, 0.0f, 3.0f));
    }

    // Draw in wireframe
    if (_scene_config.wire_frame_enabled)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    
    glViewport(0, 0, _screen.scr_width, _screen.scr_height);

    // Add this object to GLFW User Pointer to be used in callbacks
    glfwSetWindowUserPointer(_window, this);
}

unsigned char SceneManager::AddShader(Shader& shader, std::string shader_id)
{
    shaders.emplace(shader_id, shader);
    return 0;
}


unsigned char SceneManager::AddModel(Model& model, std::string model_id)
{
    models.emplace(model_id, model);
    return 0;
}

unsigned char SceneManager::AddPointCloud(PointCloud& point_cloud)
{
    this->point_cloud = &point_cloud;
    return 0;
}

void SceneManager::ExecuteRenderLoop()
{
 // ----------- RENDER LOOP ------------
    while (!glfwWindowShouldClose(_window))
    {
        // Per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        _timing.deltaTime = currentFrame - _timing.lastFrame;
        _timing.lastFrame = currentFrame;


        // Always Process Input
        // -----
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

            // Common Render
            // ------
            Shader& currentShader = shaders.at("main_shader");


            // ---------------------- LIGHT UNIFORMS ---------------------------------
            // TODO: Encapsulation of light management
            /*glm::vec3 lightColor(1.0f);

            glm::vec3 diffuseColor = lightColor * glm::vec3(0.5f);
            glm::vec3 ambientColor = diffuseColor * glm::vec3(0.8f);

            glUniform3f(glGetUniformLocation(currentShader.ID, "light.ambient"),
                ambientColor.x, ambientColor.y, ambientColor.z);
            glUniform3f(glGetUniformLocation(currentShader.ID, "light.diffuse"),
                diffuseColor.x, diffuseColor.y, diffuseColor.z);
            glUniform3f(glGetUniformLocation(currentShader.ID, "light.specular"), 1.0f, 1.0f, 1.0f);

            glm::vec3 lightPos = glm::vec3(0.0f, 6.0f, 0.0f);
            glUniform3f(glGetUniformLocation(currentShader.ID, "light.position"),
                lightPos.x, lightPos.y, lightPos.z);

            glUniform1f(glGetUniformLocation(currentShader.ID, "light.constant"), 1.0f);
            glUniform1f(glGetUniformLocation(currentShader.ID, "light.linear"), 0.09f);
            glUniform1f(glGetUniformLocation(currentShader.ID, "light.quadratic"), 0.032f);*/


            // -------- PER EYE RENDER ----------
            int num_eyes_render = 1;    // In Screen mode we render just one eye (screen mode)
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
                if (_scene_config.vr_enabled)
                {
                    cameraPosition = _vr_manager_ptr->GetCameraPosition(eyeIndex);
                    projection = _vr_manager_ptr->GetProjectionMatrix(eyeIndex);
                    view = _vr_manager_ptr->GetViewMatrix(eyeIndex);
                }
                else
                {
                    cameraPosition = _camera_ptr->Position;
                    projection = glm::perspective(glm::radians(_camera_ptr->Zoom), (float)_screen.scr_width / (float)_screen.scr_height, 0.1f, 100.0f);
                    view = _camera_ptr->GetViewMatrix();
                }

                // View Position in world space (camera position)
                //glUniform3f(glGetUniformLocation(currentShader.ID, "viewPos"), cameraPosition.x, cameraPosition.y, cameraPosition.z);

                // View/projection transformations
                glUniformMatrix4fv(glGetUniformLocation(currentShader.ID, "projection"), 1, false, glm::value_ptr(projection));
                glUniformMatrix4fv(glGetUniformLocation(currentShader.ID, "view"), 1, false, glm::value_ptr(view));

                // Render the loaded model
                // -----------------------
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(0.0f, -1.5f, -2.0f)); // translate it down so it's at the center of the scene
                model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));	// it's a bit too big for our scene, so scale it down
                glUniformMatrix4fv(glGetUniformLocation(currentShader.ID, "model"), 1, false, glm::value_ptr(model));
    
                glm::mat4 vpm = projection * view * model;
                point_cloud->Draw(vpm, glm::inverse(model) * glm::vec4(cameraPosition, 1.0));
                //point_cloud->Debug(vpm);
            }

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


SceneManager::~SceneManager()
{
    glfwTerminate();
}

bool SceneManager::SceneIsValid()
{
    return _scene_valid;
}

// Scene configuration
// -------------------
void SceneManager::FlyingMode(bool active)
{
    _scene_config.flying_enabled = active;
}




// Process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void SceneManager::processInput()
{
    if (glfwGetKey(_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(_window, true);


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
