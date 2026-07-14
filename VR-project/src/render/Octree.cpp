#include <Octree.hpp>

// ------------ Octree ----------------
Octree::Octree(std::vector<point_data_t>& points, glm::vec3& min, glm::vec3& max, float cell_size)
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

	_root_node = std::make_unique<OctreeNode>(center, half_node_size, cell_size);
	
	for (point_data_t point : points)
	{
		_root_node->AddPoint(point);
	}

	_root_node->InitGraphicBuffers();
}

void Octree::Draw(const glm::mat4& view_proj)
{
	plane_t planes[6];
	getFrustrumPlanes(view_proj, planes);
	_root_node->Draw(planes);
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

OctreeNode::OctreeNode(glm::vec3 center, float half_node_size, float cell_size)
	: _center(center), _half_node_size(half_node_size), _grid(cell_size)
{

}

OctreeNode::OctreeNode(glm::vec3 center, float half_node_size, unsigned char index, float cell_size)
	: _grid(cell_size / 5.0f)
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
	
}

OctreeNode::~OctreeNode()
{
	glDeleteBuffers(1, &VBO);
	glDeleteVertexArrays(1, &VAO);
}
	


void OctreeNode::AddPoint(point_data_t& point)
{
	// Try adding point, if not possible delegate to subnode
	if (!_grid.AddPoint(point))
	{
		addToSubnode(point);
	}
}


void OctreeNode::Draw(plane_t planes[6])
{
	if (isInsideFrustrum(planes))
	{	
		glBindVertexArray(VAO);
		glDrawArrays(GL_POINTS, 0, _grid.num_points);
		glBindVertexArray(0);
		for (int i = 0; i < 8; i++)
		{
			if (_subnodes[i] != nullptr)
				_subnodes[i]->Draw(planes);
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
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	
	std::vector<point_data_t> points_vector;
	_grid.PrepareToRender(points_vector);

	// Load point data to VBO
    glBufferData(GL_ARRAY_BUFFER, points_vector.size() * sizeof(point_data_t), points_vector.data(), GL_STATIC_DRAW);

	// Specify how to read Point coords in shader
    glEnableVertexAttribArray(0);	
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(point_data_t), (void*)0);
	// Specify how to read Point colors in shader
    glEnableVertexAttribArray(1);	
    glVertexAttribPointer(1, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(point_data_t), (void*)offsetof(point_data_t, color));

    std::cout << "[VBO DEBUG] Sizeof Vertex Struct: " << sizeof(point_data_t) << " bytes." << std::endl;
    std::cout << "[VBO DEBUG] Total Buffer Size: " << (points_vector.size() * sizeof(point_data_t) / 1024.0f) << " Kbytes." << std::endl;

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
		// Create it with the center of corresponding quarter
		_subnodes[index] = std::make_unique<OctreeNode>(_center, _half_node_size, index, _grid.GetCellSize());

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



// --------------- Support class: Grid -----------------------

Grid::Grid(float cell_size)
	: _cell_size(cell_size)
{
}

bool Grid::AddPoint(const point_data_t& point)
{
	
	GridKey key;
	key.x = std::floor(point.position[0] / _cell_size);
	key.y = std::floor(point.position[1] / _cell_size);
	key.z = std::floor(point.position[2] / _cell_size);

	// If cell is empty store the point in it
	if (hash_map.find(key) == hash_map.end()) {
		hash_map[key] = point;
		return true;	// Notify that the point has been stored
	}
	

	return false;
}

void Grid::PrepareToRender(std::vector<point_data_t>& vector)
{
	vector.clear();
	vector.reserve(hash_map.size()); // Reservamos memoria de golpe

	// Copy the points from hashmap to vector
	for (const auto& [key, punto] : hash_map) {
		vector.push_back(punto);
	}
	
	// Store number of points stored to know when rendering
	num_points = hash_map.size();
	// Free the hashmap data
	hash_map.clear();
}

size_t Grid::size()
{
	return hash_map.size();
}


float Grid::GetCellSize()
{
	return _cell_size;
}

