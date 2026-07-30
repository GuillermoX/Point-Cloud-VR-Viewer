#include <LASReader.hpp>

bool LASReader::loadLASFile(std::vector<point_data_t>& vertex_buffer, const std::string& file_path, size_t max_points, glm::vec3& min, glm::vec3& max)
{
    min, max = { 0,0,0 };

    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[LAS Loader] Error: Could not open file: " << file_path << std::endl;
        return false;
    }

    // Read the header
    LASHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(LASHeader));

    // Validate LAS format
    if (std::string(header.file_signature, 4) != "LASF") {
        std::cerr << "[LAS Loader] Error: Invalid LAS file signature." << std::endl;
        return false;
    }

    std::cout << "[DEBUG HEADER] Signature: " << std::string(header.file_signature, 4) << std::endl;
    std::cout << "[DEBUG HEADER] Scale X: " << header.x_scale_factor << std::endl;
    std::cout << "[DEBUG HEADER] Offset X: " << header.x_offset << std::endl;
    std::cout << "[DEBUG HEADER] Point Format: " << (int)header.point_data_format << std::endl;

    std::cout << "[LAS Loader] Point Data Format: " << static_cast<int>(header.point_data_format) << std::endl;
    std::cout << "[LAS Loader] Bytes per point: " << header.point_data_record_length << std::endl;

    // Calculate total points in the file
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    size_t point_data_space = file_size - header.offset_to_point_data;
    size_t total_points = point_data_space / header.point_data_record_length;
    std::cout << "[LAS Loader] Total points detected: " << total_points << std::endl;
    if (max_points != 0)
    {
        total_points = max_points;
        std::cout << "[LAS Loader | INFO] Max number of points specified: " << max_points << std::endl; 
    }
    size_t total_points_quarter = total_points / 4;
    size_t total_points_half = total_points / 2;
    size_t total_points_three_quarters = total_points - (total_points / 4);


    // Jump to the beginning of point records
    file.seekg(header.offset_to_point_data, std::ios::beg);

    // Reserve memory in the vector to avoid re-allocations during the loop
    vertex_buffer.reserve(total_points);

    LASPointRecord2 raw_point;
    
    // Static variables to store the map origin anchor (for VR precision fix)
    bool is_first_point = true;
    double origin_x = 0.0;
    double origin_y = 0.0;
    double origin_z = 0.0;
    

    // 4. Point reading loop
    for (size_t i = 0; i < total_points; ++i) {
        file.read(reinterpret_cast<char*>(&raw_point), header.point_data_record_length);

        // Convert raw integers to real-world world coordinates (meters)
        double real_x = (raw_point.x * header.x_scale_factor) + header.x_offset;
        double real_y = (raw_point.y * header.y_scale_factor) + header.y_offset;
        double real_z = (raw_point.z * header.z_scale_factor) + header.z_offset;

        // Set the map anchor with the first point to prevent float jittering in VR
        if (is_first_point) {
            origin_x = real_x;
            origin_y = real_y;
            origin_z = real_z;
            is_first_point = false;
        }

        point_data_t processedVertex;
        
        // Center the coordinate system around (0, 0, 0) and cast to float
        processedVertex.position[0] = static_cast<float>(real_x - origin_x);
        processedVertex.position[1] = static_cast<float>(real_z - origin_z);
        processedVertex.position[2] = -(static_cast<float>(real_y - origin_y));

        // Normalize color channels from [0, 65535] to [0.0, 1.0] for OpenGL
        processedVertex.color[0] = raw_point.red >> 8;
        processedVertex.color[1] = raw_point.green >> 8;
        processedVertex.color[2] = raw_point.blue >> 8;

        vertex_buffer.push_back(processedVertex);

        if (i == total_points_quarter)
            std::cout << "[LAS Loader] Loaded 25% " << std::endl;
        else if (i == total_points_half)
            std::cout << "[LAS Loader] Loaded 50% " << std::endl;
        else if (i == total_points_three_quarters)
            std::cout << "[LAS Loader] Loaded 75% " << std::endl;

    }
    
    // Get bounding box
    max.x = header.max_x - origin_x;
    max.y = header.max_z - origin_z;
    max.z = -(header.max_y - origin_y);

    min.x = header.min_x - origin_x;
    min.y = header.min_z - origin_z;
    min.z = -(header.min_y - origin_y);

    file.close();
    std::cout << "[LAS Loader] Successfully loaded " << vertex_buffer.size() << " points." << std::endl;
    
    return true;
}
