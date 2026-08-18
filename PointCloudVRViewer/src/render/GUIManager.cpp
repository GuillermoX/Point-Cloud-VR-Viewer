#include <GUIManager.hpp>

GUIManager::GUIManager(GLFWwindow* window) :
    _window{window}
{

    // Debug ImGui lib context creator
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForOpenGL(_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");     // GLSL version

    ImGuiIO& io = ImGui::GetIO();
    // Load Windows font
    ImFontConfig font_config;
    font_config.PixelSnapH = true; // Align glyphs to integer pixel boundaries
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 16.0f, &font_config);

    applyTheme();
}


bool GUIManager::StartMenu(StartingConfig& config)
{
    if (!_window) return false;

    glfwMakeContextCurrent(_window);
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // Restore normal mouse cursor
    glfwSetWindowAttrib(_window, GLFW_RESIZABLE, GLFW_FALSE);

    bool start_engine = false;

    // Loop ends if "START ENGINE" is clicked or if the window close button ("X") is pressed
    while (!start_engine && !glfwWindowShouldClose(_window))
    {
        glfwPollEvents();

        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(config.wwidth, config.wheight), ImGuiCond_Appearing);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

        if (ImGui::Begin("Point Cloud VR Viewer - Configuration", nullptr, flags))
        {
            ImGui::TextUnformatted("Adjust initial parameters before execution:");
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Data Loading section
            if (ImGui::CollapsingHeader("Data Loading", ImGuiTreeNodeFlags_DefaultOpen))
			{
				char path_buffer[512];
				snprintf(path_buffer, sizeof(path_buffer), "%s", config.path.c_str());

				// Ajustamos el ancho del InputText para dejar espacio al botón "Browse..."
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 105.0f);
				if (ImGui::InputText("##path_input", path_buffer, sizeof(path_buffer)))
				{
					config.path = std::string(path_buffer);
				}
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Path to point cloud file (.las, .meta or .points)");
				}

				ImGui::SameLine();

				// Botón para abrir el explorador de archivos de Windows
				if (ImGui::Button("Browse...", ImVec2(100, 0)))
				{
					std::string selected_path = openFileDialog();
					if (!selected_path.empty())
					{
						config.path = selected_path;
					}
				}
			}
         
            ImGui::Spacing();


            // Binary Octree section
            if (ImGui::CollapsingHeader("Octree Structure", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Create Binary Octree", &config.create_octree_bin);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Enables creation of Binary Octree.");
                }

                bool disable_octree_deps = !config.create_octree_bin;
                if (disable_octree_deps)
                {
                    ImGui::BeginDisabled();
                }
                
				uint64_t step      = 100'000ULL;
		        uint64_t step_fast = 10'000'000ULL;

		        ImGui::InputScalar("Max Points", ImGuiDataType_U64, &config.max_points, &step, &step_fast, "%llu");
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                {
                    ImGui::SetTooltip("Max number of points for Binary Octree.");
                }

                int res_int = static_cast<int>(config.node_resolution);
                if (ImGui::SliderInt("Node Resolution", &res_int, 8, 512, "%d points"))
                {
                    config.node_resolution = static_cast<unsigned int>(res_int);
                }
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                {
                    ImGui::SetTooltip("Resolution of node box in points per dimension.");
                }

                if (disable_octree_deps)
                {
                    ImGui::EndDisabled();
                }
            }

            ImGui::Spacing();

            // Window and camera modes section
            if (ImGui::CollapsingHeader("Window & Navigation Modes", ImGuiTreeNodeFlags_DefaultOpen))
            {
                int resolution[2] = { static_cast<int>(config.wwidth), static_cast<int>(config.wheight) };
                if (ImGui::InputInt2("Resolution (width x height)", resolution))
                {
                    config.wwidth = static_cast<unsigned int>(std::max(320, resolution[0]));
                    config.wheight = static_cast<unsigned int>(std::max(240, resolution[1]));
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Width and Height of window (non-VR mode).");
                }

                ImGui::Spacing();

                ImGui::Checkbox("Flying Mode", &config.flying_mode_enabled);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Enable flying camera navigation mode.");
                }

                ImGui::SameLine();

                ImGui::Checkbox("VR Mode", &config.vr_mode_enabled);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Enable Virtual Reality Mode.");
                }
            }

            ImGui::Spacing();

            // Performance section
            if (ImGui::CollapsingHeader("Performance", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::SliderFloat("Distance where LOD is minimum", &config.max_lod_distance, 10.0f, 800.0f, "%.1f m");
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Maximum LOD distance (distance where LOD is minimum).");
                }

                // VRAM usage
                // Convert current VRAM maximum to number of steps of 0.5 GB
				int steps = static_cast<int>(std::round(config.max_vram_gb * 2.0f));
				steps = std::clamp(steps, 1, 128); // Limit between 1 step (0.5GB) to 128 steps (64.0 GB)

                // Format the visual text that slider will show
				char fmt[32];
				snprintf(fmt, sizeof(fmt), "%.1f GB", steps * 0.5f);

                // SliderInt uses int numbers but transform to float
				if (ImGui::SliderInt("GPU Memory Limit", &steps, 1, 128, fmt))
				{
					config.max_vram_gb = steps * 0.5f;
				}
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Maximum memory to use by GPU (in GBytes).");
                }
            }

            ImGui::Spacing();

            ImGui::Separator();
            ImGui::Spacing();

            bool is_path_empty = config.path.empty();
            if (is_path_empty)
            {
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("START ENGINE", ImVec2(-1, 40)))
            {
                start_engine = true;
            }

            if (is_path_empty)
            {
                ImGui::EndDisabled();
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                {
                    ImGui::SetTooltip("Specify a file path to continue.");
                }
            }
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(_window);
    }


    glfwSetWindowAttrib(_window, GLFW_RESIZABLE, GLFW_TRUE);
    return start_engine;
}

