#ifndef LASREADER_H
#define LASREADER_H

#include <stdint.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#include <glm/glm.hpp>

#pragma pack(push, 1)
struct LASHeader {
    char     file_signature[4];       // Always "LASF" signature
    uint16_t file_source_id;
    uint16_t global_encoding;
    uint32_t project_id_guid_data_1;
    uint16_t project_id_guid_data_2;
    uint16_t project_id_guid_data_3;
    uint8_t  project_id_guid_data_4[8];
    uint8_t  version_major;          
    uint8_t  version_minor;           
    char     system_identifier[32];
    char     generating_software[32];
    uint16_t file_creation_day_of_year;
    uint16_t file_creation_year;
    uint16_t header_size;
    
    uint32_t offset_to_point_data;    // Where point data starts
    uint32_t number_of_variable_length_records;
    
    uint8_t  point_data_format;       // Tells the format (1, 2, 3...)
    uint16_t point_data_record_length;// Size of each point
    
    uint32_t number_of_points;
    uint32_t number_of_points_by_return[5]; // Deprecated in newer versions
    
    // Conversion Factor
    double   x_scale_factor;
    double   y_scale_factor;
    double   z_scale_factor;
    double   x_offset;
    double   y_offset;
    double   z_offset;
    
    // Bounding Box
    double   max_x;
    double   min_x;
    double   max_y;
    double   min_y;
    double   max_z;
    double   min_z;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct LASPointRecord2 {
    int32_t  x, y, z;          // Raw coordinates (unsigned int of 32 bits)
    uint16_t intensity;        // Intensity of laser
    uint8_t  return_flags;     // Return data
    uint8_t  classification;   // Type of object
    int8_t   scan_angle_rank;
    uint8_t  user_data;
    uint16_t point_source_id;
    
    uint16_t red;
    uint16_t green;
    uint16_t blue;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct point_data_t
{
    float position[3];
    uint8_t color[3];
};
#pragma pack(pop)


class LASReader
{
public:
    std::vector<point_data_t> loadLASFile(const std::string& path, size_t max_points, glm::vec3& min, glm::vec3& max);
};


#endif