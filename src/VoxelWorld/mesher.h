#pragma once
#include "godot_cpp/classes/immediate_mesh.hpp"
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/variant/array.hpp"
#include "voxel.h"
#include "voxel_chunk.h"
#include <array>
#include <cstdint>
typedef uint64_t INTSIZE;

class ChunkMesher {
public:
	// void add_voxels_to_axis_cols(Voxel voxel, Vector3 pos, std::array<std::array<std::array<INTSIZE, Chunk::ChunkSize_P>, Chunk::ChunkSize_P>, 3> *axis_cols);
	void MeshChunk(Array &mesh_data, std::vector<uint32_t> &voxels, Ref<ArrayMesh> &mesh, bool &hasverts);
	// void MeshChunkCulled(std::vector<Voxel> *voxels);
	// Chunk::ChunkMesh *MeshChunkCulled(std::vector<Voxel> *voxels);
};
