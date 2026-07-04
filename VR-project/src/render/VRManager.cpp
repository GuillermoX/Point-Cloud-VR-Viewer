#include <VRManager.hpp>

VRManager::VRManager(HDC window, HGLRC render_context)
{
	// 1. CREAR LA INSTANCIA (Conectar con el Loader)
    XrInstanceCreateInfo instanceInfo = { XR_TYPE_INSTANCE_CREATE_INFO };
    // (Omitimos rellenar nombres por simplicidad, pero aquí le dices a OpenXR qué extensiones de OpenGL vas a usar)
    xrCreateInstance(&instanceInfo, &m_instance);

    // 2. BUSCAR TUS GAFAS
    XrSystemGetInfo systemInfo = { XR_TYPE_SYSTEM_GET_INFO };
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY; // Buscamos un casco VR
    xrGetSystem(m_instance, &systemInfo, &m_systemId);

    // 3. VINCULAR OPENGL CON OPENXR (¡Aquí ocurre la magia de la conexión!)
    // OpenXR necesita saber cuál es tu contexto de OpenGL para compartir la memoria
    XrGraphicsBindingOpenGLWin32KHR graphicsBinding = { XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR };
    graphicsBinding.hDC = window;   // El Device Context de tu ventana de Windows
    graphicsBinding.hGLRC = render_context; // El Render Context de tu OpenGL
    
    XrSessionCreateInfo sessionInfo = { XR_TYPE_SESSION_CREATE_INFO };
    sessionInfo.next = &graphicsBinding; // Pasamos la info de OpenGL
    sessionInfo.systemId = m_systemId;
    
    xrCreateSession(m_instance, &sessionInfo, &m_session);

    // 4. DEFINIR EL CENTRO DEL MUNDO
    XrReferenceSpaceCreateInfo spaceInfo = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL; // Origen = Donde arranca el casco
    spaceInfo.poseInReferenceSpace.orientation.w = 1.0f; // Rotación neutra
    xrCreateReferenceSpace(m_session, &spaceInfo, &m_appSpace);

    // 5. CREAR LOS LIENZOS (Swapchain)
    // Aquí le pides a OpenXR que te reserve texturas con un formato específico (ej: GL_SRGB8_ALPHA8)
    XrSwapchainCreateInfo swapchainInfo = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
    // ... configuras resolución, formato ...
    xrCreateSwapchain(m_session, &swapchainInfo, &m_swapchain);
}