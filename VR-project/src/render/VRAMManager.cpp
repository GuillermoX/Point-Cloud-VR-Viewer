#include <VRAMManager.hpp>
#include <Octree.hpp>

void VRAMManager::RegisterNode(OctreeNode* node)
{
	_priority_vector.push_back(node);
	_current_bytes += node->GetNumPoints() * sizeof(point_data_t);
}




void VRAMManager::CheckAndAct()
{
	if (_current_bytes >= _max_bytes * THRESHOLD_LOW)
	{
		// Sort nodes vector by priority (TODO: implement more efficient way to sort, acceptable for hundreds of nodes)
		std::sort(_priority_vector.begin(), _priority_vector.end(),
			[](OctreeNode* a, OctreeNode* b) {
				return a->GetPriorityLevel() > b->GetPriorityLevel();		// Order from bigger to smaller prority level
			});

		// If higher than high threshold delete nodes
		if (_current_bytes >= _max_bytes * THRESHOLD_HIGH)
		{
			while (_current_bytes > _max_bytes*THRESHOLD_LOW)
			{
				if (_priority_vector.empty()) return;	// No more nodes to delete (unable to reduce more the data bytes in VRAM)
				deleteFirstNode();
			}
		}
	}
	

}


bool VRAMManager::CanLoadToVRAM(size_t num_points, float priority)
{
	bool write_to_vram = false;

	size_t points_bytes = num_points * sizeof(point_data_t);
	// If adding the new node to VRAM the maximum is superpassed try to delete inferior nodes
	if ((_current_bytes + points_bytes) > _max_bytes * THRESHOLD_HIGH)
	{
		bool finished = false;
		size_t acumulated_bytes = 0;
		
		unsigned int index = 0;
		unsigned int vector_size = _priority_vector.size();
		while (!finished && (index < vector_size))
		{
			// Stop when target priority level is higher (worst priority value)
			if (priority > _priority_vector[index]->GetPriorityLevel())
			{
				finished = true;
			}
			else
			{
				acumulated_bytes += _priority_vector[index]->GetNumPoints() * sizeof(point_data_t);
				// If needed bytes reached, delete inferior nodes
				if (acumulated_bytes >= points_bytes)
				{
					finished = true;
					write_to_vram = true;
					for (int i = 0; i <= index; i ++)
					{
						deleteFirstNode();
					}
				}
			}

			index++;	
		}
	}
	else
		write_to_vram = true;

	return write_to_vram;
	
}

void VRAMManager::deleteFirstNode()
{
	OctreeNode* front_node = std::move(_priority_vector.front());
	front_node->DeleteNodeVRAMData();		// Delete node's VRAM data
	front_node->GetState().store(PointBufferState::EMPTY, std::memory_order_release);
	_current_bytes -= front_node->GetNumPoints() * sizeof(point_data_t);
	_priority_vector.erase(_priority_vector.begin());		// Delete node from VRAM nodes
}
