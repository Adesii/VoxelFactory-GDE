#pragma once
#include <cstdint>
#include <sys/stat.h>
#include "godot_cpp/variant/vector3.hpp"
using namespace godot;
struct voxel_vertex_t
{
    // uint32_t data; // Packed data
    //  data is stored as follows: xyz(6 bits each) + normal 6 directions (3 bits total) + block type (4 bits)
    uint32_t data;
    inline static const uint32_t MakeVertex(const Vector3 &pos, const uint32_t &ao, const uint32_t &normal, const uint32_t &block_type)
    {
        // TODO: only use the first xyz for now. change later to use the rest
        return static_cast<uint32_t>(pos.x) | static_cast<uint32_t>(pos.y) << 6 | static_cast<uint32_t>(pos.z) << 12 | static_cast<uint32_t>(ao) << 18 | static_cast<uint32_t>(normal) << 21 | static_cast<uint32_t>(block_type) << 25;
        // return voxel_vertex_t{pos};
    }
    // delete the default constructor
    voxel_vertex_t() = delete;

    voxel_vertex_t(const Vector3 &pos, const uint32_t &ao, const uint32_t &normal, const uint32_t &block_type)
    {
        data = static_cast<uint32_t>(pos.x) | static_cast<uint32_t>(pos.y) << 6 | static_cast<uint32_t>(pos.z) << 12 | static_cast<uint32_t>(ao) << 18 | static_cast<uint32_t>(normal) << 21 | static_cast<uint32_t>(block_type) << 25;
    }
};
struct Voxel
{
    uint16_t block_type; // Type of the block (e.g., air, stone, etc.)
};
