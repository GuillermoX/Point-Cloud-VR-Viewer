#include <Octree.hpp>

// ------------ Octree ----------------
Octree::Octree(std::string& meta_path, std::string& points_path)
{
	loadBinaryOctree(meta_path, points_path);
	
	// Initialize DataManager
	DataManager::Init(points_path, NUM_WORKERS);

	_root_node->InitGraphicBuffers();
}

void Octree::Draw(const glm::mat4& view_proj, glm::vec3 camera_pos)
{
	plane_t planes[6];
	getFrustrumPlanes(view_proj, planes);
	_root_node->Draw(planes, camera_pos);
}

void Octree::Debug(const glm::mat4& vpm, glm::vec3 camera_pos)
{
	_root_node->debug_shader->use();
    glUniformMatrix4fv(glGetUniformLocation(_root_node->debug_shader->ID, "vpm"), 1, false, glm::value_ptr(vpm));
	plane_t planes[6];
	getFrustrumPlanes(vpm, planes);
	_root_node->Debug(planes, vpm, camera_pos);
}

void Octree::loadBinaryOctree(std::string& meta_path, std::string& points_path)
{
	std::ifstream file(meta_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[OCTREE | ERROR] Unable to open binary file: " << meta_path << std::endl;
    }		

	uint32_t num_nodes = 0;
	file.read(reinterpret_cast<char*> (&num_nodes), sizeof(num_nodes));
	
	if (num_nodes == 0)
	{
		std::cerr << "[OCTREE | WARNING] No nodes found in metadata bin file: " << meta_path << std::endl;
		return;
	}
	
	std::queue<OctreeNode*> node_queue;
	
	NodeMetadata node_meta;
	file.read(reinterpret_cast<char*>(&node_meta), sizeof(node_meta));
	_root_node = std::make_unique<OctreeNode>(node_meta, points_path);
	node_queue.push(_root_node.get());		// Add root as first element in queue

	for (int i = 0; i < num_nodes - 1; i++)
	{
		file.read(reinterpret_cast<char*>(&node_meta), sizeof(node_meta));
	
		OctreeNode* front_node = node_queue.front();
		std::string subnode_id(node_meta.node_id);
		// Find subnode's parent
		while (!isSubnodeOf(front_node->node_id, subnode_id))
		{
			// If is not child go to next queue's node and forget last node
			node_queue.pop();

			if (node_queue.empty())		// If no more parent nodes binary hierarchy broken
				break;
			
			front_node = node_queue.front();
		}

		if (node_queue.empty())	// If parent node not found binary is corrupted
		{
			std::cerr << "[OCTREE | ERROR] Corrupted octree metadata binary, node hierarchy broken" << std::endl;
			return;
		}

		// Get index from subnode and add it to current front queue node	
		unsigned int subnode_index = obtainIndexFromId(subnode_id);
		front_node->AddSubnode(node_meta, points_path, subnode_index);

		// Add subnode to queue since it can be other subnode's parent
		node_queue.push(front_node->GetSubnode(subnode_index));	
					
	}
}

