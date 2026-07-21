#include <VRAMManager.hpp>
#include <Octree.hpp>

void VRAMManager::RegisterNode(OctreeNode* node)
{
	_lru_list.push_back(node);
	node->lru_iterator = --_lru_list.end();		// Store iterator inside node to move it in O(1)
	_current_bytes += node->GetNumPoints() * sizeof(point_data_t);
}


void VRAMManager::UpdateNode(OctreeNode* node)
{
	_lru_list.splice(_lru_list.end(), _lru_list, node->lru_iterator);
}



void VRAMManager::CheckAndAct()
{
	if (_current_bytes < _max_bytes) return;

	while (_current_bytes > _max_bytes)
	{
		bool node_deleted = deleteOldest();
		if (!node_deleted) return;		// No more nodes to delete (unable to reduce more the data bytes in VRAM)
	}
}

bool VRAMManager::deleteOldest()
{
	if (_lru_list.empty()) return false;

	// Delete first node in list
	OctreeNode* oldest_node = _lru_list.front();
	oldest_node->DeleteNodeVRAMData();
	_current_bytes -= oldest_node->GetNumPoints() * sizeof(point_data_t);
	
	_lru_list.pop_front();		// Delete it from list	
	return true;
}
