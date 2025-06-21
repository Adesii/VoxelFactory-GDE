#include "VoxelWorld.h"
#include "godot_cpp/classes/array_mesh.hpp"
#include "godot_cpp/classes/camera3d.hpp"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/mutex.hpp"
#include "godot_cpp/classes/node.hpp"
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/classes/thread.hpp"
#include "godot_cpp/classes/viewport.hpp"
#include "godot_cpp/classes/worker_thread_pool.hpp"
#include "godot_cpp/classes/world3d.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/core/memory.hpp"
#include "godot_cpp/core/object.hpp"
#include "godot_cpp/core/print_string.hpp"
#include "godot_cpp/core/property_info.hpp"
#include "godot_cpp/variant/array.hpp"
#include "godot_cpp/variant/callable.hpp"
#include "godot_cpp/variant/callable_method_pointer.hpp"
#include "godot_cpp/variant/node_path.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "godot_cpp/variant/variant.hpp"
#include "godot_cpp/variant/vector3.hpp"
#include "godot_cpp/variant/vector3i.hpp"
#include "mesh_result.h"
#include "mesher.h"
#include "util/MultiThreadQueues.h"
#include "voxel_chunk.h"
#include <array>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

VoxelWorld::VoxelWorld() {
	//threads.instantiate();
}
VoxelWorld::~VoxelWorld() {}

void VoxelWorld::_bind_methods() {
	ClassDB::bind_method(D_METHOD("generate"), &VoxelWorld::generate);
	ClassDB::bind_method(D_METHOD("gen_chunk", "position"), &VoxelWorld::gen_chunk);
	ClassDB::bind_method(D_METHOD("has_chunk", "position"), &VoxelWorld::has_chunk);

	ClassDB::bind_method(D_METHOD("set_view_distance", "view_distance"), &VoxelWorld::set_view_distance);
	ClassDB::bind_method(D_METHOD("get_view_distance"), &VoxelWorld::get_view_distance);

	ClassDB::bind_method(D_METHOD("set_noise", "noise"), &VoxelWorld::set_noise);
	ClassDB::bind_method(D_METHOD("get_noise"), &VoxelWorld::get_noise);

	ClassDB::bind_method(D_METHOD("set_biome_noise", "biome_noise"), &VoxelWorld::set_biome_noise);
	ClassDB::bind_method(D_METHOD("get_biome_noise"), &VoxelWorld::get_biome_noise);

	ClassDB::bind_method(D_METHOD("set_material", "material"), &VoxelWorld::set_material);
	ClassDB::bind_method(D_METHOD("get_material"), &VoxelWorld::get_material);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "view_distance"), "set_view_distance", "get_view_distance");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "noise", godot::PROPERTY_HINT_RESOURCE_TYPE, "FastNoiseLite"), "set_noise", "get_noise");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "biome_noise", godot::PROPERTY_HINT_RESOURCE_TYPE, "FastNoiseLite"), "set_biome_noise", "get_biome_noise");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "material", godot::PROPERTY_HINT_RESOURCE_TYPE, "Material"), "set_material", "get_material");
}
void VoxelWorld::init() {
	// Initialize the voxel world
	godot::UtilityFunctions::print("Initializing voxel world\n");
	//debug generate a 10x0x10 grid of chunks
	//for (int x = -view_distance; x <= view_distance; ++x) {
	//	for (int z = -view_distance; z <= view_distance; ++z) {
	//		gen_new_chunk_threaded_queue(Vector3i(x, 0, z));
	//	}
	//}
	set_physics_process(true);
	//generate();
}
void VoxelWorld::generate() {
	// Generate the voxel world
	uint32_t threadcount = 8;

	for (size_t i = 0; i < threadcount; ++i) {
		Ref<Thread> thread;
		thread.instantiate();
		//WorkerThreadPool::get_singleton()->add_native_task(VoxelWorld::generate_chunk_thread_loop,this);
		threads.push_back(thread);
		thread->start(callable_mp(this, &VoxelWorld::generate_chunk_thread_loop), Thread::PRIORITY_NORMAL);
		godot::UtilityFunctions::print("Thread ", i, " started\n");
	}
	//for (size_t i = 0; i < threadcount; ++i) {
	//	Ref<Thread> thread;
	//	thread.instantiate();
	//	//WorkerThreadPool::get_singleton()->add_native_task(VoxelWorld::generate_chunk_thread_loop,this);
	//	upperthreads.push_back(thread);
	//	thread->start(callable_mp(this, &VoxelWorld::upper_generate_chunk_thread_loop), Thread::PRIORITY_NORMAL);
	//	godot::UtilityFunctions::print("upperThread ", i, " started\n");
	//}
}

void VoxelWorld::gen_new_chunk_threaded_queue(Vector3i pos) {
	//godot::UtilityFunctions::print("Generating new chunk at ", pos, "\n");
	VoxelChunk *chunk = memnew(VoxelChunk);
	chunk->set_chunk_position(Vector3i(pos.x, pos.y, pos.z));
	chunk->set_world(get_world_3d());
	add_chunk(chunk);
}

void VoxelWorld::add_chunk(VoxelChunk *chunk) {
	//inactive_chunks.Push();
	chunk->_voxel_world = this;
	chunk->main_thread_init();
	chunks[chunk->chunk_position] = chunk;
	wait_tree_chunks.emplace_back(chunk);
	//godot::UtilityFunctions::print("Added chunk at ", chunk->chunk_position, "\n");
}
void VoxelWorld::queue_chunk(VoxelChunk *chunk) {
	//std::unique_lock<std::mutex> lock(mutex);
	inactive_chunks.Push(chunk);
}
void VoxelWorld::gen_chunk(const Vector3i &pos, World3D *world) {
	VoxelChunk *chunk = memnew(VoxelChunk);
	chunk->main_thread_init();
	chunk->set_chunk_position(pos);
	chunk->set_world(world);
	//mutex.lock();
	chunks[pos] = chunk;
	chunk->init(this); //TODO: Initialize the chunk
	//mutex.unlock();
}
bool VoxelWorld::has_chunk(const Vector3i &pos) const {
	return chunks.find(pos) != chunks.end();
}
float timesincence = 0.0f;

