#ifndef LAS_H
#define LAS_H

#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <filesystem>

#include "LASReader.hpp"
#include "Octree.hpp"
#include "OctreeBuilder.hpp"


class PointCloud
{
public:
	std::unique_ptr<Octree> octree_ptr;		// Octree structure
	
	/**
	 * @brief PointCloud constructor
	 * @details It creates the point cloud structure needed to be rendered
	 * @param file_path: path where the needed files are located
	 *					 (no extension, just the name the .las, .meta, .points binaries use)
	 * @param grid_res: Resolution of one octree node (max number of points is grid_res^3)
	 * @param create_bin: It forces to create the octree binary from .las file
	 * @param max_points_bin: Maximum number of points to load from .las if binary octree files are created
	 * @param[out]: It tells if the point cloud was created successfully
	 */
	PointCloud(const std::string file_path, unsigned int grid_res, bool create_bin, size_t max_points_bin, bool& point_cloud_created);

	/**
	 * @brief Draws the point cloud
	 * @param view_proj: Multiplication of view and projection matrices used to render the scene
	 * @param camera_pos: Position of the camera
	 * @param delta_time: Delta Time of current frame
	 * @param shader: Shader used to render point cloud (used to change uniform values)
	 */
	void Draw(const glm::mat4& view_proj, glm::vec3 camera_pos, float delta_time, Shader& shader);

	/**
	 * @brief Draws the debug octree box information
	 * @param vpm: Multiplication of view and projection matrices used to render the scene
	 * @param camera_pos: Position of the camera
	 */
	void Debug(const glm::mat4& vpm, glm::vec3 camera_pos);
};



#endif