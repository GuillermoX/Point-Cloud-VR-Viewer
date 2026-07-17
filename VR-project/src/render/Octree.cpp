#include <Octree.hpp>

// ------------ Octree ----------------
Octree::Octree(std::string& meta_path, std::string& points_path)
{
	


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

void loadBinaryOctree()
{

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


// ----------- OctreeNode -------------------

OctreeNode::OctreeNode(NodeMetadata& metadata, std::string& points_path)
	: _node_id(metadata.node_id, NODE_ID_LENGHT), _half_size(metadata.half_size), _byte_offset(metadata.byte_offset),
	  _num_points(metadata.number_points), _points_binary_path(points_path)
{
	_center = glm::vec3(metadata.center[0], metadata.center[1], metadata.center[2]);
	_level = _node_id.length() - 1;	// Calculate the depth level from node id
}



OctreeNode::~OctreeNode()
{
	glDeleteBuffers(1, &VBO);
	glDeleteVertexArrays(1, &VAO);
}	


void OctreeNode::Draw(plane_t planes[6], glm::vec3 camera_pos)
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
	

	// Load point data to VBO
    glBufferData(GL_ARRAY_BUFFER, _points.size() * sizeof(point_data_t), _points.data(), GL_STATIC_DRAW);


	// Specify how to read Point coords in shader
    glEnableVertexAttribArray(0);	
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(point_data_t), (void*)0);
	// Specify how to read Point colors in shader
    glEnableVertexAttribArray(1);	
    glVertexAttribPointer(1, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(point_data_t), (void*)offsetof(point_data_t, color));

    std::cout << "[VBO DEBUG OCTREE | " << _node_id << "] Sizeof Vertex Struct : " << sizeof(point_data_t) << " bytes." << std::endl;
    std::cout << "[VBO DEBUG OCTREE | " << _node_id << "] Total Buffer Size: " << (_points.size() * sizeof(point_data_t) / 1024.0f) << " Kbytes." << std::endl;

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


