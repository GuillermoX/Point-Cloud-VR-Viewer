#include <OctreeBuilder.hpp>

// ------------ OctreeBuilder ----------------
OctreeBuilder::OctreeBuilder(std::vector<point_data_t>& points, glm::vec3& min, glm::vec3& max, unsigned int grid_res)
{
	// Get the root node center from min max coords
	glm::vec3 center;
	center.x = (max.x + min.x) * 0.5f;
	center.y = (max.y + min.y) * 0.5f;
	center.z = (max.z + min.z) * 0.5f;	

	// Get longest side to get 
	float sideX = max.x - min.x;
	float sideY = max.y - min.y;
	float sideZ = max.z - min.z;
	float half_node_size = std::max({sideX, sideY, sideZ}) * 0.5f;

	_root_node = std::make_unique<OctreeBuilderNode>("r", center, half_node_size, grid_res, 0);
	
	unsigned int quarter_vector = points.size() / 4;
	unsigned int half_vector = points.size() / 2;
	unsigned int three_quarters_vector = points.size() - quarter_vector;
	for (unsigned int i = 0; i < points.size(); i++)
	{
		if (i == quarter_vector)
			std::cout << "[OCTREE BUILDER | INFO] Loaded 25% of points from raw points data." << std::endl;
		else if (i == half_vector)
			std::cout << "[OCTREE BUILDER | INFO] Loaded 50% of points from raw points data." << std::endl;
		else if (i == three_quarters_vector)
			std::cout << "[OCTREE BUILDER | INFO] Loaded 75% of points from raw points data." << std::endl;

		_root_node->AddPoint(points[i]);
	}

}

void OctreeBuilder::CreateOctreeBinary(const std::string& metadata_path, const std::string& points_path)
{

	std::vector<NodeMetadata> metadata_list;

	// Explore the octree using BFS
	std::queue<const OctreeBuilderNode*> node_queue;
    node_queue.push(_root_node.get());	

	uint64_t current_byte_offset = 0;					// Aux byte pointer to track the last byte of points binary
    const size_t sizeof_point_data = sizeof(point_data_t);
	
	// Open binary file to write points data
	// std::ios::binary -> Avoids OS changing special characters.
    // std::ios::trunc  -> If file already exists deletes data.
    std::ofstream points_file(points_path, std::ios::binary | std::ios::trunc);

    if (!points_file.is_open()) {
        std::cerr << "[OCTREE BUILDER | ERROR] Unable to open file to write octree points: " << points_path << std::endl;
        return;
    }	

	while (!node_queue.empty())
	{
		const OctreeBuilderNode* current_node = node_queue.front();
		node_queue.pop();
		
		NodeMetadata metadata;
		std::memset(metadata.node_id, 0, sizeof(metadata.node_id));			// Clear id char array with 0s
		strncpy_s(metadata.node_id, sizeof(metadata.node_id), current_node->node_id.c_str(), _TRUNCATE);	// Node id
		glm::vec3 center = current_node->GetCenter();
		metadata.center[0] = center.x;										// Node center
		metadata.center[1] = center.y;
		metadata.center[2] = center.z;
		metadata.half_size = current_node->GetHalfSize();					// Half size width
		metadata.byte_offset = current_byte_offset;							// Byte offset
		metadata.number_points = static_cast<uint32_t>(current_node->GetPointsVector().size());		// Number of points
		
		// Add the node metadata to the list
		metadata_list.push_back(metadata);

        // Write points into the points binary file
		const std::vector<point_data_t>& node_points = current_node->GetPointsVector();
        if (metadata.number_points > 0) 
		{
            points_file.write(
                reinterpret_cast<const char*>(node_points.data()), metadata.number_points * sizeof(point_data_t)
            );
        }

		// Update offset for next node metadata
		current_byte_offset += static_cast<uint64_t>(metadata.number_points) * sizeof_point_data;
	
		// Add the subnodes to the queue
		std::array<OctreeBuilderNode*, 8> subnodes = current_node->GetSubnodes();
        for (int i = 0; i < 8; ++i) {
            if (subnodes[i] != nullptr) {
                node_queue.push(subnodes[i]);
            }
        }

		std::cout << "[OCTREE BUILDER | INFO] " << metadata.number_points << " points of node " << current_node->node_id << " written. Total KB: "
			<< (metadata.number_points * sizeof(point_data_t))/1024.0f << std::endl;
	}

	points_file.close();

	// std::ios::binary -> Avoids OS changing special characters.
    // std::ios::trunc  -> If file already exists deletes data.
    std::ofstream meta_file(metadata_path, std::ios::binary | std::ios::trunc);

    if (!meta_file.is_open()) {
        std::cerr << "[OCTREE BUILDER | ERROR] Unable to open file to write octree metadata: " << metadata_path << std::endl;
        return;
    }	
	
	// Simple header with total nodes 
    uint32_t total_nodes = static_cast<uint32_t>(metadata_list.size());
    meta_file.write(reinterpret_cast<const char*>(&total_nodes), sizeof(total_nodes));

	// Write in disk all node metadata
    size_t metadata_list_bytes = total_nodes * sizeof(NodeMetadata);
    meta_file.write(reinterpret_cast<const char*>(metadata_list.data()), metadata_list_bytes);

	meta_file.close();
    std::cout << "[OCTREE BUILDER | INFO] All octree nodes metadata exported to file: " << metadata_path << " | Total nodes: " << total_nodes << std::endl;

}


