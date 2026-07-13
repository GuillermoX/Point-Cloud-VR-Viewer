#ifndef OCTREE_H
#define OCTREE_H

#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "LASReader.hpp"

class OctreeNode
{
public:
	// Constructor that just copies center and half_node_size
	OctreeNode(glm::vec3 center, float half_node_size, unsigned int max_points = 50000);
	OctreeNode(glm::vec3 center, float half_node_size, unsigned char index, unsigned int max_points = 50000);
	void AddPoint(point_data_t& point);
	void Draw();

private:
	glm::vec3 _center;
	float _half_node_size;
	std::vector<point_data_t> _points;
	unsigned int _max_points;
	
	std::unique_ptr<OctreeNode> _subnodes[8];

	void addToSubnode(point_data_t& point);

};

class Octree
{
public:
	Octree(std::vector<point_data_t>& points, glm::vec3& min, glm::vec3& max);
	void AddPoint(point_data_t& point);
	void Draw();
private:
	std::unique_ptr<OctreeNode> _root_node;	
};



#endif