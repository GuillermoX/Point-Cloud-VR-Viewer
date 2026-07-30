#include <Octree.hpp>

// ------------ Octree ----------------
Octree::Octree(std::string& meta_path, std::string& points_path, bool& octree_created)
{
	if (loadBinaryOctree(meta_path, points_path))
	{
		std::cout << "[OCTREE | INFO] Octree structure loaded from binary file: " << meta_path << std::endl;
		std::cout << "[OCTREE | INFO] Using octree points binary file: " << points_path << std::endl;
		float size = _root_node->GetHalfSize() * 2.0f;
		std::cout << "[OCTREE | INFO] Octree box dimension: " << size << " x " << size << std::endl;

		// Initialize LoadDataManager
		LoadDataManager::Init(points_path, NUM_WORKERS);

		_root_node->InitGraphicBuffers();

		octree_created = true;
	}
	else
	{
		octree_created = false;
	}
}

void Octree::Draw(const glm::mat4& view_proj, glm::vec3 camera_pos, float delta_time, Shader& shader)
{
	plane_t planes[6];
	getFrustrumPlanes(view_proj, planes);
	VRAMManager::GetInstance().CheckAndAct();	// Check if VRAM maximum bytes have been reached and delete data if needed
	_root_node->Draw(planes, camera_pos, delta_time, shader);
}

void Octree::Debug(const glm::mat4& vpm, glm::vec3 camera_pos)
{
	_root_node->debug_shader->use();
    glUniformMatrix4fv(glGetUniformLocation(_root_node->debug_shader->ID, "vpm"), 1, false, glm::value_ptr(vpm));
	plane_t planes[6];
	getFrustrumPlanes(vpm, planes);
	_root_node->Debug(planes, vpm, camera_pos);
}

bool Octree::loadBinaryOctree(std::string& meta_path, std::string& points_path)
{
	std::ifstream file(meta_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[OCTREE | ERROR] Unable to open binary file: " << meta_path << std::endl;
		return false;
    }		

	uint32_t num_nodes = 0;
	file.read(reinterpret_cast<char*> (&num_nodes), sizeof(num_nodes));
	
	if (num_nodes == 0)
	{
		std::cerr << "[OCTREE | WARNING] No nodes found in metadata bin file: " << meta_path << std::endl;
		return false;
	}
	else
		std::cout << "[OCTREE | INFO] Number of nodes: " << num_nodes << std::endl;
	
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
			return false;
		}

		// Get index from subnode and add it to current front queue node	
		unsigned int subnode_index = obtainIndexFromId(subnode_id);
		front_node->AddSubnode(node_meta, points_path, subnode_index);

		// Add subnode to queue since it can be other subnode's parent
		node_queue.push(front_node->GetSubnode(subnode_index));	
					
	}

	return true;
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




void OctreeNode::AddSubnode(NodeMetadata& metadata, std::string& points_path, unsigned int index)
{
	if (index > 7) return;	// Not valid index

	_subnodes[index] = std::make_unique<OctreeNode>(metadata, points_path);
}

OctreeNode* OctreeNode::GetSubnode(unsigned int index)
{
	return _subnodes[index].get();
}