void Octree::getFrustrumPlanes(const glm::mat4& vp, plane_t planes[6])
{

	// Right plane
	planes[0].normal.x = vp[0][3] - vp[0][0];
	planes[0].normal.y = vp[1][3] - vp[1][0];
	planes[0].normal.z = vp[2][3] - vp[2][0];
	planes[0].d = vp[3][3] - vp[3][0];

	// Left plane
	planes[1].normal.x = vp[0][3] + vp[0][0];
	planes[1].normal.y = vp[1][3] + vp[1][0];
	planes[1].normal.z = vp[2][3] + vp[2][0];
	planes[1].d = vp[3][3] + vp[3][0];

	// Bottom plane
	planes[2].normal.x = vp[0][3] + vp[0][1];
	planes[2].normal.y = vp[1][3] + vp[1][1];
	planes[2].normal.z = vp[2][3] + vp[2][1];
	planes[2].d = vp[3][3] + vp[3][1];

	// Top plane
	planes[3].normal.x = vp[0][3] - vp[0][1];
	planes[3].normal.y = vp[1][3] - vp[1][1];
	planes[3].normal.z = vp[2][3] - vp[2][1];
	planes[3].d = vp[3][3] - vp[3][1];

	// Far plane
	planes[4].normal.x = vp[0][3] - vp[0][2];
	planes[4].normal.y = vp[1][3] - vp[1][2];
	planes[4].normal.z = vp[2][3] - vp[2][2];
	planes[4].d = vp[3][3] - vp[3][2];

	// Near plane
	planes[5].normal.x = vp[0][3] + vp[0][2];
	planes[5].normal.y = vp[1][3] + vp[1][2];
	planes[5].normal.z = vp[2][3] + vp[2][2];
	planes[5].d = vp[3][3] + vp[3][2];
	
	// Normalize planes
	for (int i = 0; i < 6; i++) {
		float lenght = glm::length(planes[i].normal);
		planes[i].normal /= lenght;
		planes[i].d /= lenght;
	}

}

int Octree::obtainIndexFromId(const std::string& id)
{
	if (id == "r" || id.empty()) return -1; // If root has no subnode index
	return id.back() - '0'; // Convert char to number

}

bool Octree::isSubnodeOf(const std::string& id_parent, const std::string& id_subnode)
{
	// Subnode's id has one more character than parent's
    if (id_subnode.length() != id_parent.length() + 1) {
        return false;
    }
	// Subnode's id has to start as the parent one
    return id_subnode.rfind(id_parent, 0) == 0;
}


// ----------- OctreeNode -------------------

OctreeNode::OctreeNode(NodeMetadata& metadata, std::string& points_path)
	: node_id(metadata.node_id), _half_size(metadata.half_size), _byte_offset(metadata.byte_offset),
	  _num_points(metadata.number_points), _points_binary_path(points_path)
{
	_center = glm::vec3(metadata.center[0], metadata.center[1], metadata.center[2]);
	_level = node_id.length() - 1;	// Calculate the depth level from node id
}



OctreeNode::~OctreeNode()
{
	glDeleteBuffers(1, &VBO);
	glDeleteVertexArrays(1, &VAO);
}	

void OctreeNode::AddSubnode(NodeMetadata& metadata, std::string& points_path, unsigned int index)
{
	if (index > 7) return;	// Not valid index

	_subnodes[index] = std::make_unique<OctreeNode>(metadata, points_path);
}

OctreeNode* OctreeNode::GetSubnode(unsigned int index)
{
	return _subnodes[index].get();
}

void OctreeNode::Draw(plane_t planes[6], glm::vec3 camera_pos)
{	
	// Check if buffer ready to be uploaded to VRAM
	if (_state == PointBufferState::READY_TO_LOAD_VRAM)	// If buffer in RAM buf not VRAM load it to GPU
	{
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(point_data_t), points.data(), GL_STATIC_DRAW);	
		glBindVertexArray(0);
		_state.store(PointBufferState::READY_IN_VRAM, std::memory_order_release);
		points.clear();		// Delete point buffer from RAM because is already stored in VRAM
	}
	else if (_state == PointBufferState::EMPTY)			// If buffer empty (not in VRAM nor RAM)
	{
		points.resize(_num_points);		// Resize to prepare buffer 
		DataManager& data_manager = DataManager::GetInstance();		// Get singleton instance of DataManager
		// Add the load buffer task to the DataManager instance
		data_manager.AddLoadVectorTask(reinterpret_cast<char*>(points.data()), _num_points, _byte_offset, _state);
	}
	
	// Check if buffer is ready to be drawn
	if (_state == PointBufferState::READY_IN_VRAM)		// If buffer inside VRAM just render it
	{
		if (isInsideFrustrum(planes) && levelOfDetail(camera_pos))
		{
			glBindVertexArray(VAO);
			glDrawArrays(GL_POINTS, 0, _num_points);
			glBindVertexArray(0);
		}
		for (int i = 0; i < 8; i++)
		{
			if (_subnodes[i] != nullptr)
				_subnodes[i]->Draw(planes, camera_pos);
		}
	}
}

