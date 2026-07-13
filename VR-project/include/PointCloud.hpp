#ifndef LAS_H
#define LAS_H

#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "LASReader.hpp"
#include "Octree.hpp"


class PointCloud
{
public:
	std::vector<point_data_t> points;
	std::unique_ptr<Octree> octree_ptr;
	
	PointCloud(const std::string file_path, size_t max_points);
	void Draw(const glm::mat4& view_proj);

private:
	unsigned int VAO, VBO;
	float min, max;
	void setupPointCloud();
};



#endif