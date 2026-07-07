#include <VRManager.hpp>

VRManager::VRManager(GLFWwindow* glfwWindow)
{
    // Create Instance (Connect to Loader)
    // -----------------------------------
    XrInstanceCreateInfo instanceInfo = { XR_TYPE_INSTANCE_CREATE_INFO };
    strcpy_s(instanceInfo.applicationInfo.applicationName, XR_MAX_APPLICATION_NAME_SIZE, "ScanDraw VR Viewer");
    instanceInfo.applicationInfo.applicationVersion = 1;

    strcpy_s(instanceInfo.applicationInfo.engineName, XR_MAX_ENGINE_NAME_SIZE, "ScanDraw VR Engine");
    instanceInfo.applicationInfo.engineVersion = 1;
    
    // Specify Graphics API
    instanceInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION; // Current API Version
    const char* extensionesActivas[] = {
        XR_KHR_OPENGL_ENABLE_EXTENSION_NAME     
    };
    instanceInfo.enabledExtensionCount = 1;
    instanceInfo.enabledExtensionNames = extensionesActivas;

    XrResult result = xrCreateInstance(&instanceInfo, &m_instance);
    if (result != XR_SUCCESS) {
        std::cout << "ERROR: Instance failed with error code " << result << std::endl;
    }

    // Find VR Visor
    // -------------
    XrSystemGetInfo systemInfo = { XR_TYPE_SYSTEM_GET_INFO };
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY; // Find VR goggles

    result = xrGetSystem(m_instance, &systemInfo, &m_systemId);
    if (result != XR_SUCCESS) {
        std::cout << "ERROR: xrGetSystem failed with code " << result << std::endl;
    }

    // Bind OpenGL with OpenXR
    // -----------------------
    // OpenXR needs to know the OpenGL context being used to share memory
    XrGraphicsBindingOpenGLWin32KHR graphicsBinding = { XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR };
    graphicsBinding.hDC = GetDC(glfwGetWin32Window(glfwWindow));   // El Device Context de tu ventana de Windows
    graphicsBinding.hGLRC = glfwGetWGLContext(glfwWindow); // El Render Context de tu OpenGL
    
    // Debugging info 
    PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = nullptr;

    XrResult resultadoBusqueda = xrGetInstanceProcAddr(
        m_instance,
        "xrGetOpenGLGraphicsRequirementsKHR",
        (PFN_xrVoidFunction*)(&pfnGetOpenGLGraphicsRequirementsKHR)
    );

    if (resultadoBusqueda != XR_SUCCESS || pfnGetOpenGLGraphicsRequirementsKHR == nullptr) {
        std::cout << "CRÍTICO: No se pudo encontrar la extensión de OpenGL en OpenXR." << std::endl;
    }
    XrGraphicsRequirementsOpenGLKHR graphicsRequirements = { XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR };
    pfnGetOpenGLGraphicsRequirementsKHR(m_instance, m_systemId, &graphicsRequirements);    //Get graphics requirements
    
    // Debugging info
    std::cout << "--- OPENXR REQUIREMENTS ---" << std::endl;
    std::cout << "Minimum OpenGL version needed: "
        << XR_VERSION_MAJOR(graphicsRequirements.minApiVersionSupported) << "."
        << XR_VERSION_MINOR(graphicsRequirements.minApiVersionSupported) << std::endl;

    std::cout << "Maximum OpenGL version allowed: "
        << XR_VERSION_MAJOR(graphicsRequirements.maxApiVersionSupported) << "."
        << XR_VERSION_MINOR(graphicsRequirements.maxApiVersionSupported) << std::endl;
    
    // Create and Begin Session 
    // ------------------------
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

    // Define World Center
    // -------------------
    XrReferenceSpaceCreateInfo spaceInfo = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL; // Set the origin where the VR googles start running
    spaceInfo.poseInReferenceSpace.orientation.w = 1.0f; //Neutral Rotation
    xrCreateReferenceSpace(m_session, &spaceInfo, &m_appSpace);
    
    // Create Swapchain
    // ----------------
    XrSwapchainCreateInfo swapchainInfo = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
    swapchainInfo.next = nullptr;
    swapchainInfo.createFlags = 0;

    // Config usage: in this case we are going to draw on it
    swapchainInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_SRC_BIT;
    
    // OpenGL format we are using to draw
    swapchainInfo.format = GL_RGBA8;


    // Get High and Weight of Visor Lenses
    // Get all views (VR standart uses 2 views)
    uint32_t viewCount = 0;
    xrEnumerateViewConfigurationViews(m_instance, m_systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &viewCount, nullptr);
    // Create vector to store data from each eye
    std::vector<XrViewConfigurationView> configViews(viewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
    // Get view data
    xrEnumerateViewConfigurationViews(m_instance, m_systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, viewCount, &viewCount, configViews.data());

    // Store width and height of one eye (both eyes should have same resolution in most VR goggles)
    m_visorWidth = configViews[0].recommendedImageRectWidth;
    m_visorHeight = configViews[0].recommendedImageRectHeight;
    swapchainInfo.width = m_visorWidth;
    swapchainInfo.height = m_visorHeight;

    // VR Standard Configuration
    swapchainInfo.sampleCount = 1;  // 1 to avoid antialiasing problems 
    swapchainInfo.faceCount = 1;    // always 1 unless it's a cube map.
    swapchainInfo.arraySize = 2;    // 2 layers: 0 = left eye, 1 = right eye
    swapchainInfo.mipCount = 1;     // No mipmaps
    
    // Create Swapchain info
    XrResult res = xrCreateSwapchain(m_session, &swapchainInfo, &m_swapchain);
    if (res != XR_SUCCESS) {
        std::cout << "ERROR: Failed creating Swapchain, error code: " << res << std::endl;
    }

    // Get number of images/textures created in total (usually 3 textures)
    uint32_t imageCount = 0;
    xrEnumerateSwapchainImages(m_swapchain, 0, &imageCount, nullptr);
    
    // Resize vector to store all textures
    m_swapchainImages.resize(imageCount, { XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR });

    // Fill the vector of textures with correct OpenGL data
    xrEnumerateSwapchainImages(
        m_swapchain,
        imageCount,
        &imageCount,
        reinterpret_cast<XrSwapchainImageBaseHeader*>(m_swapchainImages.data())
    );



   


}


uint32_t VRManager::InitFrame()
{
    // Wait for visor (sleeps CPU until OpenGL can draw)
    // -------------------------------------------------
    XrFrameWaitInfo waitInfo = { XR_TYPE_FRAME_WAIT_INFO };
    frameState = { XR_TYPE_FRAME_STATE };
    xrWaitFrame(m_session, &waitInfo, &frameState);

    // Start frame
    // -----------
    XrFrameBeginInfo beginInfo = { XR_TYPE_FRAME_BEGIN_INFO };
    XrResult resBegin = xrBeginFrame(m_session, &beginInfo);
    if (resBegin != XR_SUCCESS) {
        std::cout << "ERROR xrBeginFrame: " << resBegin << std::endl;
    }

    // Get views info (position, projection, etc)
    // ------------------------------------------
    XrViewLocateInfo viewLocateInfo = { XR_TYPE_VIEW_LOCATE_INFO };
    viewLocateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    viewLocateInfo.displayTime = frameState.predictedDisplayTime;
    viewLocateInfo.space = m_appSpace;

    XrViewState viewState = { XR_TYPE_VIEW_STATE };
    uint32_t viewCount;
    views[0] = XrView{ XR_TYPE_VIEW }; // Left eye
    views[1] = XrView{ XR_TYPE_VIEW }; // Right eye
    xrLocateViews(m_session, &viewLocateInfo, &viewState, 2, &viewCount, views);

    // Get texture to draw in this frame (triple buffering)
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
    // Release texture
    XrSwapchainImageReleaseInfo releaseInfo = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
    xrReleaseSwapchainImage(m_swapchain, &releaseInfo);

    // Config projection layers for each eye (0 = left eye, 1 = right eye)
    XrCompositionLayerProjectionView projectionViews[2] = {};
    for (int i = 0; i < 2; i++) {
        projectionViews[i].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
        projectionViews[i].pose = views[i].pose; // Use pose used in InitFrame function
        projectionViews[i].fov = views[i].fov;   // Use FOV used in InitFrame function

        // Specify the swapchain where to get the image/texture
        projectionViews[i].subImage.swapchain = m_swapchain;
        projectionViews[i].subImage.imageRect.offset = { 0, 0 };
        projectionViews[i].subImage.imageRect.extent = { (int32_t)m_visorWidth, (int32_t)m_visorHeight };

        // Important! Specify which layer index belongs to each eye
        projectionViews[i].subImage.imageArrayIndex = i;
    }

    // Combine both eyes in one unique projection layer
    XrCompositionLayerProjection projectionLayer = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
    projectionLayer.space = m_appSpace;
    projectionLayer.viewCount = 2;
    projectionLayer.views = projectionViews;
    
    // Send frame to the VR goggles
    XrFrameEndInfo endInfo = { XR_TYPE_FRAME_END_INFO };
    endInfo.displayTime = frameState.predictedDisplayTime; // Use frameState of InitFrame
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;

    // Pack this layer in a pointer array needed by OpenXR
    const XrCompositionLayerBaseHeader* layers[] = { (XrCompositionLayerBaseHeader*)&projectionLayer };
    endInfo.layerCount = 1;
    endInfo.layers = layers;

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

    // Convert types to GLM (order of quaternion in GLM -> w, x, y, z)
    glm::quat rotation = glm::quat(openxrQuat.w, openxrQuat.x, openxrQuat.y, openxrQuat.z);
    glm::vec3 position = glm::vec3(openxrPos.x, openxrPos.y, openxrPos.z);

    // Build the transformation matrix of the camera (rotation * translation)
    glm::mat4 cameraTransform = glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation);

    // View matrix is the mathematical inverse of the camera transformation
    return glm::inverse(cameraTransform);
}

glm::mat4 VRManager::GetProjectionMatrix(int eyeIndex)
{
    XrFovf fov = views[eyeIndex].fov;

    // Calculate limits of lense frustum
    float tanLeft = tanf(fov.angleLeft);
    float tanRight = tanf(fov.angleRight);
    float tanUp = tanf(fov.angleUp);
    float tanDown = tanf(fov.angleDown);

    // Define tus planos de corte (cerca y lejos en metros)
    // Define the cutting Z planes (near and far in meters units)
    float nearZ = 0.05f;
    float farZ = 500.0f;

    // Creat projection matrix in GLM
    return glm::frustum(
        tanLeft * nearZ,   // Left
        tanRight * nearZ,  // Right
        tanDown * nearZ,   // Down
        tanUp * nearZ,     // Top
        nearZ,             // Near
        farZ               // Far
    );
}

