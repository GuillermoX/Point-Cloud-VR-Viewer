#include <GUIManager.hpp>

GUIManager::GUIManager(GLFWwindow* window) :
    _window{window}
{

    // Debug ImGui lib context creator
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForOpenGL(_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");     // GLSL version
}
/**
 * @brief Renderiza el menú de configuración inicial mediante Dear ImGui.
 * @return true Si el usuario ha pulsado el botón para iniciar el motor.
 */
bool GUIManager::StartMenu(StartingConfig& config)
{
    if (!_window) return false;

    glfwMakeContextCurrent(_window);
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // Restore normal mouse cursor

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
        ImGui::SetNextWindowSize(ImVec2(540, 680), ImGuiCond_Appearing);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

        if (ImGui::Begin("Configuration - PointCloudVRViewer", nullptr, flags))
        {
            ImGui::TextUnformatted("Adjust initial parameters before execution:");
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // -------------------------------------------------------------------------
            // 1. FILE PATH (-c / --cloud-path) [REQUIRED]
            // -------------------------------------------------------------------------
            if (ImGui::CollapsingHeader("Data Loading (-c)", ImGuiTreeNodeFlags_DefaultOpen))
            {
                char path_buffer[512];
                snprintf(path_buffer, sizeof(path_buffer), "%s", config.path.c_str());

                if (ImGui::InputText("File Path (-c / --cloud-path)*", path_buffer, sizeof(path_buffer)))
                {
                    config.path = std::string(path_buffer);
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("[-c, --cloud-path] REQUIRED: Path to point cloud files.");
                }
            }

            ImGui::Spacing();

            // -------------------------------------------------------------------------
            // 2. OCTREE & BINARY CACHE (-b, -p, -r)
            // -------------------------------------------------------------------------
            if (ImGui::CollapsingHeader("Octree Structure (-b, -p, -r)", ImGuiTreeNodeFlags_DefaultOpen))
            {
                // -b / --binary-octree
                ImGui::Checkbox("Create Binary Octree (-b / --binary-octree)", &config.create_octree_bin);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("[-b, --binary-octree] Enables creation of Binary Octree.");
                }

                // DEPENDENCIES: -p and -r require --binary-octree (-b) to be active
                bool disable_octree_deps = !config.create_octree_bin;
                if (disable_octree_deps)
                {
                    ImGui::BeginDisabled();
                }

                // -p / --points
                int max_points_int = static_cast<int>(config.max_points);
                if (ImGui::InputInt("Max Points (-p / --points)", &max_points_int, 100000, 1000000))
                {
                    config.max_points = static_cast<size_t>(std::max(0, max_points_int));
                }
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                {
                    ImGui::SetTooltip("[-p, --points] Max number of points for Binary Octree.\n* Requires '--binary-octree' (-b)");
                }

                // -r / --resolution-box
                int res_int = static_cast<int>(config.node_resolution);
                if (ImGui::SliderInt("Node Resolution (-r / --resolution-box)", &res_int, 8, 512, "%d pts"))
                {
                    config.node_resolution = static_cast<unsigned int>(res_int);
                }
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                {
                    ImGui::SetTooltip("[-r, --resolution-box] Resolution of node box in points per dimension.\n* Requires '--binary-octree' (-b)");
                }

                if (disable_octree_deps)
                {
                    ImGui::EndDisabled();
                }
            }

            ImGui::Spacing();

            // -------------------------------------------------------------------------
            // 3. PERFORMANCE & LOD (-d, -g)
            // -------------------------------------------------------------------------
            if (ImGui::CollapsingHeader("Performance & LOD (-d, -g)", ImGuiTreeNodeFlags_DefaultOpen))
            {
                // -d / --distance
                ImGui::SliderFloat("Max LOD Distance (-d / --distance)", &config.max_lod_distance, 10.0f, 2000.0f, "%.1f m");
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("[-d, --distance] Maximum LOD distance (distance where LOD is minimum).");
                }

                // -g / --gpumem
                ImGui::SliderFloat("GPU Memory Limit (-g / --gpumem)", &config.max_vram_gb, 0.25f, 24.0f, "%.2f GB");
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("[-g, --gpumem] Maximum memory to use by GPU (in GBytes).");
                }
            }

            ImGui::Spacing();

            // -------------------------------------------------------------------------
            // 4. WINDOW & CAMERA MODES (-W, -H, -f, -v)
            // -------------------------------------------------------------------------
            if (ImGui::CollapsingHeader("Window & Navigation Modes (-W, -H, -f, -v)", ImGuiTreeNodeFlags_DefaultOpen))
            {
                // -W / --wwidth and -H / --wheight
                int resolution[2] = { static_cast<int>(config.wwidth), static_cast<int>(config.wheight) };
                if (ImGui::InputInt2("Resolution W x H (-W / -H)", resolution))
                {
                    config.wwidth = static_cast<unsigned int>(std::max(320, resolution[0]));
                    config.wheight = static_cast<unsigned int>(std::max(240, resolution[1]));
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("[-W, --wwidth] Width and [-H, --wheight] Height of window (non-VR mode).");
                }

                ImGui::Spacing();

                // -f / --flying
                ImGui::Checkbox("Flying Mode (-f / --flying)", &config.flying_mode_enabled);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("[-f, --flying] Enable flying camera navigation mode.");
                }

                ImGui::SameLine();

                // -v / --vr
                ImGui::Checkbox("VR Mode (-v / --vr)", &config.vr_mode_enabled);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("[-v, --vr] Enable Virtual Reality Mode.");
                }
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // -------------------------------------------------------------------------
            // 5. START BUTTON (Validation for -c)
            // -------------------------------------------------------------------------
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
                    ImGui::SetTooltip("REQUIRED option '--cloud-path' (-c) is missing. Specify a file path to continue.");
                }
            }
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(_window);
    }

    return start_engine;
}