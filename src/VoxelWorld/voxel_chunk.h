#pragma once

#include "godot_cpp/classes/array_mesh.hpp"
#include "godot_cpp/classes/fast_noise_lite.hpp"
#include "godot_cpp/classes/mesh_instance3d.hpp"
#include "godot_cpp/classes/mutex.hpp"
#include "godot_cpp/classes/visual_instance3d.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/core/memory.hpp"
#include "godot_cpp/variant/packed_int32_array.hpp"
#include "godot_cpp/variant/vector3.hpp"
#include "godot_cpp/variant/vector3i.hpp"
#include "mesh_result.h"
#include "util/MultiThreadQueues.h"
#include "voxel.h"
#include <mutex>
using namespace godot;

class VoxelWorld;
class VoxelChunk : public MeshInstance3D {
	GDCLASS(VoxelChunk, MeshInstance3D)
protected:
	static void _bind_methods();
	PackedInt32Array voxels;
	Ref<Mutex> mutex;
	bool generated = false;

public:
	Ref<ArrayMesh> mesh;
	Vector3 threaded_global_pos = Vector3(0, 0, 0);
	VoxelChunk();
	~VoxelChunk();

	static const size_t ChunkSize = 62; // Number of blocks in a direction inside of a chunk
	static const size_t ChunkSize_P = ChunkSize + 2;
	static const size_t ChunkSize_P2 = ChunkSize_P * ChunkSize_P;
	static const size_t ChunkSize_P3 = ChunkSize_P * ChunkSize_P * ChunkSize_P;

	Vector3i chunk_position; // Position of the chunk in world space
	void set_chunk_position(Vector3i pos) {
		mutex->lock();
		chunk_position = pos;
		set_global_position(pos * ChunkSize);
		threaded_global_pos = pos * ChunkSize;
		mutex->unlock();
	}
	Vector3i get_chunk_position() const {
		//mutex->lock();
		auto ret = chunk_position;
		//mutex->unlock();
		return ret;
	}

	void init(VoxelWorld *w, Ref<FastNoiseLite> g_world, CompleteQueue<meshResult *> *finished, Vector3i pos);

	void main_thread_init();

	void _notification(int what);
};
