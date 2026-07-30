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
	// Register node into the list when it loades data to VRAM
	void RegisterNode(OctreeNode* node);
	void CheckAndAct();
	float CalculateVRAMPressure();
	bool CanLoadToVRAM(size_t num_points, float priority);
	unsigned int GetCurrentMB() { return _current_bytes / (1*MB); };
	unsigned int GetMaxMB() { return _max_bytes / (1*MB); };
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

	void deleteFirstNode();

	
};


#endif