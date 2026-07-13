#ifndef OCTREE_H
#define OCTREE_H

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>

#include "LASReader.hpp"
#include "Shader.hpp"

// Support struct for support calculus
struct plane_t
{
	glm::vec3 normal;
	float d;

	float getDistance(glm::vec3 point)
	{
        return glm::dot(normal, point) + d;
	}
};
	

class OctreeNode
{
public:
	std::unique_ptr<Shader> debug_shader;
	// Constructor that just copies center and half_node_size
	OctreeNode(glm::vec3 center, float half_node_size, unsigned int max_points = 50000);

	// Constructor that creates Octree Node 
	OctreeNode(glm::vec3 center, float half_node_size, unsigned char index, unsigned int max_points = 50000);

	~OctreeNode();

	void AddPoint(point_data_t& point);
	void InitGraphicBuffers();
	void Draw(plane_t planes[6]);
	void Debug(plane_t planes[6], const glm::mat4& vpm);

private:
	glm::vec3 _center;
	float _half_node_size;
	std::vector<point_data_t> _points;
	unsigned int _max_points;
	
	std::unique_ptr<OctreeNode> _subnodes[8];

	unsigned int VBO, VAO;
	unsigned int debug_VBO, debug_VAO;

	void addToSubnode(point_data_t& point);
	bool isInsideFrustrum(plane_t planes[6]);

};



class Octree
{
public:
	Octree(std::vector<point_data_t>& points, glm::vec3& min, glm::vec3& max);
	void AddPoint(point_data_t& point);
	void Draw(const glm::mat4& view_proj);
	void Debug(const glm::mat4& vpm);
private:
	std::unique_ptr<OctreeNode> _root_node;	
	void getFrustrumPlanes(const glm::mat4& vp, plane_t planes[6]);
};



#endif