void VoxelWorld::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY:
			init();
			break;
		case NOTIFICATION_PHYSICS_PROCESS:
			auto delta = get_physics_process_delta_time();
			timesincence += delta;
			single_thread_generate(delta);
			break;
	}
}
bool generating = false;

float time_since_last_generated_chunk = 0.0f;

void VoxelWorld::single_thread_generate(float delta) {
	if (timesincence <= 0.1f)
		return;
	if (!generating) {
		generating = true;
		generate(); //TODO: figure out how to properly handle scene starts
	}
	for (size_t i = 0; i < wait_tree_chunks.size(); i++) {
		VoxelChunk *c = wait_tree_chunks[i];
		if (c) {
			queue_chunk(c);
			wait_tree_chunks.erase(wait_tree_chunks.begin() + i); // Remove the chunk from the list
			i--; // Adjust the index after erasing an element
		}
	}

	std::vector<VoxelChunk *> chunks_to_unload;
	Camera3D *cam = get_viewport()->get_camera_3d();
	Vector3i cam_position = cam->get_global_position() / VoxelChunk::ChunkSize;
	for (auto &child : chunks) {
		//print_line("Checking chunk at ", child.second->chunk_position);
		if (Vector3Util::DistanceXZTo(child.second->chunk_position, cam_position) >= float(view_distance) && child.second->get_mesh().is_valid()) {
			chunks_to_unload.emplace_back(child.second);
			//print_line("Unloading chunk at ", child.second->chunk_position);
			continue;
		}
	}
	for (VoxelChunk *chunk : chunks_to_unload) {
		chunks.erase(chunk->chunk_position);
		memdelete(chunk);
	}
	int pos_x = cam_position.x;
	int pos_z = cam_position.z;
	int x, y, dx, dy;
	x = y = dx = 0;
	dy = -1;
	int t = view_distance;
	int maxI = t * t;
	time_since_last_generated_chunk += delta;
	for (int i = 0; i < maxI; i++) {
		if ((-view_distance <= x) && (x <= view_distance) && (-view_distance <= y) && (y <= view_distance)) {
			auto pos = Vector3i(pos_x + x, 0, pos_z + y);
			//print_line("Checking chunk at ", pos, " iterator:", x, ",", y);
			if (!has_chunk(pos)) {
				gen_new_chunk_threaded_queue(pos);
			} else {
				VoxelChunk *chunk = chunks[pos];
				if (chunk->has_mesh()) {
					bool brrr = (-2 <= x) && (x <= 2) && (-2 <= y) && (y <= 2);
					//if (brrr && !chunk->is_collision_enabled()) {
					chunk->set_collision_enabled(brrr, time_since_last_generated_chunk);
					//} else if (!brrr) {
					//	chunk->set_collision_enabled(false, time_since_last_generated_chunk);
					//}
				} else {
					chunk->drop_collision();
				}
			}
		}

		if ((x == y) || ((x < 0) && (x == -y)) || ((x > 0) && (x == 1 - y))) {
			t = dx;
			dx = -dy;
			dy = t;
		}
		x += dx;
		y += dy;
	}
}

void VoxelWorld::generate_chunk_thread_loop() {
	//std::array<VoxelChunk *, 8> work_group;
	//ChunkMesher mesher;
	//Vector3i playerPos = Vector3i(0, 0, 0); //TODO: Get player position
	while (true) {
		//godot::UtilityFunctions::print("Generating chunks...\n");
		//for (size_t i = 0; i < 2; i++) {
		VoxelChunk *c = inactive_chunks.Pop();
		if (c == nullptr)
			continue;
		//auto chunk_pos = c->threaded_global_pos;
		//auto xDistance = std::abs(playerPos.x - chunk_pos.x) / VoxelChunk::ChunkSize;
		//auto zDistance = std::abs(playerPos.z - chunk_pos.z) / VoxelChunk::ChunkSize;
		//if (xDistance > view_distance || zDistance > view_distance)
		c->init(this);
		//else
		//    work_group[i]->call_deferred("queue_free");
		//}
		if (inactive_chunks.ShouldKillThread()) {
			break;
		}
	}
}

void VoxelWorld::set_view_distance(const int32_t p_view_distance) {
	view_distance = p_view_distance;
}
int32_t VoxelWorld::get_view_distance() const {
	return view_distance;
}

void VoxelWorld::set_noise(const Ref<FastNoiseLite> &p_noise) {
	noise = p_noise;
}
Ref<FastNoiseLite> VoxelWorld::get_noise() {
	return noise;
}

void VoxelWorld::set_biome_noise(const Ref<FastNoiseLite> &p_noise) {
	biome_noise = p_noise;
}
Ref<FastNoiseLite> VoxelWorld::get_biome_noise() {
	return biome_noise;
}

void VoxelWorld::set_material(const Ref<Material> &p_material) {
	material = p_material;
}
Ref<Material> VoxelWorld::get_material() {
	return material;
}

void VoxelWorld::add_modification(AABB mod_aabb, int block_type) {
	//Gather all affected chunks so that non-generated chunks know they need to change something before meshing
}