void OctreeNode::Draw(plane_t planes[6], glm::vec3 camera_pos, float delta_time, Shader& shader)
{	
	bool insideFrustrum = isInsideFrustrum(planes);		// Check if node box inside frustrum
	_priority_level = calculatePriorityLevel(camera_pos, insideFrustrum);	// Update priority level
	// Check if node should be rendered, if not don't try anything
	if (insideFrustrum)
	{
		// Check if node should be rendered and fade in depending of LOD
		if (_level <= levelOfDetail(camera_pos))
		{

			// Check if buffer ready to be uploaded to VRAM
			if (_state == PointBufferState::READY_TO_LOAD_VRAM)	// If buffer in RAM buf not VRAM load it to GPU
			{
				if (VRAMManager::GetInstance().CanLoadToVRAM(_num_points, _priority_level))
				{
					glBindVertexArray(VAO);
					glBindBuffer(GL_ARRAY_BUFFER, VBO);
					glBufferData(GL_ARRAY_BUFFER, _num_points * sizeof(point_data_t), _points_ptr->data(), GL_STATIC_DRAW);
					glBindVertexArray(0);
					glBindBuffer(GL_ARRAY_BUFFER, 0);
					_state.store(PointBufferState::READY_IN_VRAM, std::memory_order_release);
					_points_ptr.reset();	// Delete point buffer pointer so the data inside is deleted from RAM
					VRAMManager::GetInstance().RegisterNode(this);
				}
			}
			else if (_state == PointBufferState::EMPTY)			// If buffer empty (not in VRAM nor RAM)
			{	
				_last_frame_active = false;
				LoadDataManager& data_manager = LoadDataManager::GetInstance();		// Get singleton instance of LoadDataManager
				if (data_manager.CanAddTask())
				{
					_points_ptr = std::make_unique<std::vector<point_data_t>>();
					_points_ptr->resize(_num_points);
					// Add the load buffer task to the LoadDataManager instance
					data_manager.AddLoadVectorTask(reinterpret_cast<char*>(_points_ptr->data()), _num_points, _byte_offset, _state);
				}
				
			}

			// Check if buffer is ready to be drawn
			if (_state == PointBufferState::READY_IN_VRAM)		// If buffer inside VRAM just render it
			{
				// If last frame wasn't visible start fade
				if (!_last_frame_active)
				{
					startFade();
					_last_frame_active = true;
				}
				else     // If was visible just update fade alpha
					updateFade(delta_time);

				glUniform1f(glGetUniformLocation(shader.ID, "fadeAlpha"), _fade_alpha);
				glBindVertexArray(VAO);
				glDrawArrays(GL_POINTS, 0, _num_points);
				glBindVertexArray(0);

			}

		}
		else
		{
			_last_frame_active = false;
		}


		// Always go to lower levels since this should have been drawn
		for (int i = 0; i < 8; i++)
		{
			if (_subnodes[i] != nullptr)
				_subnodes[i]->Draw(planes, camera_pos, delta_time, shader);
		}
	}
}

void OctreeNode::Debug(plane_t planes[6], const glm::mat4& vpm, glm::vec3 camera_pos)
{
	float priority = calculatePriorityLevel(camera_pos, isInsideFrustrum(planes));
	//glUniform3f(glGetUniformLocation(debug_shader->ID, "Color"), priority*0.0001, 10000.0/priority, 0.0);

	if(_level <= levelOfDetail(camera_pos))
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

void OctreeNode::DeleteNodeVRAMData()
{	
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);	
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	_state.store(PointBufferState::EMPTY, std::memory_order_release);
	
}

std::atomic<PointBufferState>& OctreeNode::GetState()
{
	return _state;
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


unsigned char OctreeNode::levelOfDetail(glm::vec3 camera_pos)
{
	float distance = glm::distance(camera_pos, _center);

	if (distance >= MAX_LOD_DISTANCE) return 0; // Minimum LOD
    if (distance <= 0.0f) return MAX_OCTREE_LEVEL; // Maximum LOD

	// Normalize distance between 0.0 and 1.0
    float norm_dist = distance / MAX_LOD_DISTANCE;

	// Use exponential function to concentrate all near LOD
    float factor = std::pow(norm_dist, 2.5);

	int lod = static_cast<int>(std::round(((1.0f - factor) * MAX_OCTREE_LEVEL)));
    return std::clamp(lod, 0, MAX_OCTREE_LEVEL);
}

float OctreeNode::calculatePriorityLevel(glm::vec3 camera_pos, bool insideFrustrum)
{
	float prio_level = (_level + 1) * std::pow(glm::distance(camera_pos, _center), 0.8);
	return prio_level;
}


void OctreeNode::startFade()
{
	// Start fading to avoid instant node appearing
	_fade_alpha = 0.0f;
	_is_fading = true;
}


void OctreeNode::updateFade(float delta_time)
{
	if (_is_fading) {
		_fade_alpha += delta_time;
		
		if (_fade_alpha >= 1.0f) {
			_fade_alpha = 1.0f;
			_is_fading = false; // Transición terminada
		}
	}
}


