#ifndef OCTREE_H
#define OCTREE_H

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <memory>
#include <vector>
#include <algorithm>

#include "LASReader.hpp"
#include "Shader.hpp"

// -------------------- Support Struct for Frustrum Culling -------------------
struct plane_t
{
	glm::vec3 normal;
	float d;

	float getDistance(glm::vec3 point)
	{
        return glm::dot(normal, point) + d;
	}
};

// --------------------- Support Class for Uniform Grid ---------------------
struct GridKey {
    int x, y, z;

	// Equality operator to use in std::unordered_map
    bool operator==(const GridKey& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

// Simple hash function
struct GridKeyHash {
	// Object Function
    std::size_t operator()(const GridKey& k) const {
        return ((k.x * 73856093) ^ (k.y * 19349663) ^ (k.z * 83492791));
    }
};

// Support class for uniform grid
class Grid {
public:
	size_t num_points;

	Grid(unsigned int grid_res, float cell_size, glm::vec3 min);
	bool AddPoint(const point_data_t& point, unsigned int level);
	size_t size();
	unsigned int GetGridResolution();
	std::vector<point_data_t>& GetPointVector();
	void DeleteData();	// Deletes all RAM data (since it has already been loaded to GPU RAM)

private:
	unsigned int _grid_res;
	float _cell_size;
	glm::vec3 _min;
	std::vector<bool> _grid_control;
	std::vector<point_data_t> _points;
};
	

// -------------------------- Octree main classes --------------------------

class OctreeNode
{
public:
	std::unique_ptr<Shader> debug_shader;
	// Constructor that just copies center and half_node_size
	OctreeNode(glm::vec3 center, float half_node_size, unsigned int grid_res, unsigned int level);

	// Constructor that creates Octree Node 
	OctreeNode(glm::vec3 center, float half_node_size, unsigned char index, unsigned int grid_res, unsigned int level);

	~OctreeNode();

	void AddPoint(point_data_t& point);
	void InitGraphicBuffers();
	void Draw(plane_t planes[6]);
	void Debug(plane_t planes[6], const glm::mat4& vpm);

private:
	glm::vec3 _center;
	float _half_node_size;
	std::unique_ptr<Grid> _grid_ptr;
	unsigned int _level;
	
	std::unique_ptr<OctreeNode> _subnodes[8];

	unsigned int VBO, VAO;
	unsigned int debug_VBO, debug_VAO;

	void addToSubnode(point_data_t& point);
	bool isInsideFrustrum(plane_t planes[6]);

};



class Octree
{
public:
	Octree(std::vector<point_data_t>& points, glm::vec3& min, glm::vec3& max, unsigned int grid_res);
	void AddPoint(point_data_t& point);
	void Draw(const glm::mat4& view_proj);
	void Debug(const glm::mat4& vpm);
private:
	std::unique_ptr<OctreeNode> _root_node;	
	void getFrustrumPlanes(const glm::mat4& vp, plane_t planes[6]);
};




#endif