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

#define MIN_LOD_DISTANCE 500.0		// Distance where the LOD is minimum



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

class OctreeNode
{
public:
	std::unique_ptr<Shader> debug_shader;
	std::string node_id;
	
	// Constructor that just copies center and half_node_size
	OctreeNode(NodeMetadata& metadata, std::string& points_path);


	~OctreeNode();
	
	void InitGraphicBuffers();
	void Draw(plane_t planes[6], glm::vec3 camera_pos, float delta_time, Shader& shader);
	void Debug(plane_t planes[6], const glm::mat4& vpm, glm::vec3 camera_pos);
	void AddSubnode(NodeMetadata& metadata, std::string& points_path, unsigned int index);
	OctreeNode* GetSubnode(unsigned int index);
	void DeleteNodeVRAMData();
	size_t GetNumPoints() { return _num_points; }
	float GetPriorityLevel() { return _priority_level;  }

private:
	glm::vec3 _center;
	float _half_size;
	uint64_t _byte_offset;
	size_t _num_points;
	unsigned int _level;
	std::string _points_binary_path;

	std::vector<point_data_t> points;
	
	std::unique_ptr<OctreeNode> _subnodes[8];

	unsigned int VBO, VAO;
	unsigned int debug_VBO, debug_VAO;
	std::atomic<PointBufferState> _state{ PointBufferState::EMPTY };
	float _priority_level;	// Punctuation that determinates the priority to don't be deleted from VRAM
	bool _last_frame_active = false;	// In last frame the node was active according to LOD (not same as visible, which depends on frustrum)
	float _fade_alpha = 0.0;
	bool _is_fading = false;
	
	void startFade();
	void updateFade(float delta_time);
	bool isInsideFrustrum(plane_t planes[6]);
	bool levelOfDetail(glm::vec3 camera_pos);
	float calculatePriorityLevel(glm::vec3 camera_pos, bool insideFrustrum);

};



class Octree
{
public:
	Octree(std::string& meta_path, std::string& points_path);
	void Draw(const glm::mat4& view_proj, glm::vec3 camera_pos, float delta_time, Shader& shader);
	void Debug(const glm::mat4& vpm, glm::vec3 camera_pos);
private:
	std::unique_ptr<OctreeNode> _root_node;	
	void loadBinaryOctree(std::string& meta_path, std::string& points_path);
	void getFrustrumPlanes(const glm::mat4& vp, plane_t planes[6]);
	int obtainIndexFromId(const std::string& id);
	bool isSubnodeOf(const std::string& id_parent, const std::string& id_subnode);
};




#endif