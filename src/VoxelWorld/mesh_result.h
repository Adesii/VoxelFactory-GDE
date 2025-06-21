#pragma once

#include "godot_cpp/variant/array.hpp"
#include "godot_cpp/variant/vector3i.hpp"
struct meshResult {
public:
	godot::Array mesh_data;
	godot::Vector3i position;
	meshResult(godot::Array mesh, godot::Vector3i pos) :
			mesh_data(mesh), position(pos) {}
	~meshResult() {}
	meshResult() {}
};
