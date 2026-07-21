#ifndef VRAM_MANAGER_H
#define VRAM_MANAGER_H

#include <list>


#define KB * 1024
#define MB KB * 1024
#define GB MB * 1024
#define MAX_VRAM_BYTES 100 MB

class OctreeNode;	// Forward Declaration of OctreeNode to avoid circular dependencies with Octree.hpp

class VRAMManager
{
public:
	// Register node into the list when it loades data to VRAM
	void RegisterNode(OctreeNode* node);
	// Positions node to the end of the LRU list (called each time node is drawn)
	void UpdateNode(OctreeNode* node);
	void CheckAndAct();

	static VRAMManager& GetInstance()
	{
		static VRAMManager vram_manager;	// Creates only one instance although GetInstance called multiple times
		return vram_manager;
	}

private:
	std::list<OctreeNode*> _lru_list;
	size_t _max_bytes = MAX_VRAM_BYTES;
	size_t _current_bytes = 0;

	bool deleteOldest();	// Returns true if a node has been deleted

	VRAMManager() {};// Make constructor private
	VRAMManager(const VRAMManager&) = delete;	// Delete copy constructor
	VRAMManager& operator=(const VRAMManager&) = delete;	// Delete asignation operator

	
};


#endif