// ----------- OctreeBuilderNode -------------------

OctreeBuilderNode::OctreeBuilderNode(const std::string& id, glm::vec3 center, float half_node_size, unsigned int grid_res, unsigned int level)
	: node_id(id), _center(center), _half_node_size(half_node_size), _level(level)
{
	_grid_ptr = std::make_unique<Grid>(grid_res, (_half_node_size * 2.0) / (float)grid_res, _center - glm::vec3(half_node_size));
}

OctreeBuilderNode::OctreeBuilderNode(const std::string& id, glm::vec3 center, float half_node_size, unsigned char index, unsigned int grid_res, unsigned int level)
	: node_id(id), _level(level)
{
	float quarter_node_size = half_node_size * 0.5;
	if (index == 0)
		_center = center + glm::vec3(-quarter_node_size);
	else if (index == 1)
		_center = center + glm::vec3(quarter_node_size, -quarter_node_size, -quarter_node_size);
	else if (index == 2)
		_center = center + glm::vec3(-quarter_node_size, quarter_node_size, -quarter_node_size);
	else if (index == 3)
		_center = center + glm::vec3(quarter_node_size, quarter_node_size, -quarter_node_size);
	else if (index == 4)
		_center = center + glm::vec3(-quarter_node_size, -quarter_node_size, quarter_node_size);
	else if (index == 5)
		_center = center + glm::vec3(quarter_node_size, -quarter_node_size, quarter_node_size);
	else if (index == 6)
		_center = center + glm::vec3(-quarter_node_size, quarter_node_size, quarter_node_size);
	else if (index == 7)
		_center = center + glm::vec3(quarter_node_size);
	else
		_center = center;

	node_id += std::to_string(index);		// Update ID

	_half_node_size = quarter_node_size;	

	_grid_ptr = std::make_unique<Grid>(grid_res, (_half_node_size * 2.0) / (float)grid_res, _center - glm::vec3(_half_node_size));
}


glm::vec3 OctreeBuilderNode::GetCenter() const
{
	return _center;
}

float OctreeBuilderNode::GetHalfSize() const
{
	return _half_node_size;
}
	
std::array<OctreeBuilderNode*, 8> OctreeBuilderNode::GetSubnodes() const
{
	std::array<OctreeBuilderNode*, 8> subnodes;
	
	for (int i = 0; i < 8; ++i) {
		subnodes[i] = _subnodes[i].get();	// Get raw pointer from std::unique_ptr
	}
	
	return subnodes;
}

const std::vector<point_data_t>& OctreeBuilderNode::GetPointsVector() const
{
	return _points;
}


void OctreeBuilderNode::AddPoint(point_data_t& point)
{
	// Try adding point, if not possible delegate to subnode
	// Limit the maximum level of depth of subnodes
	if (_grid_ptr->FreeGridCell(point) || _level == MAX_OCTREE_LEVEL)
	{
		_points.push_back(point);
	}
	else	
		addToSubnode(point);
}


void OctreeBuilderNode::addToSubnode(point_data_t& point)
{
	unsigned char index = 0;
	if (point.position[0] > _center.x)
		index |= 1;		// First bit to one

	if (point.position[1] > _center.y)
		index |= 2;		// Second bit to one

	if (point.position[2] > _center.z)
		index |= 4;		// Third bit to one
	
	// Create OctreeBuilder Node if not created
	if (_subnodes[index] == nullptr)
	{
		// Create it with the center of corresponding quarter
		_subnodes[index] = std::make_unique<OctreeBuilderNode>(node_id, _center, _half_node_size, index, _grid_ptr->GetGridResolution(), _level + 1);
	}

	_subnodes[index]->AddPoint(point);
}



// --------------- Support class: Grid -----------------------

Grid::Grid(unsigned int grid_res, float cell_size, glm::vec3 min)
	: _grid_control(grid_res* grid_res* grid_res, false), _grid_res(grid_res), 
	  _cell_size(cell_size), _min(min)
{
}

bool Grid::FreeGridCell(const point_data_t& point)
{
	int cellX = std::clamp((int)((point.position[0] - _min.x) / _cell_size), 0, (int)(_grid_res - 1));
	int cellY = std::clamp((int)((point.position[1] - _min.y) / _cell_size), 0, (int)(_grid_res - 1));
	int cellZ = std::clamp((int)((point.position[2] - _min.z) / _cell_size), 0, (int)(_grid_res - 1));

	int idx = cellX + (cellY * _grid_res) + (cellZ * _grid_res * _grid_res);

	if (!_grid_control[idx]) 
	{
		_grid_control[idx] = true;
		return true;
	} 
	else 
		return false;
}

unsigned int Grid::GetGridResolution()
{
	return _grid_res;
}

void Grid::DeleteData()
{
	_grid_control.clear();
}


