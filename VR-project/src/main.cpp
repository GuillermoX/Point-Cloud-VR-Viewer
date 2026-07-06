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

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;


int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Requerido por algunos Runtimes VR

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader ourShader("assets/shaders/shader.vert", "assets/shaders/shader.frag");

    // load models
    // -----------
    Model ourModel("assets/objects/backpack/backpack.obj");


    // VR setup
    std::cout << "GPU en uso por OpenGL: " << glGetString(GL_RENDERER) << std::endl;
    VRManager vr_manager = VRManager(window);
    GLuint vr_FBO = 0;
    GLuint vr_depthBuffer = 0;
    // 1. Crear el FBO "cascarón"
    glGenFramebuffers(1, &vr_FBO);

    // 2. Crear un Depth Buffer (Z-Buffer) para que tus puntos se tapen correctamente entre sí en VR
    // El tamaño tiene que ser exactamente el ancho y alto del visor que te dio OpenXR
    glGenRenderbuffers(1, &vr_depthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, vr_depthBuffer);

    // Si estás usando Texture Array (capas para cada ojo), el Depth Buffer también debe ser un Array
    // Si usas Side-by-Side o texturas simples, usarías glRenderbufferStorage
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 0, GL_DEPTH_COMPONENT24, vr_manager.m_visorWidth, vr_manager.m_visorHeight);

    // 3. Dejarlo desvinculado por ahora para no interferir con el PC
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // VR frame initialization
        // ----------------------
        uint32_t imageIndex = vr_manager.InitFrame();
        // 1. Obtenemos la textura de OpenGL correspondiente a ese índice
        // (m_swapchainImages es el vector que rellenaste al crear el swapchain)
        GLuint openxrTexture = vr_manager.m_swapchainImages[imageIndex].image;

        

        
        // Si usas un formato de textura normal (no Array), usarías:
        // glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, openxrTexture, 0);

        // 4. ¡Listo! Todo lo que dibujes ahora con tu motor caerá en las gafas
        glViewport(0, 0, vr_manager.m_visorWidth, vr_manager.m_visorHeight);


        // input
        // -----
        processInput(window);

        // render
        // ------

        // don't forget to enable shader before setting uniforms
        ourShader.use();


        //Light uniforms
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

        // Render per eye
        for (int eyeIndex = 0; eyeIndex < 2; eyeIndex++)
        {
            // 2. Vinculamos TU Framebuffer (el FBO que creaste en tu motor)
            glBindFramebuffer(GL_FRAMEBUFFER, vr_FBO);
            // 3. Vincular también nuestro buffer de profundidad (para el Z-test)
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, vr_depthBuffer);
   
            // 3. Acoplamos la textura de OpenXR a nuestro FBO.
            // Si configuraste OpenXR como un Texture Array (capas para cada ojo):
            // En el bucle de los ojos, usas 'ojoIndex' (0 para izquierdo, 1 para derecho)
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, openxrTexture, 0, eyeIndex);

            //Clean screen for this eye
            glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glm::vec3 cameraPosition = vr_manager.GetCameraPosition(eyeIndex);
            // View Position
            glUniform3f(glGetUniformLocation(ourShader.ID, "viewPos"), cameraPosition.x, cameraPosition.y, cameraPosition.z);

            // view/projection transformations
            glm::mat4 projection = vr_manager.GetProjectionMatrix(eyeIndex);
            glm::mat4 view = vr_manager.GetViewMatrix(eyeIndex);
            glUniformMatrix4fv(glGetUniformLocation(ourShader.ID, "projection"), 1, false, glm::value_ptr(projection));
            glUniformMatrix4fv(glGetUniformLocation(ourShader.ID, "view"), 1, false, glm::value_ptr(view));

            // render the loaded model
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, -1.0f)); // translate it down so it's at the center of the scene
            model = glm::scale(model, glm::vec3(0.25f, 0.25f, 0.25f));	// it's a bit too big for our scene, so scale it down
            glUniformMatrix4fv(glGetUniformLocation(ourShader.ID, "model"), 1, false, glm::value_ptr(model));
            ourModel.Draw(ourShader);
        }

        vr_manager.FinishFrame();


        // glfw: poll IO events (keys pressed/released, mouse moved etc.)
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(Camera::FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(Camera::BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(Camera::LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(Camera::RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
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

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
