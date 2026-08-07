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
#include <random>
#include <array>
#include <queue>
#include <iostream>

#include "LASReader.hpp" 
#include "Shader.hpp"

#define MAX_OCTREE_LEVEL	5			// From 0 to MAX_OCTREE_LEVEL
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
	unsigned int _max_points;
	unsigned int _current_points;
};
	

// -------------------------- OctreeBuilder main classes --------------------------

class OctreeBuilderNode
{
public:
	std::string node_id;
	/**
	 * @brief OctreeBuilderNode constructor
	 * @details Copies the values passed as arguments into the new OctreeBuilderNode
	 * @param id: Identificador of type "r023..." being r the root node
	 * @param center: Center of node box
	 * @param half_node_size: Lenght of side of the node box
	 * @param grid_res: Resolution of the node box grid (number of points in node is (grid_res x grid_res x grid_res))
	 * @param level: Depth level of node (0 is the root - N is the deepest)
	 */
	OctreeBuilderNode(const std::string& id, glm::vec3 center, float half_node_size, unsigned int grid_res, unsigned int level);

	// Constructor that creates OctreeBuilder Node 
	/**
	 * @brief OctreeBuilderNode constructor
	 * @details Creates a node child from values of parent OctreeBuilderNode. Parents values passed as params are modified to fit the new child node.
	 * @param id: Identificador of type "r023..." being r the root node
	 * @param center: Center of parent's node box
	 * @param half_node_size: Lenght of side of the parent's node box
	 * @param index: Index of child node to be created (from 0 to 7)
	 * @param grid_res: Resolution of the parent's node box grid (number of points in node is (grid_res x grid_res x grid_res))
	 * @param level: Depth level of parent's node (0 is the root - N is the deepest)
	 */
	OctreeBuilderNode(const std::string& id, glm::vec3 center, float half_node_size, unsigned char index, unsigned int grid_res, unsigned int level);
	
	/// @brief Gets the center of node
	glm::vec3 GetCenter() const;
	/// @brief Gets the half_size_node value
	float GetHalfSize() const;
	/// @brief Gets an array of all childs (may contain nullptr values if child have not been created)
	std::array<OctreeBuilderNode*, 8> GetSubnodes() const;
	/// @brief Gets points vector
	const std::vector<point_data_t>& GetPointsVector() const;

	/**
	 * @brief Adds a point to the octree node
	 * @details The same function decides if the point has to be added to the node or to the node's child
	 * @param point: Reference to the point to be added
	 */
	void AddPoint(point_data_t& point);

private:
	glm::vec3 _center;
	float _half_node_size;
	std::unique_ptr<Grid> _grid_ptr;
	std::vector<point_data_t> _points;
	unsigned int _level;
	
	std::unique_ptr<OctreeBuilderNode> _subnodes[8];

	/**
	 * @brief Adds a point to a child node
	 * @details It decides to which subnode has to be added and it creates the subnode if not created
	 * @param point: Reference to the point to be added
	 */
	void addToSubnode(point_data_t& point);

};



class OctreeBuilder
{
public:
	/**
	 * @brief OctreeBuilder constructor
	 * @details It creates all the octree structure just in memory
	 * @param points: Reference to the vector of points to be added to the octree
	 * @param min: Minimum coordinate of the point cloud
	 * @param max: Maximum coordinate of the point cloud
	 * @param grid_res: Grid resolution to be used in each node (number of points = grid_res * grid_res * grid_res9
	 */
	OctreeBuilder(std::vector<point_data_t>& points, glm::vec3& min, glm::vec3& max, unsigned int grid_res);

	/**
	 * @brief Creates the octree binary files from in-memory octree
	 * @param metadata_path: Path of the file that stores octree nodes's metadata
	 * @param points_path: Path of the file that stores octree node's points
	 */
	void CreateOctreeBinary(const std::string& metadata_path, const std::string& points_path);
private:
	std::unique_ptr<OctreeBuilderNode> _root_node;

	/**
	 * @brief Support function that randomizes the position of elements in a vector
	 * @details Used to randomize order of points when being added to the in-memory octree. This avoids spatial accumulation of points
	 *          influenced by the ".las" file points organization.
	 */
	void randomizeVector(std::vector<point_data_t>& points);

};


#endif