#include <PointCloud.hpp>

PointCloud::PointCloud(const std::string file_path, size_t max_points)
{
	LASReader las_reader;
    glm::vec3 min, max;
	points = las_reader.loadLASFile(file_path, max_points, min, max);
	if (!points.empty()) {
        std::cout << "[RENDER CHECK] Total vertices in vector: " << points.size() << std::endl;
    } else {
        std::cout << "[RENDER CHECK] ERROR: Vector is EMPTY!" << std::endl;
    }
    
    std::cout << "Min: (" << min.x << "," << min.y << "," << min.z << ")" << std::endl;
    std::cout << "Max: (" << max.x << "," << max.y << "," << max.z << ")" << std::endl;

    setupPointCloud();
}


void PointCloud::setupPointCloud()
{
	// Ensure Point Size Shader variable is active
	glEnable(GL_PROGRAM_POINT_SIZE);

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	
	// Load point data to VBO
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(point_data_t), points.data(), GL_STATIC_DRAW);

	// Specify how to read Point coords in shader
    glEnableVertexAttribArray(0);	
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(point_data_t), (void*)0);
	// Specify how to read Point colors in shader
    glEnableVertexAttribArray(1);	
    glVertexAttribPointer(1, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(point_data_t), (void*)offsetof(point_data_t, color));

    std::cout << "[VBO DEBUG] Sizeof Vertex Struct: " << sizeof(point_data_t) << " bytes." << std::endl;
    std::cout << "[VBO DEBUG] Total Buffer Size: " << (points.size() * sizeof(point_data_t) >> 20) << " Mbytes." << std::endl;

    glBindVertexArray(0);	
}

void PointCloud::Draw()
{
    glBindVertexArray(VAO);
    glDrawArrays(GL_POINTS, 0, points.size());
    glBindVertexArray(0);
}