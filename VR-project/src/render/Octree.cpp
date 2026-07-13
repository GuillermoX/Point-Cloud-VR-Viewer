#include <Octree.hpp>

Octree::Octree(std::vector<point_data_t>& points, glm::vec3& min, glm::vec3& max)
{
	// Get the root node center from min max coords
	glm::vec3 center;
	center.x = (max.x + min.x) * 0.5f;
	center.y = (max.y + min.y) * 0.5f;
	center.z = (max.z + min.z) * 0.5f;	

	// Get longest side to get 
	float sideX = max.x - min.x;
	float sideY = max.y - min.y;
	float sideZ = max.z - min.z;
	float half_node_size = std::max({sideX, sideY, sideZ}) * 0.5f;

	_root_node = std::make_unique<OctreeNode>(center, half_node_size);
}

OctreeNode::OctreeNode(glm::vec3 center, float half_node_size, unsigned int max_points)
	: _center(center), _half_node_size(half_node_size), _max_points(max_points)
{

}

OctreeNode::OctreeNode(glm::vec3 center, float half_node_size, unsigned char index, unsigned int max_points)
{
	if (index == 0)
		_center = _center + glm::vec3(-half_node_size);
	else if (index == 1)
		_center = _center + glm::vec3(half_node_size, -half_node_size, -half_node_size);
	else if (index == 2)
		_center = _center + glm::vec3(-half_node_size, half_node_size, -half_node_size);
	else if (index == 3)
		_center = _center + glm::vec3(half_node_size, half_node_size, -half_node_size);
	else if (index == 4)
		_center = _center + glm::vec3(-half_node_size, -half_node_size, half_node_size);
	else if (index == 5)
		_center = _center + glm::vec3(half_node_size, -half_node_size, half_node_size);
	else if (index == 6)
		_center = _center + glm::vec3(-half_node_size, half_node_size, -half_node_size);
	else if (index == 7)
		_center = _center + glm::vec3(half_node_size);	

	_half_node_size /= 2;
	
}

void OctreeNode::AddPoint(point_data_t& point)
{
	if (_points.size() < _max_points)
		_points.push_back(point);
	else
	{
		addToSubnode(point);
	}
}

void OctreeNode::addToSubnode(point_data_t& point)
{
	unsigned char index = 0;
	if (point.position[0] > _center.x)
		index |= 1;

	if (point.position[1] > _center.y)
		index |= 2;

	if (point.position[2] > _center.z)
		index |= 4;
	
	// Create Octree Node if not created
	if (_subnodes[index] == nullptr)
		// Create it with the center of corresponding quarter
		_subnodes[index] = std::make_unique<OctreeNode>(_center, _half_node_size, index, _max_points);
}
