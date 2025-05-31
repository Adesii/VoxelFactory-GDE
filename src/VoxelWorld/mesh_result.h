#pragma once

#include "godot_cpp/variant/array.hpp"
#include "godot_cpp/variant/vector3i.hpp"
struct meshResult {
public:
	godot::Array mesh_data;
	godot::Vector3i position;
	godot::Vector3i gen_new_pos;
	bool should_gen_new_chunk;
	meshResult(godot::Array mesh, godot::Vector3i pos, bool should_gen) :
			mesh_data(mesh), position(pos), should_gen_new_chunk(should_gen) {}
	~meshResult() {}
	meshResult() {}
};
