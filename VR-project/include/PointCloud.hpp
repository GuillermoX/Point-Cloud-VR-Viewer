#ifndef LAS_H
#define LAS_H

#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <filesystem>

#include "LASReader.hpp"
//#include "Octree.hpp"
#include "OctreeBuilder.hpp"


class PointCloud
{
public:
	std::vector<point_data_t> points;
	//std::unique_ptr<Octree> octree_ptr;
	
	PointCloud(const std::string file_path, size_t max_points, unsigned int grid_res);
	void Draw(const glm::mat4& view_proj, glm::vec3 camera_pos);
	void Debug(const glm::mat4& vpm, glm::vec3 camera_pos);
};



#endif