void GUIManager::RenderDebugHudInfo()
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


std::string GUIManager::openFileDialog()
{
    OPENFILENAMEA ofn;
    char szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    // File extension filters
    ofn.lpstrFilter = "Point Cloud Files (*.las;*.points;*.meta)\0*.las;*.points;*.meta\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR; // OFN_NOCHANGEDIR to not break relative paths 

    if (GetOpenFileNameA(&ofn) == TRUE)
    {
        return std::string(ofn.lpstrFile);
    }
    
    return "";
}


void GUIManager::applyTheme()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    style.WindowRounding    = 8.0f;
    style.FrameRounding     = 5.0f;
    style.PopupRounding     = 5.0f;
    style.GrabRounding      = 4.0f;
    style.ScrollbarRounding = 4.0f;

    style.WindowPadding     = ImVec2(8.0f, 8.0f);   
    style.FramePadding      = ImVec2(5.0f, 3.0f);   
    style.ItemSpacing       = ImVec2(6.0f, 4.0f);  
    style.ItemInnerSpacing  = ImVec2(4.0f, 4.0f); 
    style.IndentSpacing     = 15.0f;

	  colors[ImGuiCol_WindowBg]           = ImVec4(0.11f, 0.10f, 0.13f, 1.00f);
	  colors[ImGuiCol_Header]             = ImVec4(0.22f, 0.18f, 0.25f, 1.00f);
	  colors[ImGuiCol_HeaderHovered]      = ImVec4(0.31f, 0.25f, 0.35f, 1.00f);
	  colors[ImGuiCol_HeaderActive]       = ImVec4(0.39f, 0.30f, 0.45f, 1.00f);

	  colors[ImGuiCol_Button]             = ImVec4(0.26f, 0.20f, 0.32f, 1.00f);
	  colors[ImGuiCol_ButtonHovered]      = ImVec4(0.37f, 0.28f, 0.45f, 1.00f);
	  colors[ImGuiCol_ButtonActive]       = ImVec4(0.48f, 0.35f, 0.58f, 1.00f);

	  colors[ImGuiCol_FrameBg]            = ImVec4(0.17f, 0.15f, 0.19f, 1.00f);
	  colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.25f, 0.22f, 0.29f, 1.00f);
	  colors[ImGuiCol_FrameBgActive]      = ImVec4(0.33f, 0.28f, 0.38f, 1.00f);

	  colors[ImGuiCol_TitleBg]            = ImVec4(0.09f, 0.08f, 0.10f, 1.00f);
	  colors[ImGuiCol_TitleBgActive]      = ImVec4(0.14f, 0.12f, 0.16f, 1.00f);

	  colors[ImGuiCol_CheckMark]          = ImVec4(0.75f, 0.40f, 1.00f, 1.00f);
	  colors[ImGuiCol_SliderGrab]         = ImVec4(0.75f, 0.40f, 1.00f, 1.00f);
	  colors[ImGuiCol_SliderGrabActive]   = ImVec4(0.85f, 0.55f, 1.00f, 1.00f);
	  colors[ImGuiCol_Separator]          = ImVec4(0.24f, 0.20f, 0.27f, 1.00f);

}


