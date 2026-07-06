#include <VRManager.hpp>

VRManager::VRManager(GLFWwindow* glfwWindow)
{
	// 1. CREAR LA INSTANCIA (Conectar con el Loader)
    XrInstanceCreateInfo instanceInfo = { XR_TYPE_INSTANCE_CREATE_INFO };
    strcpy_s(instanceInfo.applicationInfo.applicationName, XR_MAX_APPLICATION_NAME_SIZE, "MyVREngine");
    instanceInfo.applicationInfo.applicationVersion = 1;

    strcpy_s(instanceInfo.applicationInfo.engineName, XR_MAX_ENGINE_NAME_SIZE, "MyEngine");
    instanceInfo.applicationInfo.engineVersion = 1;

    instanceInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION; // Versión de la API actual
    // (Omitimos rellenar nombres por simplicidad, pero aquí le dices a OpenXR qué extensiones de OpenGL vas a usar)
    const char* extensionesActivas[] = {
        XR_KHR_OPENGL_ENABLE_EXTENSION_NAME
    };
    // Le pasamos el array con nuestra extensión de OpenGL
    instanceInfo.enabledExtensionCount = 1;
    instanceInfo.enabledExtensionNames = extensionesActivas;

    XrResult result = xrCreateInstance(&instanceInfo, &m_instance);
    if (result != XR_SUCCESS) {
        std::cout << "CRÍTICO: La instancia falló con código " << result << ". Abortando." << std::endl;
    }

    // 2. BUSCAR TUS GAFAS
    XrSystemGetInfo systemInfo = { XR_TYPE_SYSTEM_GET_INFO };
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY; // Buscamos un casco VR

    result = xrGetSystem(m_instance, &systemInfo, &m_systemId);
    if (result != XR_SUCCESS) {
        std::cout << "CRÍTICO: xrGetSystem falló con código " << result << std::endl;
    }

    // 3. VINCULAR OPENGL CON OPENXR (¡Aquí ocurre la magia de la conexión!)
    // OpenXR necesita saber cuál es tu contexto de OpenGL para compartir la memoria
    XrGraphicsBindingOpenGLWin32KHR graphicsBinding = { XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR };
    // Extraemos los contextos nativos de Windows justo aquí, garantizando que estén vivos y activos
    HWND hwnd = glfwGetWin32Window(glfwWindow);
    HDC hdc = GetDC(hwnd);
    HGLRC hglrc = glfwGetWGLContext(glfwWindow);
    graphicsBinding.hDC = hdc;   // El Device Context de tu ventana de Windows
    graphicsBinding.hGLRC = hglrc; // El Render Context de tu OpenGL

    // 1. Declarar la variable con su tipo oficial de OpenXR (Añade esta línea)
    PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = nullptr;

    // 2. Pedirle a OpenXR que busque la dirección de la función en el Driver
    XrResult resultadoBusqueda = xrGetInstanceProcAddr(
        m_instance,
        "xrGetOpenGLGraphicsRequirementsKHR",
        (PFN_xrVoidFunction*)(&pfnGetOpenGLGraphicsRequirementsKHR)
    );

    if (resultadoBusqueda != XR_SUCCESS || pfnGetOpenGLGraphicsRequirementsKHR == nullptr) {
        std::cout << "CRÍTICO: No se pudo encontrar la extensión de OpenGL en OpenXR." << std::endl;
    }
    XrGraphicsRequirementsOpenGLKHR graphicsRequirements = { XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR };
    result = pfnGetOpenGLGraphicsRequirementsKHR(m_instance, m_systemId, &graphicsRequirements);

    // ¡AÑADE ESTO PARA DEBUGEAR!
    std::cout << "--- REQUISITOS DE OPENXR ---" << std::endl;
    std::cout << "Versión Mínima de OpenGL requerida: "
        << XR_VERSION_MAJOR(graphicsRequirements.minApiVersionSupported) << "."
        << XR_VERSION_MINOR(graphicsRequirements.minApiVersionSupported) << std::endl;
    std::cout << "Versión Máxima de OpenGL soportada: "
        << XR_VERSION_MAJOR(graphicsRequirements.maxApiVersionSupported) << "."
        << XR_VERSION_MINOR(graphicsRequirements.maxApiVersionSupported) << std::endl;
    
    XrSessionCreateInfo sessionInfo = { XR_TYPE_SESSION_CREATE_INFO };
    sessionInfo.next = &graphicsBinding; // Pasamos la info de OpenGL
    sessionInfo.systemId = m_systemId;
    
    
    result = xrCreateSession(m_instance, &sessionInfo, &m_session);
    if (result != XR_SUCCESS) {
        std::cout << "CRÍTICO: xrCreateSession falló con código " << result << std::endl;
    }

    XrSessionBeginInfo sessionBeginInfo = { XR_TYPE_SESSION_BEGIN_INFO };
    sessionBeginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

    result = xrBeginSession(m_session, &sessionBeginInfo);
    if (result != XR_SUCCESS) {
        std::cout << "CRÍTICO: No se pudo iniciar la sesión: " << result << std::endl;
    }

    // 4. DEFINIR EL CENTRO DEL MUNDO
    XrReferenceSpaceCreateInfo spaceInfo = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL; // Origen = Donde arranca el casco
    spaceInfo.poseInReferenceSpace.orientation.w = 1.0f; // Rotación neutra
    xrCreateReferenceSpace(m_session, &spaceInfo, &m_appSpace);

    // 5. CREAR LOS LIENZOS (Swapchain)
    // Aquí le pides a OpenXR que te reserve texturas con un formato específico (ej: GL_SRGB8_ALPHA8)
    XrSwapchainCreateInfo swapchainInfo = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
    swapchainInfo.next = nullptr;
    swapchainInfo.createFlags = 0;

    // Configurar el uso: le decimos que vamos a pintar en él (Color Attachment)
    swapchainInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_SRC_BIT;

    // FORMATO: Muy importante usar un enum nativo de OpenGL
    swapchainInfo.format = GL_RGBA8; // O simplemente GL_RGBA8 si no usas espacio de color sRGB

        // OBTENER ALTO Y ANCHO DEL VISOR
    // Para VR estándar (Stereo), siempre habrá 2 vistas (ojo izquierdo y derecho)
    uint32_t viewCount = 0;
    xrEnumerateViewConfigurationViews(m_instance, m_systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &viewCount, nullptr);
    // 3. Crear un array/vector para recibir los datos de cada ojo
    std::vector<XrViewConfigurationView> configViews(viewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });

    // 4. Rellenar el array con la información real del hardware
    xrEnumerateViewConfigurationViews(m_instance, m_systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, viewCount, &viewCount, configViews.data());
    // 5. Guardar el ancho y alto del primer ojo (el izquierdo, configViews[0])
    // Ambos ojos tienen exactamente la misma resolución nativa en casi todos los visores
    m_visorWidth = configViews[0].recommendedImageRectWidth;
    m_visorHeight = configViews[0].recommendedImageRectHeight;
    swapchainInfo.width = m_visorWidth;   // (Por ejemplo: 2400)
    swapchainInfo.height = m_visorHeight; // (Por ejemplo: 2200)

    // CONFIGURACIÓN DE VR ESTÁNDAR (Texturas tipo Array para los dos ojos)
    swapchainInfo.sampleCount = 1;  // 1 para evitar problemas de antialiasing por ahora
    swapchainInfo.faceCount = 1;    // Siempre 1 a menos que sea un mapa de cubos (cubemap)
    swapchainInfo.arraySize = 2;    // ¡CRUCIAL! 2 capas: Capa 0 = Ojo Izquierdo, Capa 1 = Ojo Derecho
    swapchainInfo.mipCount = 1;     // Sin mipmaps para las pantallas del visor

    XrResult res = xrCreateSwapchain(m_session, &swapchainInfo, &m_swapchain);
    if (res != XR_SUCCESS) {
        // Si entra aquí, mira qué número da 'res'. 
        // Si da XR_ERROR_GRAPHICS_REQUIREMENTS_CALL_MISSING, revisa el punto 2 de abajo.
        std::cout << "¡El Swapchain ha fallado al crearse! Código: " << res << std::endl;
    }
    // 2. Preguntar a OpenXR cuántas imágenes/texturas ha creado en total (suele crear 3 para hacer Triple Buffering)
    uint32_t imageCount = 0;
    xrEnumerateSwapchainImages(m_swapchain, 0, &imageCount, nullptr);

    // 3. Redimensionar tu vector para que quepan todas
    m_swapchainImages.resize(imageCount, { XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR });

    // 4. Rellenar el vector con los datos reales de OpenGL
    xrEnumerateSwapchainImages(
        m_swapchain,
        imageCount,
        &imageCount,
        reinterpret_cast<XrSwapchainImageBaseHeader*>(m_swapchainImages.data())
    );



   


}


