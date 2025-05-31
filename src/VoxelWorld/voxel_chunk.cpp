#include "voxel_chunk.h"
#include "VoxelWorld.h"
#include "godot_cpp/classes/array_mesh.hpp"
#include "godot_cpp/classes/mesh_instance3d.hpp"
#include "godot_cpp/classes/mutex.hpp"
#include "godot_cpp/classes/noise.hpp"
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/core/memory.hpp"
#include "godot_cpp/core/print_string.hpp"
#include "godot_cpp/variant/array.hpp"
#include "godot_cpp/variant/packed_float32_array.hpp"
#include "godot_cpp/variant/packed_int32_array.hpp"
#include "godot_cpp/variant/vector3.hpp"
#include "godot_cpp/variant/vector3i.hpp"
#include "mesh_result.h"
#include "mesher.h"
#include "util/MultiThreadQueues.h"
#include <mutex>
void VoxelChunk::_bind_methods() {
	// Bind methods here if needed
}

VoxelChunk::VoxelChunk() {
	mutex.instantiate();
	mesh.instantiate();
}
VoxelChunk::~VoxelChunk() {
}
void VoxelChunk::main_thread_init() {
	// Initialization logic that should be done on the main thread
	//mutex = memnew(Mutex);
	voxels.resize(ChunkSize_P3);
	//g_world->set_offset(Vector3i(chunk_position.x,chunk_position.z,0)*ChunkSize);
}

void VoxelChunk::init(VoxelWorld *w, Ref<FastNoiseLite> g_world, CompleteQueue<meshResult *> *finished,
		Vector3i pos) {
	PackedInt32Array local_voxels = PackedInt32Array();
	local_voxels.resize(ChunkSize_P3);
	Vector3i swiffled_pos = Vector3i((chunk_position.z * ChunkSize), (chunk_position.x * ChunkSize), 0);
	//g_world->set_offset(swiffled_pos);
	Vector3i chunk_above = Vector3i();
	bool should_gen_chunk_above = false;
	// Initialization logic for the voxel chunk
	//g_world->set_offset(Vector3i(chunk_position.x,chunk_position.z,0)*ChunkSize);
	for (size_t z = 0; z < ChunkSize_P; ++z) {
		for (size_t x = 0; x < ChunkSize_P; ++x) {
			float n = g_world->get_noise_2d(((float)swiffled_pos.x) + z, ((float)swiffled_pos.y) + x);
			for (size_t y = 0; y < ChunkSize_P; ++y) {
				size_t indexs = (z * ChunkSize_P + y) * ChunkSize_P + x;
				//size_t xy_index = (z * ChunkSize_P + x); // y * ChunkSize + x

				// Vector3I pos = Vector3I{(int)y, (int)x, (int)z}; // Chunk::ChunkSize_P * pos;

				if (indexs >= ChunkSize_P3) {
					continue;
				}

				float noise_point = ((n + 1.) / 2.) * 100 - pos.y * ChunkSize;
				if (noise_point >= ChunkSize) {
					//print_line("noise_point > y + ChunkSize_P ", noise_point, " , ", y + ChunkSize_P);
					//continue;
					chunk_above = Vector3i(chunk_position.x, chunk_position.y + 1, chunk_position.z);
					should_gen_chunk_above = true;
				}

				if (noise_point <= y) {
					(local_voxels)[indexs] = { 0 };
					continue;
				}
				if (noise_point <= y + 1) {
					//float othern = noiseOutputbiomemap.at(xy_index);
					//if (othern < 0.5)
					(local_voxels)[indexs] = { 1 };
					//else
					//    (voxels)[indexs] = {2};
					continue;
				}
				(local_voxels)[indexs] = { 2 };
				/*
								if (pos.DistanceXZTo(Vector3I{30, 30, 30}) < 15)
								{
									(voxels)[indexs] = {2};
								}
								else
								{
									(voxels)[indexs] = {0};
								} */
			}
		}
	}

	ChunkMesher mesher;
	meshResult *result = memnew(meshResult);
	if (should_gen_chunk_above) {
		//print_line("Generating chunk above for position: ", chunk_above);
		//generate_chunk_above(voxels, noiseOutputbiomemap, mesh);
		w->gen_chunk(chunk_above);
		//result->gen_new_pos = chunk_above;
		result->should_gen_new_chunk = false;
	} else {
		result->should_gen_new_chunk = false;
		result->gen_new_pos = Vector3i(0, 0, 0);
	}
	bool has_verts = false;
	result->mesh_data = mesher.MeshChunk(local_voxels, mesh, has_verts);
	result->position = get_chunk_position();
	//result->position = chunk_position;
	//result->mesh_data =;
	//mutex->lock();
	if (has_verts)
		finished->Push(result);

	//generated = true;
	//mutex->unlock();
}

void VoxelChunk::_notification(int what) {
	/* switch (what) {
		case NOTIFICATION_PHYSICS_PROCESS:
			//{
			//	std::lock_guard<std::mutex> lock(mutex);
			//	if (generated) {
			//		set_mesh(mesh);
			//		generated = false;
			//	}
			//}
			break;

	} */
}
