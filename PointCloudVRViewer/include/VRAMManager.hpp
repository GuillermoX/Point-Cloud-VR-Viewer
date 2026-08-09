#ifndef VRAM_MANAGER_H
#define VRAM_MANAGER_H

#include <vector>
#include "LASReader.hpp"

#define KB (1024ULL)
#define MB (1024ULL * KB)
#define GB (1024ULL * MB)
//#define MAX_VRAM_BYTES 1ULL GB
inline size_t MAX_VRAM_BYTES;
#define THRESHOLD_LOW 0.90
#define THRESHOLD_HIGH 0.95

class OctreeNode;	// Forward Declaration of OctreeNode to avoid circular dependencies with Octree.hpp

class VRAMManager
{
public:
	/**
	 * @brief Registers OctreeNode into the list of nodes with VRAM data loaded
	 * @param node: Pointer to the OctreeNode to be tracked
	 */
	void RegisterNode(OctreeNode* node);

	/**
	 * @brief Checks the state of VRAM capacity and acts in case memory is full
	 */
	void CheckAndAct();
	
	/**
	 * @brief Checks if a number of points can be loaded into VRAM based on priority level of node
	 * @details Used to avoid flickering of nodes being loaded into VRAM and instantly being deleted continuosly
	 * @param num_points: Number of points to be loaded
	 * @param priority: Priority level of node
	 */
	bool CanLoadToVRAM(size_t num_points, float priority);

	/// @brief Gets the current VRAM's MegaBytes used by te program 
	unsigned int GetCurrentMB() { return _current_bytes / (1*MB); };
	/// @brief Gets the maximum VRAM's MegaBytes that can be used by te program 
	unsigned int GetMaxMB() { return _max_bytes / (1*MB); };
	/**
	 * @brief Gets the instance of the VRAMManager
	 * @details Follows a Singleton design pattern
	 */
	static VRAMManager& GetInstance()
	{
		static VRAMManager vram_manager;	// Creates only one instance although GetInstance called multiple times
		return vram_manager;
	}

private:
	std::vector<OctreeNode*> _priority_vector;
	size_t _max_bytes = MAX_VRAM_BYTES;
	size_t _current_bytes = 0;


	VRAMManager() {};// Make constructor private
	VRAMManager(const VRAMManager&) = delete;	// Delete copy constructor
	VRAMManager& operator=(const VRAMManager&) = delete;	// Delete asignation operator
	
	/**
	 * @brief Deletes the first node of the OctreeNode list
	 */
	void deleteFirstNode();

	
};


#endif