void OctreeNode::Debug(plane_t planes[6], const glm::mat4& vpm, glm::vec3 camera_pos)
{
	if (isInsideFrustrum(planes) && levelOfDetail(camera_pos))
		glUniform3f(glGetUniformLocation(debug_shader->ID, "Color"), 0.0, 1.0, 0.0);
	else
		glUniform3f(glGetUniformLocation(debug_shader->ID, "Color"), 1.0, 0.0, 0.0);

	glm::mat4 insideModel = glm::mat4(1.0f);
	insideModel = glm::translate(insideModel, _center);
	insideModel = glm::scale(insideModel, glm::vec3(_half_size * 2.0f));
	glUniformMatrix4fv(glGetUniformLocation(debug_shader->ID, "insideModel"), 1, false, glm::value_ptr(insideModel));

	glBindVertexArray(debug_VAO);
	glDrawArrays(GL_LINES, 0, 24);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	
	for (int i = 0; i < 8; i++)
	{
		if (_subnodes[i] != nullptr)
			_subnodes[i]->Debug(planes, vpm, camera_pos);
	}
}

void OctreeNode::InitGraphicBuffers()
{
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	// Specify how to read Point coords in shader
    glEnableVertexAttribArray(0);	
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(point_data_t), (void*)0);
	// Specify how to read Point colors in shader
    glEnableVertexAttribArray(1);	
    glVertexAttribPointer(1, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(point_data_t), (void*)offsetof(point_data_t, color));

    std::cout << "[VBO DEBUG OCTREE | " << node_id << "] Sizeof Vertex Struct : " << sizeof(point_data_t) << " bytes." << std::endl;
    std::cout << "[VBO DEBUG OCTREE | " << node_id << "] Total Buffer Size: " << (points.size() * sizeof(point_data_t) / 1024.0f) << " Kbytes." << std::endl;

    glBindVertexArray(0);	

	
	// Init all buffers of active subnodes
	for (int i = 0; i < 8; i++)
	{
		if (_subnodes[i] != nullptr)
			_subnodes[i]->InitGraphicBuffers();
	}
	
	// DEBUG
	glGenVertexArrays(1, &debug_VAO);
	glGenBuffers(1, &debug_VBO);

	glBindVertexArray(debug_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, debug_VBO);
	

	const float wireframeCubeVertices[] = {
		// Bottom face
		-0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,
		 0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,
		-0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f,
		// Top face
		-0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
		 0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f,
		// Columns
		-0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
		 0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
		-0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f
	};

    glBufferData(GL_ARRAY_BUFFER, sizeof(wireframeCubeVertices), wireframeCubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);	
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

	debug_shader = std::make_unique<Shader>("assets/shaders/debug.vert", "assets/shaders/debug.frag");
	
}

bool OctreeNode::isInsideFrustrum(plane_t planes[6])
{	
	for (int i = 0; i < 6; i++) {
		plane_t plane = planes[i];

		// Find point of node nearest to the plane
		glm::vec3 p = _center;
		if (plane.normal.x >= 0) p.x += _half_size; else p.x -= _half_size;
		if (plane.normal.y >= 0) p.y += _half_size; else p.y -= _half_size;
		if (plane.normal.z >= 0) p.z += _half_size; else p.z -= _half_size;

		// If the nearest point of the plane is behind the plane the whole node box is out
		if (plane.getDistance(p) < 0) {
			return false;
		}
	}
	
	return true;
}


bool OctreeNode::levelOfDetail(glm::vec3 camera_pos)
{
	float distance = glm::distance(camera_pos, _center);
	unsigned int max_level = distance / (MIN_LOD_DISTANCE / (MAX_OCTREE_LEVEL + 1));
	if (max_level > MAX_OCTREE_LEVEL) max_level = MAX_OCTREE_LEVEL;
	max_level = MAX_OCTREE_LEVEL - max_level;		// If further LOD level is smaller

	return _level <= max_level;
}


