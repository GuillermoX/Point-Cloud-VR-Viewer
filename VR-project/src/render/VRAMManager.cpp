#include <VRAMManager.hpp>
#include <Octree.hpp>

void VRAMManager::RegisterNode(OctreeNode* node)
{
	_priority_vector.push_back(node);
	_current_bytes += node->GetNumPoints() * sizeof(point_data_t);
}




void VRAMManager::CheckAndAct()
{
	if (_current_bytes < _max_bytes) return;

	// Sort nodes vector by priority
	std::sort(_priority_vector.begin(), _priority_vector.end(), 
        [](OctreeNode* a, OctreeNode* b) { 
            return a->GetPriorityLevel() > b->GetPriorityLevel(); // Los de mayor score (peores) van al principio
        });

	while (_current_bytes > _max_bytes)
	{
		if (_priority_vector.empty()) return;	// No more nodes to delete (unable to reduce more the data bytes in VRAM)

		OctreeNode* front_node = std::move(_priority_vector.front());
		front_node->DeleteNodeVRAMData();		// Delete node's VRAM data
		_current_bytes -= front_node->GetNumPoints() * sizeof(point_data_t);
		_priority_vector.erase(_priority_vector.begin());		// Delete node from VRAM nodes
	}
}

