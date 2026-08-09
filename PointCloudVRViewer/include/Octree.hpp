#ifndef OCTREE_H
#define OCTREE_H

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
#include <iostream>
#include <atomic>
#include <list>

#include "LASReader.hpp" 
#include "Shader.hpp"
#include "OctreeBuilder.hpp"
#include "PointBufferState.hpp"
#include "LoadDataManager.hpp"
#include "VRAMManager.hpp"

inline float MAX_LOD_DISTANCE;	 // Distance where the LOD is minimum



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


// -------------------------- Octree main classes --------------------------

/**
 * @brief Octree node element from octree structure
 */
class OctreeNode
{
public:
	std::unique_ptr<Shader> debug_shader;
	std::string node_id;
	
	/**
	 * @brief OctreeNode constructor
	 * @details Copies the metadata info to the octree node parameters and stores the path to the points binary file
	 * @param metadata: Metadata information of the OctreeNode to create
	 * @param points_path: Path to the points binary file. Used to know where to get points from.
	 */
	OctreeNode(NodeMetadata& metadata, std::string& points_path);
	
	/**
	 * @brief Initializes all graphics buffers (VAO and VBO) of the node.
	 */
	void InitGraphicBuffers();

	/**
	 * @brief Draws the point cloud inside the octree node box
	 * @param planes: Planes of the camera frustrum
	 * @param camera_pos: Position of the camera
	 * @param delta_time: Delta time of current frame
	 */
	void Draw(plane_t planes[6], glm::vec3 camera_pos, float delta_time, Shader& shader);

	/**
	 * @brief Draws the debugging info of the node box
	 * @param planes: Planes of the camera frustrum
	 * @param camera_pos: Position of the camera
	 */
	void Debug(plane_t planes[6], glm::vec3 camera_pos);

	/**
	 * @brief Adds a subnode to the current octree node
	 * @details It creates the instance of the subnode
	 * @param metadata: Metadata of the new subnode
	 * @points_path: Path to the points binary file
	 * @index: Index of the subnode to be created
	 */
	void AddSubnode(NodeMetadata& metadata, std::string& points_path, unsigned int index);

	/**
	 * @brief Get the instance of the specified subndode
	 * @param index: Index of the wanted subnode
	 * @returns Pointer to the subnode OctreeNode. !WARNING: nullptr returned if subnode doesn't exists.
	 */
	OctreeNode* GetSubnode(unsigned int index);

	/**
	 * @brief Deletes current VBO data from VRAM
	 */
	void DeleteNodeVRAMData();

	/// @brief Gets the number of points inside the octree node box
	size_t GetNumPoints() { return _num_points; }
	/// @brief Gets the current priority level of the octree node
	float GetPriorityLevel() { return _priority_level;  }
	/// @brief Gets the octree node box half size 
	float GetHalfSize() { return _half_size;  }
	/// @brief Gets the state of the octree node buffer
	std::atomic<PointBufferState>& GetState() { return _state;  }
	

private:
	glm::vec3 _center;
	float _half_size;
	uint64_t _byte_offset;
	size_t _num_points;
	unsigned int _level;
	std::string _points_binary_path;

	std::unique_ptr<std::vector<point_data_t>> _points_ptr;	// Bridge buffer between disk and VRAM
	
	std::unique_ptr<OctreeNode> _subnodes[8];

	unsigned int VBO, VAO;
	unsigned int debug_VBO, debug_VAO;
	std::atomic<PointBufferState> _state{ PointBufferState::EMPTY };	// State of the point buffer
	float _priority_level;	// Punctuation that determinates the priority to don't be deleted from VRAM
	bool _last_frame_active = false;	// In last frame the node was active according to LOD (not same as visible, which depends on frustrum)
	float _fade_alpha = 0.0;
	bool _is_fading = false;
	
	/**
	 * @brief Starts the appearing fade of the points 
	 */
	void startFade();

	/**
	 * @brief Updates the fade alpha value in a frame
	 * @param delta_time: Delta time of the current frame
	 */
	void updateFade(float delta_time);

	/**
	 * @brief Checks if current node is inside the frustrum
	 * @planes planes: Frustrum planes
	 */
	bool isInsideFrustrum(plane_t planes[6]);

	/**
	 * @brief Get the maximum level of detail to be rendered depending on camera position and octree node position
	 * @param camera_pos: Camera position
	 * @return Maximum value of LOD to be drawn
	 */
	unsigned char levelOfDetail(glm::vec3 camera_pos);

	/**
	 * @brief Calculates a priority level based in camera_position and octree node
	 */
	float calculatePriorityLevel(glm::vec3 camera_pos);

};


/**
 * @brief In-memory octree structure loaded from octree binary files
 */
class Octree
{
public:
	/**
	 * @brief Octree constructor
	 * @details It creates the octree in-memory structure from binary octree files
	 * @param meta_path: Path to the metadata binary file
	 * @param points_path: Path to the points binary file
	 * @param octree_creates[out]: returns true if octree was created successfully, otherwise returns false
	 */
	Octree(std::string& meta_path, std::string& points_path, bool& octree_created);

	/**
	 * @brief Draws the point cloud inside the octree structure
	 * @param view_proj: Multiplication of view and projection matrices used to render the scene
	 * @param camera_pos: Position of the camera
	 * @param delta_time: Delta Time of current frame
	 * @param shader: Shader used to render point cloud (used to change uniform values)
	 */
	void Draw(const glm::mat4& view_proj, glm::vec3 camera_pos, float delta_time, Shader& shader);

	/**
	 * @brief Draws the debug octree box information
	 * @param vpm: Multiplication of view-projection-model matrices used to render the scene
	 * @param camera_pos: Position of the camera
	 */
	void Debug(const glm::mat4& vpm, glm::vec3 camera_pos);

private:
	std::unique_ptr<OctreeNode> _root_node;	

	/**
	 * @brief Loads from binary files all the octree structure
	 * @details Used in Octree constructor
	 * @param meta_path: Path to the metadata binary file
	 * @param points_path: Path to the points binary file
	 */
	bool loadBinaryOctree(std::string& meta_path, std::string& points_path);

	/**
	 * @brief Obtains the planes of the frustrum from a view-projection matrix
	 * @param vp: Multiplication of the view and projection matrix
	 * @param planes[out]: Array of plane_t where the frustrum planes will be stored
	 */
	void getFrustrumPlanes(const glm::mat4& vp, plane_t planes[6]);

	/**
	 * @brief Helper function to get the subnode index from the node id. For example from id "r0123" the subnode index is 3
	 * @param id: Identifier of the node
	 */
	int obtainIndexFromId(const std::string& id);

	/**
	 * @brief Checks if a node is the subnode of another one from corresponding ids
	 * @param id_parent: Identifier of possible parent node
	 * @param id_subnode: Identifier of the possible subnode
	 */
	bool isSubnodeOf(const std::string& id_parent, const std::string& id_subnode);
};




#endif