#include "VoxelWorld.h"
#include "godot_cpp/classes/array_mesh.hpp"
#include "godot_cpp/classes/mutex.hpp"
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/classes/thread.hpp"
#include "godot_cpp/classes/worker_thread_pool.hpp"
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

VoxelWorld::VoxelWorld() {
	chunk_mutex.instantiate();
	//threads.instantiate();
}
VoxelWorld::~VoxelWorld() {}

void VoxelWorld::_bind_methods() {
	ClassDB::bind_method(D_METHOD("generate"), &VoxelWorld::generate);
	ClassDB::bind_method(D_METHOD("gen_chunk", "position"), &VoxelWorld::gen_chunk);
	ClassDB::bind_method(D_METHOD("add_chunk", "chunk"), &VoxelWorld::add_chunk);
	ClassDB::bind_method(D_METHOD("has_chunk", "position"), &VoxelWorld::has_chunk);

	ClassDB::bind_method(D_METHOD("set_view_distance", "view_distance"), &VoxelWorld::set_view_distance);
	ClassDB::bind_method(D_METHOD("get_view_distance"), &VoxelWorld::get_view_distance);
	ClassDB::bind_method(D_METHOD("set_noise", "noise"), &VoxelWorld::set_noise);
	ClassDB::bind_method(D_METHOD("get_noise"), &VoxelWorld::get_noise);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "view_distance"), "set_view_distance", "get_view_distance");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "noise", godot::PROPERTY_HINT_RESOURCE_TYPE, "FastNoiseLite"), "set_noise", "get_noise");
}
void VoxelWorld::init() {
	// Initialize the voxel world
	godot::UtilityFunctions::print("Initializing voxel world\n");
	//debug generate a 10x0x10 grid of chunks
	for (int x = -view_distance; x <= view_distance; ++x) {
		for (int z = -view_distance; z <= view_distance; ++z) {
			gen_new_chunk_threaded_queue(Vector3i(x, 0, z));
		}
	}
	set_physics_process(true);
	//generate();
}
void VoxelWorld::generate() {
	// Generate the voxel world
	uint32_t threadcount = 4;

	for (size_t i = 0; i < threadcount; ++i) {
		Ref<Thread> thread;
		thread.instantiate();
		//WorkerThreadPool::get_singleton()->add_native_task(VoxelWorld::generate_chunk_thread_loop,this);
		threads.push_back(thread);
		thread->start(callable_mp(this, &VoxelWorld::generate_chunk_thread_loop), Thread::PRIORITY_NORMAL);
		godot::UtilityFunctions::print("Thread ", i, " started\n");
	}
}

void VoxelWorld::gen_new_chunk_threaded_queue(Vector3i pos) {
	godot::UtilityFunctions::print("Generating new chunk at ", pos, "\n");
	VoxelChunk *chunk = memnew(VoxelChunk);
	add_child(chunk);
	chunk->set_chunk_position(Vector3i(pos.x, pos.y, pos.z));
	chunk->set_name("Chunk_" + Variant(pos.x).stringify() + "_" + Variant(pos.y).stringify() + "_" + Variant(pos.z).stringify());
	add_chunk(chunk);
}

void VoxelWorld::add_chunk(VoxelChunk *chunk) {
	//inactive_chunks.Push();
	chunk->main_thread_init();
	chunks[chunk->chunk_position] = chunk->get_path();
	wait_tree_chunks.emplace_back(chunk);
	godot::UtilityFunctions::print("Added chunk at ", chunk->chunk_position, "\n");
}
void VoxelWorld::queue_chunk(VoxelChunk *chunk) {
	std::unique_lock<std::mutex> lock(mutex);
	inactive_chunks.Push(chunk);
}
void VoxelWorld::gen_chunk(const Vector3i &pos) {
	VoxelChunk *chunk = memnew(VoxelChunk);
	chunk->main_thread_init();
	chunk->set_chunk_position(pos);
	if (chunks.try_emplace(pos, chunk->get_path()).second) {
		chunk->init(this, noise, &Finished_chunks, pos); //TODO: Initialize the chunk

		add_child(chunk);
	}
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
			timesincence += get_physics_process_delta_time();
			single_thread_generate();
			break;
	}
}
bool generating = false;

