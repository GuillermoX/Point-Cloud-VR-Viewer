#ifndef LAS_H
#define LAS_H

#include <vector>
#include "LASReader.hpp"
#include "Shader.hpp"

class PointCloud
{
public:
	std::vector<point_data_t> points;
	
	PointCloud(const std::string file_path, size_t max_points = 0);
	void Draw();

private:
	unsigned int VAO, VBO;
	void setupPointCloud();
};



#endif