#ifndef VRAM_MANAGER_H
#define VRAM_MANAGER_H

#include <vector>

#define KB * 1024
#define MB KB * 1024
#define GB MB * 1024
#define MAX_VRAM_BYTES 1.5f GB

class OctreeNode;	// Forward Declaration of OctreeNode to avoid circular dependencies with Octree.hpp

class VRAMManager
{
public:
	// Register node into the list when it loades data to VRAM
	void RegisterNode(OctreeNode* node);
	void CheckAndAct();

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

	
};


#endif