void VoxelWorld::single_thread_generate() {
	if (!generating && timesincence > 0.1f) {
		//godot::UtilityFunctions::print("Generating chunks... single thread\n");
		generating = true;
		generate(); //TODO: figure out how to properly handle scene starts
		//VoxelChunk* c = inactive_chunks.Pop();
		//if (c == nullptr){
		//	generating = false;
		//    return;
		//}
		//c->init(this, noise);
		//generating = false;

		//if (inactive_chunks.Count() > 0) {
		//	VoxelChunk* c = inactive_chunks.Pop();
		//	c->init(this, noise, Finished_chunks); //TODO: Initialize the chunk
		//}
	}
	for (size_t i = 0; i < wait_tree_chunks.size(); i++) {
		VoxelChunk *c = wait_tree_chunks[i];
		if (c->is_inside_tree() && c->is_node_ready() && c->is_visible_in_tree()) {
			queue_chunk(c);
			wait_tree_chunks.erase(wait_tree_chunks.begin() + i); // Remove the chunk from the list
			i--; // Adjust the index after erasing an element
		}
	}
	if (Finished_chunks.Count() > 0) {
		meshResult *c = memnew(meshResult(Array(), Vector3i(0, 0, 0), false));
		Finished_chunks.Pop(c);
		if (chunks.find(c->position) == chunks.end()) {
			print_line("chunk not found in chunks map");
			return;
		}
		NodePath chunkPath = chunks.at(c->position);
		//if(chunkPath.is_empty()){
		//	print_line("chunk path is empty");
		//	return;
		//}
		print_line("chunk found in chunks map at " + chunkPath);
		VoxelChunk *chunk = get_node<VoxelChunk>(chunkPath);
		if (chunk) {
			chunk->mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, c->mesh_data);
			chunk->set_mesh(chunk->mesh);
			if (c->should_gen_new_chunk) {
				gen_new_chunk_threaded_queue(c->gen_new_pos);
			}

			print_line("chunk generated and set mesh");
		}
	}
	/* std::array<VoxelChunk*,8> work_group;
	Vector3i playerPos = Vector3i(0,0,0);//TODO: Get player position
		godot::UtilityFunctions::print("Generating chunks...\n");
		for (size_t i = 0; i < 8; i++)
		{
			VoxelChunk* c = inactive_chunks.Pop();
			if (c == nullptr)
				continue;
			auto chunk_pos = c->get_global_position();
			auto xDistance = std::abs(playerPos.x - chunk_pos.x);
			auto zDistance = std::abs(playerPos.z - chunk_pos.z);
			if (xDistance > view_distance || zDistance > view_distance)
				work_group[i] = c;
			else
				work_group[i]->queue_free();
		}
		for (auto chunk : work_group)
		{
			if (chunk != nullptr)
			{
				chunk->init(this,noise); //TODO: Init Chunks
			}
		} */
}

void VoxelWorld::generate_chunk_thread_loop() {
	std::array<VoxelChunk *, 8> work_group;
	ChunkMesher mesher;
	Vector3i playerPos = Vector3i(0, 0, 0); //TODO: Get player position
	while (true) {
		godot::UtilityFunctions::print("Generating chunks...\n");
		for (size_t i = 0; i < 8; i++) {
			VoxelChunk *c = inactive_chunks.Pop();
			if (c == nullptr)
				continue;
			auto chunk_pos = c->threaded_global_pos;
			auto xDistance = std::abs(playerPos.x - chunk_pos.x) / VoxelChunk::ChunkSize;
			auto zDistance = std::abs(playerPos.z - chunk_pos.z) / VoxelChunk::ChunkSize;
			//if (xDistance > view_distance || zDistance > view_distance)
			c->init(this, noise, &Finished_chunks, Vector3i(c->chunk_position));
			//else
			//    work_group[i]->call_deferred("queue_free");
		}
		if (inactive_chunks.ShouldKillThread()) {
			break;
		}
		//for (auto chunk : work_group)
		//{
		//    if (chunk != nullptr)
		//    {
		//		//chunk->call_deferred("set_mesh", mesh);
		//		//chunk_mutex->lock();
		//		chunk->init(this,noise,&Finished_chunks,Vector3i(chunk->chunk_position));
		//		//chunk_mutex->unlock();
		//    }
		//}
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