uint32_t VRManager::InitFrame()
{
    // 1. ESPERAR AL VISOR
    // Esto duerme tu hilo de la CPU los milisegundos exactos para que OpenGL dibuje a tiempo.
    XrFrameWaitInfo waitInfo = { XR_TYPE_FRAME_WAIT_INFO };
    frameState = { XR_TYPE_FRAME_STATE };
    xrWaitFrame(m_session, &waitInfo, &frameState);

    // 2. INICIAR EL FRAME
    XrFrameBeginInfo beginInfo = { XR_TYPE_FRAME_BEGIN_INFO };
    XrResult resBegin = xrBeginFrame(m_session, &beginInfo);
    if (resBegin != XR_SUCCESS) {
        std::cout << "ERROR xrBeginFrame: " << resBegin << std::endl;
    }

    // 3. ¿DÓNDE ESTÁ LA CABEZA DEL USUARIO?
    // Le pedimos a OpenXR que nos dé la matriz de proyección y la posición de los 2 ojos
    XrViewLocateInfo viewLocateInfo = { XR_TYPE_VIEW_LOCATE_INFO };
    viewLocateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    viewLocateInfo.displayTime = frameState.predictedDisplayTime;
    viewLocateInfo.space = m_appSpace;

    XrViewState viewState = { XR_TYPE_VIEW_STATE };
    uint32_t viewCount;
    views[0] = XrView{ XR_TYPE_VIEW }; // Ojo Izquierdo y Derecho
    views[1] = XrView{ XR_TYPE_VIEW };
    xrLocateViews(m_session, &viewLocateInfo, &viewState, 2, &viewCount, views);

    // 4. EL LIENZO QUE TOCA PINTAR HOY
    uint32_t imageIndex;
    XrSwapchainImageAcquireInfo acquireInfo = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
    xrAcquireSwapchainImage(m_swapchain, &acquireInfo, &imageIndex);

    XrSwapchainImageWaitInfo waitImageInfo = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
    waitImageInfo.timeout = XR_INFINITE_DURATION;
    xrWaitSwapchainImage(m_swapchain, &waitImageInfo);

    return imageIndex;
}

