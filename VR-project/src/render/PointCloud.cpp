#include <PointCloud.hpp>

PointCloud::PointCloud(const std::string file_path, size_t max_points, unsigned int grid_res)
{
    // Check if octree preprocessed binaries available
    std::filesystem::path path = file_path;
    std::string meta_path = path.replace_extension(".meta").string();
    std::string points_path = path.replace_extension(".points").string();
    // If any of the binary octree files don't exist create full octree and binaries
    if (!std::filesystem::exists(meta_path) || !std::filesystem::exists(points_path))
    {	
		// Read original .las file to get raw points
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
		
		// Create octree builder structure from raw point data
		OctreeBuilder octree_builder = OctreeBuilder(points, min, max, grid_res);    
		
		// Generate octree preprocessed binaries
        octree_builder.CreateOctreeBinary(meta_path, points_path);
    }
	
	// Create rendering octree structure
	octree_ptr = std::make_unique<Octree>(meta_path, points_path);

}

void PointCloud::Draw(const glm::mat4& view_proj, glm::vec3 camera_pos)
{
    octree_ptr->Draw(view_proj, camera_pos);
}

void PointCloud::Debug(const glm::mat4& vpm, glm::vec3 camera_pos)
{
    octree_ptr->Debug(vpm, camera_pos); 
}
