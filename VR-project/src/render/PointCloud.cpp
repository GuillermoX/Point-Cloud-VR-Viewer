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

    octree_ptr = std::make_unique<Octree>(points, min, max);


    
}


void PointCloud::Draw(const glm::mat4& view_proj)
{
    octree_ptr->Draw(view_proj);
}

void PointCloud::Debug(const glm::mat4& vpm)
{
    octree_ptr->Debug(vpm); 
}
