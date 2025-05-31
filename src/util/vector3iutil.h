#pragma once

#include "godot_cpp/variant/vector3.hpp"
#include "godot_cpp/variant/vector3i.hpp"

class Vector3Hasher {
public:
	size_t operator()(const godot::Vector3i &v) const {
		return std::hash<int>()(v.x) ^ std::hash<int>()(v.y) ^ std::hash<int>()(v.z);
	}
};
class Vector3Util {
public:
	static inline float DistanceXZTo(const godot::Vector3i &v, const godot::Vector3i &Other) {
		return sqrt((v.x - Other.x) * (v.x - Other.x) + (v.z - Other.z) * (v.z - Other.z));
	}
};