void VRManager::FinishFrame()
{
    // 5. SOLTAR EL LIENZO
    XrSwapchainImageReleaseInfo releaseInfo = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
    xrReleaseSwapchainImage(m_swapchain, &releaseInfo);

    // 6. CONFIGURAR LAS CAPAS DE PROYECCIÓN (Lo que faltaba)
        // Creamos la info de visualización para cada ojo (Izquierdo = 0, Derecho = 1)
    XrCompositionLayerProjectionView projectionViews[2] = {};
    for (int i = 0; i < 2; i++) {
        projectionViews[i].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
        projectionViews[i].pose = views[i].pose; // Usa el array 'views' que rellenaste en InitFrame
        projectionViews[i].fov = views[i].fov;   // Usa el FOV que obtuviste en InitFrame

        // Le decimos de qué swapchain sacar la imagen
        projectionViews[i].subImage.swapchain = m_swapchain;
        projectionViews[i].subImage.imageRect.offset = { 0, 0 };
        projectionViews[i].subImage.imageRect.extent = { (int32_t)m_visorWidth, (int32_t)m_visorHeight };

        // ¡CRUCIAL!: Le indicamos qué capa del Texture Array corresponde a cada ojo
        projectionViews[i].subImage.imageArrayIndex = i;
    }

    // Unimos los dos ojos en una única capa de proyección principal
    XrCompositionLayerProjection projectionLayer = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
    projectionLayer.space = m_appSpace;
    projectionLayer.viewCount = 2;
    projectionLayer.views = projectionViews;

    // 7. ENVIAR EL FRAME AL CASCO
    XrFrameEndInfo endInfo = { XR_TYPE_FRAME_END_INFO };
    endInfo.displayTime = frameState.predictedDisplayTime; // Usa el frameState de InitFrame
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;

    // Empaquetamos nuestra capa en un array de punteros requerido por OpenXR
    const XrCompositionLayerBaseHeader* layers[] = { (XrCompositionLayerBaseHeader*)&projectionLayer };
    endInfo.layerCount = 1;
    endInfo.layers = layers; // <-- ¡Ahora sí le pasamos las imágenes!

    XrResult resEnd = xrEndFrame(m_session, &endInfo);
    if (resEnd != XR_SUCCESS) {
        std::cout << "ERROR xrEndFrame: " << resEnd << std::endl;
    }
}

glm::vec3 VRManager::GetCameraPosition(int eyeIndex)
{
    XrVector3f openxrPos = views[eyeIndex].pose.position;
    return glm::vec3(openxrPos.x, openxrPos.y, openxrPos.z);
}

glm::mat4 VRManager::GetViewMatrix(int eyeIndex)
{
    XrQuaternionf openxrQuat = views[eyeIndex].pose.orientation;
    XrVector3f    openxrPos = views[eyeIndex].pose.position;

    // 1. Convertimos a tipos GLM (¡Ojo al orden del cuaternión en GLM: w, x, y, z!)
    glm::quat rotation = glm::quat(openxrQuat.w, openxrQuat.x, openxrQuat.y, openxrQuat.z);
    glm::vec3 position = glm::vec3(openxrPos.x, openxrPos.y, openxrPos.z);

    // 2. Construimos la matriz de transformación del objeto "Cámara" (Rotación * Traslación)
    glm::mat4 cameraTransform = glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation);

    // 3. La Matriz de Vista es la inversa matemática de la transformación de la cámara
    return glm::inverse(cameraTransform);
}

glm::mat4 VRManager::GetProjectionMatrix(int eyeIndex)
{
    XrFovf fov = views[eyeIndex].fov;

    // Calculamos los límites del frustum de la lente
    float tanLeft = tanf(fov.angleLeft);
    float tanRight = tanf(fov.angleRight);
    float tanUp = tanf(fov.angleUp);
    float tanDown = tanf(fov.angleDown);

    // Define tus planos de corte (cerca y lejos en metros)
    float nearZ = 0.05f;
    float farZ = 500.0f;

    // Creamos la matriz de proyección con GLM
    return glm::frustum(
        tanLeft * nearZ,   // Izquierda
        tanRight * nearZ,  // Derecha
        tanDown * nearZ,   // Abajo
        tanUp * nearZ,     // Arriba
        nearZ,             // Near
        farZ               // Far
    );
}

