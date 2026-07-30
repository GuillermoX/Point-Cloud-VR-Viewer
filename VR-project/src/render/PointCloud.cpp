#include <PointCloud.hpp>

PointCloud::PointCloud(const std::string file_path, unsigned int grid_res, bool create_bin, size_t max_points_bin, bool& point_cloud_created)
{
    // Check if octree preprocessed binaries available
    std::filesystem::path path = file_path;
    std::string meta_path = path.replace_extension(".meta").string();
    std::string points_path = path.replace_extension(".points").string();
	std::string las_path = path.replace_extension(".las").string();
    // If any of the binary octree files don't exist or create_bin=true create full octree and binaries
    if (create_bin ||(!std::filesystem::exists(meta_path) || !std::filesystem::exists(points_path)))
    {	
		// Read original .las file to get raw points
		LASReader las_reader;
		glm::vec3 min, max;
		if (!las_reader.loadLASFile(points, las_path, max_points_bin, min, max))
		{
			point_cloud_created = false;
			return;
		}
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
	bool octree_created;
	octree_ptr = std::make_unique<Octree>(meta_path, points_path, octree_created);
	if (octree_created)
		point_cloud_created = true;
	else
		point_cloud_created = false;
}

void PointCloud::Draw(const glm::mat4& view_proj, glm::vec3 camera_pos, float delta_time, Shader& shader)
{
    octree_ptr->Draw(view_proj, camera_pos, delta_time, shader);
}

void PointCloud::Debug(const glm::mat4& vpm, glm::vec3 camera_pos)
{
    octree_ptr->Debug(vpm, camera_pos); 
}
