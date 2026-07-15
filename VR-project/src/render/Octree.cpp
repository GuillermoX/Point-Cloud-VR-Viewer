#include <Octree.hpp>

// ------------ Octree ----------------
Octree::Octree(std::vector<point_data_t>& points, glm::vec3& min, glm::vec3& max, unsigned int grid_res)
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

	_root_node = std::make_unique<OctreeNode>(center, half_node_size, grid_res, 0);
	
	unsigned int quarter_vector = points.size() / 4;
	unsigned int half_vector = points.size() / 2;
	unsigned int three_quarters_vector = points.size() - quarter_vector;
	for (unsigned int i = 0; i < points.size(); i++)
	{
		if (i == quarter_vector)
			std::cout << "[OCTREE | INFO] Loaded 25% of points." << std::endl;
		else if (i == half_vector)
			std::cout << "[OCTREE | INFO] Loaded 50% of points." << std::endl;
		else if (i == three_quarters_vector)
			std::cout << "[OCTREE | INFO] Loaded 75% of points." << std::endl;

		_root_node->AddPoint(points[i]);
	}

	_root_node->InitGraphicBuffers();
}

void Octree::Draw(const glm::mat4& view_proj, glm::vec3 camera_pos)
{
	plane_t planes[6];
	getFrustrumPlanes(view_proj, planes);
	_root_node->Draw(planes, camera_pos);
}

void Octree::Debug(const glm::mat4& vpm)
{
	_root_node->debug_shader->use();
    glUniformMatrix4fv(glGetUniformLocation(_root_node->debug_shader->ID, "vpm"), 1, false, glm::value_ptr(vpm));
	plane_t planes[6];
	getFrustrumPlanes(vpm, planes);
	_root_node->Debug(planes, vpm);
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


// ----------- OctreeNode -------------------

OctreeNode::OctreeNode(glm::vec3 center, float half_node_size, unsigned int grid_res, unsigned int level)
	: _num_points(0), _center(center), _half_node_size(half_node_size), _level(level)
{
	_grid_ptr = std::make_unique<Grid>(grid_res, (_half_node_size * 2.0) / (float)grid_res, _center - glm::vec3(half_node_size));
}

OctreeNode::OctreeNode(glm::vec3 center, float half_node_size, unsigned char index, unsigned int grid_res, unsigned int level)
	: _num_points(0), _level(level)
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

	_half_node_size = quarter_node_size;	

	_grid_ptr = std::make_unique<Grid>(grid_res, (_half_node_size * 2.0) / (float)grid_res, _center - glm::vec3(half_node_size));
}

OctreeNode::~OctreeNode()
{
	glDeleteBuffers(1, &VBO);
	glDeleteVertexArrays(1, &VAO);
}
	


void OctreeNode::AddPoint(point_data_t& point)
{
	// Try adding point, if not possible delegate to subnode
	// Limit the maximum level of depth of subnodes
	if (_grid_ptr->FreeGridCell(point) || _level >= 4)
	{
		_points.push_back(point);
		_num_points++;
	}
	else	
		addToSubnode(point);
}


void OctreeNode::Draw(plane_t planes[6], glm::vec3 camera_pos)
{
	if (isInsideFrustrum(planes) && levelOfDetail(camera_pos))
	{	
		glBindVertexArray(VAO);
		glDrawArrays(GL_POINTS, 0, _num_points);
		glBindVertexArray(0);
		for (int i = 0; i < 8; i++)
		{
			if (_subnodes[i] != nullptr)
				_subnodes[i]->Draw(planes, camera_pos);
		}
	}
}

void OctreeNode::Debug(plane_t planes[6], const glm::mat4& vpm)
{
	if (isInsideFrustrum(planes))
		glUniform3f(glGetUniformLocation(debug_shader->ID, "Color"), 0.0, 1.0, 0.0);
	else
		glUniform3f(glGetUniformLocation(debug_shader->ID, "Color"), 1.0, 0.0, 0.0);

	glm::mat4 insideModel = glm::mat4(1.0f);
	insideModel = glm::translate(insideModel, _center);
	insideModel = glm::scale(insideModel, glm::vec3(_half_node_size * 2.0f));
	glUniformMatrix4fv(glGetUniformLocation(debug_shader->ID, "insideModel"), 1, false, glm::value_ptr(insideModel));

	glBindVertexArray(debug_VAO);
	glDrawArrays(GL_LINES, 0, 24);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	
	for (int i = 0; i < 8; i++)
	{
		if (_subnodes[i] != nullptr)
			_subnodes[i]->Debug(planes, vpm);
	}
}

void OctreeNode::InitGraphicBuffers()
{
	_grid_ptr->DeleteData();		// Delete grid control arrays

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

    std::cout << "[VBO DEBUG] Sizeof Vertex Struct: " << sizeof(point_data_t) << " bytes." << std::endl;
    std::cout << "[VBO DEBUG] Total Buffer Size: " << (_points.size() * sizeof(point_data_t) / 1024.0f) << " Kbytes." << std::endl;

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

void OctreeNode::addToSubnode(point_data_t& point)
{
	unsigned char index = 0;
	if (point.position[0] > _center.x)
		index |= 1;		// First bit to one

	if (point.position[1] > _center.y)
		index |= 2;		// Second bit to one

	if (point.position[2] > _center.z)
		index |= 4;		// Third bit to one
	
	// Create Octree Node if not created
	if (_subnodes[index] == nullptr)
	{
		// Create it with the center of corresponding quarter
		_subnodes[index] = std::make_unique<OctreeNode>(_center, _half_node_size, index, _grid_ptr->GetGridResolution(), _level + 1);
	}

	_subnodes[index]->AddPoint(point);
}

bool OctreeNode::isInsideFrustrum(plane_t planes[6])
{	
	for (int i = 0; i < 6; i++) {
		plane_t plane = planes[i];

		// Find point of node nearest to the plane
		glm::vec3 p = _center;
		if (plane.normal.x >= 0) p.x += _half_node_size; else p.x -= _half_node_size;
		if (plane.normal.y >= 0) p.y += _half_node_size; else p.y -= _half_node_size;
		if (plane.normal.z >= 0) p.z += _half_node_size; else p.z -= _half_node_size;

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
	unsigned int max_level;
	if (distance <= 2.0f)
		max_level = 4;
	else if (distance <= 5.0f)
		max_level = 3;
	else
		max_level = 2;

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


