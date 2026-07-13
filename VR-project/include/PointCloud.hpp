#ifndef LAS_H
#define LAS_H

#include <vector>
#include <glm/glm.hpp>

#include "LASReader.hpp"
#include "Shader.hpp"
#include "Octree.hpp"


class PointCloud
{
public:
	std::vector<point_data_t> points;
	std::unique_ptr<Octree> octree;
	
	PointCloud(const std::string file_path, size_t max_points);
	void Draw();

private:
	unsigned int VAO, VBO;
	float min, max;
	void setupPointCloud();
};



#endif