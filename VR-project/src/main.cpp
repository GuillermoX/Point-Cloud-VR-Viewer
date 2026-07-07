//Importante que GLAD se importe antes que otras librerias que usan funciones de OpenGL
#include <glad/glad.h>

#define GLFW_EXPOSE_NATIVE_WIN32 
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <iostream>
#include <cmath>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION

#include <Shader.hpp>
#include <Camera.hpp>
#include <Model.hpp>

#include <VRManager.hpp>

#define VR_ENABLED true
#define WIRE_FRAME_MODE false

// callback functions declaration
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window, Camera& camera);

// settings
unsigned int scr_width = 1920;
unsigned int scr_height = 1080;

// virtual camera object (only in no VR mode)
float lastX = scr_width / 2.0f;     // position first mouse coordinates to screen center
float lastY = scr_height / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;


int main()
{
    // glfw: Initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    if(VR_ENABLED)
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Requerido por algunos Runtimes VR

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Glfw window creation
    // --------------------

    GLFWwindow* window = glfwCreateWindow(scr_width, scr_height, "VR-cloud-point-viewer", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    // Bind functions to glfw callbacks
    if (!VR_ENABLED)
    {
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        glfwSetCursorPosCallback(window, mouse_callback);
        glfwSetScrollCallback(window, scroll_callback);
    }

    // Tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: Load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    // Configure global opengl state
    glEnable(GL_DEPTH_TEST);

    // Build and compile shaders
    Shader ourShader("assets/shaders/shader.vert", "assets/shaders/shader.frag");
    
    // Load Models
    Model ourModel("assets/objects/backpack/backpack.obj");

    
    std::unique_ptr<Camera> camera;
    std::unique_ptr<VRManager> vr_manager;
    // ------------- VR Setup ----------------
    GLuint vr_FBO = 0;
    GLuint vr_depthBuffer = 0;
    if (VR_ENABLED)
    {    
        std::cout << "INFO: ----- VR IS ENABLED -----"  << std::endl;
        std::cout << "GPU en uso por OpenGL: " << glGetString(GL_RENDERER) << std::endl;
        vr_manager = std::make_unique<VRManager>(window);
        // 1. Crear el FBO "cascarón"
        glGenFramebuffers(1, &vr_FBO);

        // 2. Crear un Depth Buffer (Z-Buffer) para que tus puntos se tapen correctamente entre sí en VR
        // El tamaño tiene que ser exactamente el ancho y alto del visor que te dio OpenXR
        glGenRenderbuffers(1, &vr_depthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, vr_depthBuffer);

        // Si estás usando Texture Array (capas para cada ojo), el Depth Buffer también debe ser un Array
        // Si usas Side-by-Side o texturas simples, usarías glRenderbufferStorage
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, 0, GL_DEPTH_COMPONENT24, vr_manager->m_visorWidth, vr_manager->m_visorHeight);

        // 3. Dejarlo desvinculado por ahora para no interferir con el PC
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    else    // ----------- NO VR Setup ----------------
    { 
        camera = std::make_unique<Camera>(glm::vec3(0.0f, 0.0f, 3.0f));
        glfwSetWindowUserPointer(window, camera.get());
    }

    // Draw in wireframe
    if (WIRE_FRAME_MODE)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    
    glViewport(0, 0, scr_width, scr_height);

    // ----------- RENDER LOOP ------------
    while (!glfwWindowShouldClose(window))
    {
        // Per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // VR frame initialization
        // ----------------------
        uint32_t imageIndex;
        GLuint openxrTexture;
        if (VR_ENABLED)
        {
            imageIndex = vr_manager->InitFrame();
            // 1. Obtenemos la textura de OpenGL correspondiente a ese índice
            // (m_swapchainImages es el vector que rellenaste al crear el swapchain)
            openxrTexture = vr_manager->m_swapchainImages[imageIndex].image;
            // 4. ¡Listo! Todo lo que dibujes ahora con tu motor caerá en las gafas
            glViewport(0, 0, vr_manager->m_visorWidth, vr_manager->m_visorHeight);

        }


        // Process Input
        // -----
        processInput(window, *camera);

        // Render
        // ------
        ourShader.use();


        // ---------------------- LIGHT UNIFORMS ---------------------------------
        // TODO: Encapsulation of light management
        glm::vec3 lightColor(1.0f);

        glm::vec3 diffuseColor = lightColor * glm::vec3(0.5f);
        glm::vec3 ambientColor = diffuseColor * glm::vec3(0.8f);

        glUniform3f(glGetUniformLocation(ourShader.ID, "light.ambient"),
            ambientColor.x, ambientColor.y, ambientColor.z);
        glUniform3f(glGetUniformLocation(ourShader.ID, "light.diffuse"),
            diffuseColor.x, diffuseColor.y, diffuseColor.z);
        glUniform3f(glGetUniformLocation(ourShader.ID, "light.specular"), 1.0f, 1.0f, 1.0f);

        glm::vec3 lightPos = glm::vec3(0.0f, 6.0f, 0.0f);
        glUniform3f(glGetUniformLocation(ourShader.ID, "light.position"),
            lightPos.x, lightPos.y, lightPos.z);

        glUniform1f(glGetUniformLocation(ourShader.ID, "light.constant"), 1.0f);
        glUniform1f(glGetUniformLocation(ourShader.ID, "light.linear"), 0.09f);
        glUniform1f(glGetUniformLocation(ourShader.ID, "light.quadratic"), 0.032f);

                
        // -------- PER EYE RENDER ----------
        int num_eyes_render = 1;    // In Screen mode we render just one eye (screen mode)
        if (VR_ENABLED)
            num_eyes_render = 2;    // In VR mode we render two eyes
        for (int eyeIndex = 0; eyeIndex < num_eyes_render; eyeIndex++)
        {
            if (VR_ENABLED)
            {
                // Bind VR FrameBuffer
                glBindFramebuffer(GL_FRAMEBUFFER, vr_FBO);

                // Bind VR Depth FrameBuffer
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, vr_depthBuffer);
   
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
            if (VR_ENABLED)
            {
                cameraPosition = vr_manager->GetCameraPosition(eyeIndex);
                projection = vr_manager->GetProjectionMatrix(eyeIndex);
                view = vr_manager->GetViewMatrix(eyeIndex);
            }
            else
            {
                cameraPosition = camera->Position;
                projection = glm::perspective(glm::radians(camera->Zoom), (float)scr_width / (float)scr_height, 0.1f, 100.0f);
                view = camera->GetViewMatrix();
            }

            // View Position in world space (camera position)
            glUniform3f(glGetUniformLocation(ourShader.ID, "viewPos"), cameraPosition.x, cameraPosition.y, cameraPosition.z);

            // View/projection transformations
            glUniformMatrix4fv(glGetUniformLocation(ourShader.ID, "projection"), 1, false, glm::value_ptr(projection));
            glUniformMatrix4fv(glGetUniformLocation(ourShader.ID, "view"), 1, false, glm::value_ptr(view));

            // Render the loaded model
            // -----------------------
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, -1.0f)); // translate it down so it's at the center of the scene
            model = glm::scale(model, glm::vec3(0.25f, 0.25f, 0.25f));	// it's a bit too big for our scene, so scale it down
            glUniformMatrix4fv(glGetUniformLocation(ourShader.ID, "model"), 1, false, glm::value_ptr(model));

            ourModel.Draw(ourShader);
        }
           
        // Finish the frame in VR mode or screen mode
        if(VR_ENABLED)
            vr_manager->FinishFrame();
        else
            glfwSwapBuffers(window);


        // glfw: Poll IO events (keys pressed/released, mouse moved etc.)
        glfwPollEvents();
    }

    // glfw: Terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// Process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window, Camera& camera)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Get camera pointer
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(Camera::FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(Camera::BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(Camera::LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(Camera::RIGHT, deltaTime);
}

// glfw: Whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: Whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    // Get camera pointer
    Camera* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));
    camera->ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Get camera pointer
    Camera* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));
    camera->ProcessMouseScroll(static_cast<float>(yoffset));
}
