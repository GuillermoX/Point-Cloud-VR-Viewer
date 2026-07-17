#ifndef OCTREE_BUILDER_H
#define OCTREE_BUILDER_H

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <memory>
#include <cstring>
#include <vector>
#include <algorithm>
#include <array>
#include <queue>

#include "LASReader.hpp" 
#include "Shader.hpp"

#define MAX_OCTREE_LEVEL	3			// From 0 to MAX_OCTREE_LEVEL
#define NODE_ID_LENGHT		16


// -------------------- OctreeBuilder Binary Node Metadata ---------------------------
#pragma pack(push, 1)
struct NodeMetadata
{
	char node_id[16];		// Hierarchycal node id
	float center[3];		// Center of node box
	float half_size;		// Half the width of the box
	uint64_t byte_offset;	// Initial byte of points of this node in binary 
	uint32_t number_points;	// Number of points in node
};
#pragma pack(pop)



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
	Grid(unsigned int grid_res, float cell_size, glm::vec3 min);
	bool FreeGridCell(const point_data_t& point);
	unsigned int GetGridResolution();
	void DeleteData();	// Deletes all RAM data (since it has already been loaded to GPU RAM)

private:
	unsigned int _grid_res;
	float _cell_size;
	glm::vec3 _min;
	std::vector<bool> _grid_control;
};
	

// -------------------------- OctreeBuilder main classes --------------------------

class OctreeBuilderNode
{
public:
	std::string node_id;
	// Constructor that just copies center and half_node_size
	OctreeBuilderNode(const std::string& id, glm::vec3 center, float half_node_size, unsigned int grid_res, unsigned int level);

	// Constructor that creates OctreeBuilder Node 
	OctreeBuilderNode(const std::string& id, glm::vec3 center, float half_node_size, unsigned char index, unsigned int grid_res, unsigned int level);

	glm::vec3 GetCenter() const;
	float GetHalfSize() const;
	std::array<OctreeBuilderNode*, 8> GetSubnodes() const;
	const std::vector<point_data_t>& GetPointsVector() const;
	void AddPoint(point_data_t& point);

private:
	glm::vec3 _center;
	float _half_node_size;
	std::unique_ptr<Grid> _grid_ptr;
	std::vector<point_data_t> _points;
	unsigned int _level;
	
	std::unique_ptr<OctreeBuilderNode> _subnodes[8];

	void addToSubnode(point_data_t& point);

};



class OctreeBuilder
{
public:
	OctreeBuilder(std::vector<point_data_t>& points, glm::vec3& min, glm::vec3& max, unsigned int grid_res);
	void AddPoint(point_data_t& point);
	void CreateOctreeBinary(const std::string& metadata_path, const std::string& points_path);
private:
	std::unique_ptr<OctreeBuilderNode> _root_node;

};


#endif