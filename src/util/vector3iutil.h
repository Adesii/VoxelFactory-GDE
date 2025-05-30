#pragma once



#include "godot_cpp/variant/vector3i.hpp"

class Vector3Hasher {
public:
    size_t operator()(const godot::Vector3i& v) const {
        return std::hash<int>()(v.x) ^ std::hash<int>()(v.y) ^ std::hash<int>()(v.z);
    }
};
