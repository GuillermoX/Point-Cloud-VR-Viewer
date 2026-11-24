<div align="center">

  <img src="https://github.com/GuillermoX/GuillermoX/raw/main/.github/img/PointCloudVRViewer/PointCloudVRViewerBanner.png" alt="Point Cloud VR Viewer" width="1000"/>

  #### A graphics engine developed to visualize massive point cloud data from LiDAR scans using VR headsets.

  [![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)](https://isocpp.org/)
  [![OpenGL](https://img.shields.io/badge/OpenGL-4.5-5586A4?logo=opengl&logoColor=white)](https://www.opengl.org/)
  [![OpenXR](https://img.shields.io/badge/OpenXR-1.1-purple?logo=khronosgroup&logoColor=white)](https://www.khronos.org/openxr/)


</div>

---

## General Description

**Point Cloud VR Viewer** is a graphical tool designed to load and visualize massive point cloud data in `.las` format. The program generates a **binary Octree structure** from raw `.las` files. Once these binary files are generated, the engine loads and streams the Octree structure into memory to render the point cloud efficiently.

<div align="center">
    <p align="center">
    <sub><em>⬇️ Click image to watch the video demo.</em></sub>
  </p>
  <a href="https://www.youtube.com/watch?v=V-fOTT33HaI">
    <img src="https://i.ytimg.com/vi/V-fOTT33HaI/hqdefault.jpg" alt="Haz clic para reproducir" width="600" />
  </a>
</div>


### Main Features
* **VR visualization:** Point cloud rendering via VR headsets with full OpenXR API compatibility.
* **Octree spatial division:** Efficient spatial partitioning of massive point clouds using an Octree structure.
* **Dynamic VRAM Point Streaming:** Out-of-core architecture that dynamically streams points from disk to VRAM based on memory budget limits.
* **Adaptive LOD & Frustum Culling:** Distance-based Level of Detail and camera visibility culling to maximize rendering performance (FPS).
* **Interactive Camera:** Free-look FPS-style camera system (Flycam) for navigation.
<p align="center">
  <img src="https://github.com/GuillermoX/GuillermoX/raw/main/.github/img/PointCloudVRViewer/PointCloudVRViewer_Octree.png" alt="Point Cloud VR Viewer Octree Visualization" width="300"/>
  <br>
  <sub><em>Visualization of Octree structure and LOD (red boxes aren't being rendered)</em></sub>
</p>



---

## Technical information

| Element | Technology / Library |
| :--- | :--- |
| **Language** | [C++20](https://isocpp.org/) |
| **Graphics API** | [OpenGL 4.5+](https://www.opengl.org/) |
| **VR API** | [OpenXR 1.1](https://www.khronos.org/openxr/) |
| **Window and Input** | [GLFW](https://www.glfw.org/) |
| **OpenGL Function Loader**| [GLAD](https://gen.glad.sh/) |
| **3D Math** | [GLM (*OpenGL Mathematics*)](https://glm.g-truc.net/0.9.9/index.html) |
| **GUI** | [ImGui](https://github.com/ocornut/imgui) |
| **CLI** | [CLI11](https://github.com/CLIUtils/CLI11) |


---

## Installation and Compilation

### System Requirements

| Requirement | Minimum | Recommended |
| :--- | :--- | :--- |
| **OS** | Windows 10 | Windows 10/11
| **RAM** | 16 GB | +32 GB |
| **VRAM** | 8 GB | +16 GB |
| **Disk** | 4 MB | 4 MB |


> [!NOTE]
> Point Cloud VR Viewer has been developed exclusively for Windows 10/11 systems due to the lack of OpenXR implementation for iOS/Linux in common VR headsets.

> [!NOTE]
> These disk requirements are exclusively for the **Point Cloud VR Viewer** program. Point cloud `.las` files from LiDAR scans can easily reach tens of gigabytes. Since the engine generates its own binary Octree structure from the raw data, you will need **at least as much free disk space as the size of your `.las` file** to process it (e.g., a 10 GB `.las` file requires at least 10 GB of additional free space).

> [!NOTE]
> VRAM and RAM consumption depends entirely on the total point count of the point cloud. For example, a system with 8 GB of VRAM will smoothly render a 10-million-point dataset, but it may struggle with massive datasets containing 10 billion points without adjusting the memory limits.

### Option 1: Precompiled Executable 
No compilation or Visual Studio installation is required to run the application directly.

1. **Download the latest release:**
   Go to the [Releases](https://github.com/GuillermoX/Point-Cloud-VR-Viewer/releases) section on GitHub and download the latest `PointCloudVRViewer_vX.Y.Z_x64.zip` package.

2. **Extract the archive:**
   Unzip the contents to any folder on your machine.

3. **Run from the command line:**
   Open CMD or PowerShell in the extracted folder and launch the executable with the [required flags](#usage--command-line-arguments).


### Option 2: Self Compilation

#### Prerequisites for Building

* **Visual Studio 2022** (Community, Professional, or Enterprise)
* **C++ Workload:** During Visual Studio installation, make sure to select **Desktop development with C++**.

> [!NOTE]
> No external libraries or SDKs need to be installed manually, as all dependencies are pre-configured in the `external/` directory.

#### Compilation Steps
1. **Clone the repository:**
   ```bash
   git clone https://github.com/GuillermoX/Point-Cloud-VR-Viewer.git
   ```
2. **Open the solution:** Double-click `PointCloudVRViewer.sln` to open the project in **Visual Studio 2022**.
3. **Select configuration:** Set the build configuration to **Release** and platform to **x64** in the top toolbar. 
4. **Compile:** Go to the top menu and click **Build -> Build Solution** *(or press Ctrl + Shift + B)*.

---

## Hardware & Compatibility

This application is built on top of the **OpenXR** standard, making it theoretically compatible with any PC VR headset that supports OpenXR.

* **Primary Target & Tested On:** Meta Quest 3 (via Quest Link, AirLink, or Virtual Desktop / SteamVR).
* **Compatible Headsets (PC VR):** Meta Quest series, Valve Index, HTC Vive series, Pico 4, Varjo, etc.

> [!IMPORTANT]
> For non-Quest headsets, make sure your device's runtime (or SteamVR) is set as the **Active OpenXR Runtime** in your system settings prior to launching the app.

---

## Usage & Configuration

There are two ways to launching the application: from the command line or via the GUI.

### 1. Launching from CLI

The application can be launched directly from the command line (CMD / PowerShell) by passing the path to the binary files along with optional configuration flags.

#### Examples of Syntax

```bash
PointCloudVRViewer.exe --cloud-path path/to/cloud [options]

# Launch with default settings (Desktop mode)
PointCloudVRViewer.exe --cloud-path "C:\Data\LiDAR_Scan"

# Set a 4 GB VRAM budget and launch directly in VR mode
PointCloudVRViewer.exe --cloud-path "C:\Data\LiDAR_Scan" --vr --gpumem 4

# Force the octree binary structure generation
# (if .meta and .points files not found new binary octree files will be automatically generated from .las file)
PointCloudVRViewer.exe --cloud-path "C:\Data\LiDAR_Scan" --binary-octree
```

#### Command-Line Flags & Options

> [!WARNING]
> **On File Paths (`-c / --cloud-path`):**
> Do **not** include the `.las` extension when specifying the cloud path argument.
> 
> * **Correct:** `-c "C:\Data\my_scan"`
> * **Incorrect:** `-c "C:\Data\my_scan.las"`
> 
> **Why?** When you run the engine with `-b / --binary-octree`, it processes `my_scan.las` and generates custom binary files (`my_scan.meta` and `my_scan.points`) in the exact same directory. Once these binary files are generated, the original `.las` file can be safely deleted or moved. The engine expects the base filename so it can automatically bind to the generated `.meta` and `.points` files.

| Option | Flag | Type | Required | Description |
| :--- | :--- | :--- | :--- | :--- |
| **Help** | `-h, --help` | `Flag` | No | Prints the help message and exits the program. |
| **Input Path** | `-c, --cloud-path` | `TEXT` | **Yes** | Base path/filename of the point cloud **without extension** (see note below). |
| **VR Mode** | `-v, --vr` | `Flag` | No | Enables Virtual Reality rendering mode via OpenXR. |
| **Flying Camera** | `-f, --flying` | `Flag` | No | Enables the free-look flying camera navigation mode. |
| **Movement Speed** | `-s, --speed` | `FLOAT` | No | Sets the camera movement speed |
| **Create Binary Octree** | `-b, --binary-octree` | `Flag` | No | Pre-processes the raw `.las` file to generate the binary Octree structure. |
| **Max Points per Node** | `-p, --points` | `UINT` | Conditional | Sets the maximum number of points per Octree node *(Requires `-b / --binary-octree`)*. |
| **Point Size** | `-z, --point-size` | `FLOAT` | No | Sets the point radius size in centimeters. |
| **Box Resolution** | `-r, --resolution-box` | `UINT` | Conditional | Sets the bounding box spatial resolution for Octree nodes *(Requires `-b / --binary-octree`)*. |
| **Max LOD Distance** | `-d, --distance` | `FLOAT` | No | Sets the maximum distance threshold where the lowest Level of Detail (LOD) is applied. |
| **VRAM Limit** | `-g, --gpumem` | `FLOAT` | No | Sets the maximum VRAM budget allowed for point streaming (in Gigabytes). |
| **Window Width** | `-W, --wwidth` | `UINT` | No | Sets the initial window width in pixels (Desktop / non-VR mode). |
| **Window Height** | `-H, --wheight` | `UINT` | No | Sets the initial window height in pixels (Desktop / non-VR mode). |

> [!IMPORTANT]
> **Octree Pre-processing**
> 
> By default, the application checks the specified path for existing pre-processed binary files (`.meta` and `.points`).
> 
> * **First Run / Missing Files:** If `.meta` and `.points` are not found, the viewer will automatically build the Octree structure and generate these binary files from the input `.las` file located at the target path.
> * **Subsequent Runs:** If both files already exist, the application directly streams and renders the pre-processed binaries, skipping the Octree generation phase to ensure instant loading.
> * **Force Rebuilding:** You can bypass the existing cache and force a complete re-generation of the binary Octree files at any time by passing the `--binary-octree` flag.



### 2. Launching via the GUI

The application can be launched in GUI mode by running the `.exe` directly from File Explorer or by executing it without any command-line flags. Once launched, the following configuration menu will appear:

<img src="https://github.com/GuillermoX/GuillermoX/raw/main/.github/img/PointCloudVRViewer/PointCloudVRViewer_ConfigMenu.png" alt="Point Cloud VR Viewer Config Menu" width="700"/>

> [!NOTE]
> This menu provides the same options as the CLI flags. Your last used settings are automatically saved and restored for future runs.

---

## Controls & Navigation

| Action | Desktop Mode | VR Mode (OpenXR) |
| :--- | :--- | :--- |
| **Camera Movement** | `W` / `A` / `S` / `D` keys | Left Thumbstick |
| **Look Around / Orientation** | Move Mouse | Head Movement |


### General Shortcuts & Debug Controls

| Key | Action / Function |
| :---: | :--- |
| <kbd>ESC</kbd> | **Exit Application:** Closes the viewer immediately. |
| <kbd>C</kbd> | **Cursor:** Enables and disables the mouse cursor. |
| <kbd>H</kbd> | **Performance Overlay:** Toggles rendering metrics on screen (FPS & VRAM Usage). |
| <kbd>B</kbd> | **Octree Debug:** Toggles the visualization of the Octree bounding boxes. |
| <kbd>K</kbd> | **Key Information:** Hides and shows the controls keys. |
