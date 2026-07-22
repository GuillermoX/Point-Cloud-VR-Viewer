#include <VRAMManager.hpp>
#include <Octree.hpp>

void VRAMManager::RegisterNode(OctreeNode* node)
{
	_priority_vector.push_back(node);
	_current_bytes += node->GetNumPoints() * sizeof(point_data_t);
}




void VRAMManager::CheckAndAct()
{
	std::cout << "[VRAMManager | INFO] VRAM usage: " << (_current_bytes / (1024 * 1024)) << " / " << (_max_bytes / (1024 * 1024))
		<< " MB  |  " << ((float)_current_bytes / (float)_max_bytes)*100.0 << "%" << std::endl;
	if (_current_bytes < _max_bytes*THRESHOLD_HIGH) return;

	// Sort nodes vector by priority (TODO: implement more efficient way to sort, acceptable for hundreds of nodes)
	std::sort(_priority_vector.begin(), _priority_vector.end(),
		[](OctreeNode* a, OctreeNode* b) {
			return a->GetPriorityLevel() > b->GetPriorityLevel();
		});

	while (_current_bytes > _max_bytes*THRESHOLD_HIGH)
	{
		if (_priority_vector.empty()) return;	// No more nodes to delete (unable to reduce more the data bytes in VRAM)

		OctreeNode* front_node = std::move(_priority_vector.front());
		front_node->DeleteNodeVRAMData();		// Delete node's VRAM data
		_current_bytes -= front_node->GetNumPoints() * sizeof(point_data_t);
		_priority_vector.erase(_priority_vector.begin());		// Delete node from VRAM nodes
	}
}



float VRAMManager::CalculateVRAMPressure() {
    size_t threshold_low  = static_cast<size_t>(_max_bytes * THRESHOLD_LOW); // Low VRAM pressure
    size_t threshold_high = static_cast<size_t>(_max_bytes * THRESHOLD_HIGH); // High VRAM pressure

    // If below THRESHOLD LOW no pressure
    if (_current_bytes <= threshold_low) {
        return 0.0f;
    }

	// Calculate pressure as lineal value between low and high threshold
    float pressure = static_cast<float>(_current_bytes - threshold_low) / 
                     static_cast<float>(threshold_high - threshold_low);

	// Ensure result is not above 1.0f or below 0.0f
    return std::clamp(pressure, 0.0f, 1.0f);
}
