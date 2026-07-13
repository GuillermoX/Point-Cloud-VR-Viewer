#include <Octree.hpp>

// ------------ Octree ----------------
Octree::Octree(std::vector<point_data_t>& points, glm::vec3& min, glm::vec3& max)
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

	_root_node = std::make_unique<OctreeNode>(center, half_node_size);
	
	for (point_data_t point : points)
	{
		_root_node->AddPoint(point);
	}

	_root_node->InitGraphicBuffers();
}

void Octree::Draw(const glm::mat4& view_proj)
{
	_root_node->Draw(view_proj);
}

// ----------- OctreeNode -------------------

OctreeNode::OctreeNode(glm::vec3 center, float half_node_size, unsigned int max_points)
	: _center(center), _half_node_size(half_node_size), _max_points(max_points)
{

}

OctreeNode::OctreeNode(glm::vec3 center, float half_node_size, unsigned char index, unsigned int max_points)
	: _max_points(max_points)
{
	if (index == 0)
		_center = center + glm::vec3(-half_node_size);
	else if (index == 1)
		_center = center + glm::vec3(half_node_size, -half_node_size, -half_node_size);
	else if (index == 2)
		_center = center + glm::vec3(-half_node_size, half_node_size, -half_node_size);
	else if (index == 3)
		_center = center + glm::vec3(half_node_size, half_node_size, -half_node_size);
	else if (index == 4)
		_center = center + glm::vec3(-half_node_size, -half_node_size, half_node_size);
	else if (index == 5)
		_center = center + glm::vec3(half_node_size, -half_node_size, half_node_size);
	else if (index == 6)
		_center = center + glm::vec3(-half_node_size, half_node_size, -half_node_size);
	else if (index == 7)
		_center = center + glm::vec3(half_node_size);
	else
		_center = center;

	_half_node_size = half_node_size / 2;
	
}

OctreeNode::~OctreeNode()
{
	glDeleteBuffers(1, &VBO);
	glDeleteVertexArrays(1, &VAO);
}
	


void OctreeNode::AddPoint(point_data_t& point)
{
	if (_points.size() < _max_points)
		_points.push_back(point);
	else
		addToSubnode(point);
}


void OctreeNode::Draw(const glm::mat4& view_proj)
{
	if (isInsideFrustrum(view_proj))
	{	
		glBindVertexArray(VAO);
		glDrawArrays(GL_POINTS, 0, _points.size());
		glBindVertexArray(0);
		for (int i = 0; i < 8; i++)
		{
			if (_subnodes[i] != nullptr)
				_subnodes[i]->Draw(view_proj);
		}
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

    std::cout << "[VBO DEBUG] Sizeof Vertex Struct: " << sizeof(point_data_t) << " bytes." << std::endl;
    std::cout << "[VBO DEBUG] Total Buffer Size: " << (_points.size() * sizeof(point_data_t) / 1024.0f) << " Kbytes." << std::endl;

    glBindVertexArray(0);	
	
	// Init all buffers of active subnodes
	for (int i = 0; i < 8; i++)
	{
		if (_subnodes[i] != nullptr)
			_subnodes[i]->InitGraphicBuffers();
	}
		
	
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
		_subnodes[index] = std::make_unique<OctreeNode>(_center, _half_node_size, index, _max_points);

	_subnodes[index]->AddPoint(point);
}

bool OctreeNode::isInsideFrustrum(glm::mat4& vp)
{
	plane_t planes[6];

	// Right plane
	planes[0].x = vp[0][3] - vp[0][0];
	planes[0].y = vp[1][3] - vp[1][0];
	planes[0].z = vp[2][3] - vp[2][0];
	planes[0].d = vp[3][3] - vp[3][0];

	// Left plane
	planes[1].x = vp[0][3] + vp[0][0];
	planes[1].y = vp[1][3] + vp[1][0];
	planes[1].z = vp[2][3] + vp[2][0];
	planes[1].d = vp[3][3] + vp[3][0];

	// Bottom plane
	planes[2].x = vp[0][3] + vp[0][1];
	planes[2].y = vp[1][3] + vp[1][1];
	planes[2].z = vp[2][3] + vp[2][1];
	planes[2].d = vp[3][3] + vp[3][1];

	// Top plane
	planes[3].x = vp[0][3] - vp[0][1];
	planes[3].y = vp[1][3] - vp[1][1];
	planes[3].z = vp[2][3] - vp[2][1];
	planes[3].d = vp[3][3] - vp[3][1];

	// Far plane
	planes[4].x = vp[0][3] - vp[0][2];
	planes[4].y = vp[1][3] - vp[1][2];
	planes[4].z = vp[2][3] - vp[2][2];
	planes[4].d = vp[3][3] - vp[3][2];

	// Near plane
	planes[5].x = vp[0][3] + vp[0][2];
	planes[5].y = vp[1][3] + vp[1][2];
	planes[5].z = vp[2][3] + vp[2][2];
	planes[5].d = vp[3][3] + vp[3][2];
}


