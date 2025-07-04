#pragma once

#include "godot_cpp/classes/fast_noise_lite.hpp"
#include "godot_cpp/classes/node3d.hpp"
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/classes/thread.hpp"
#include "godot_cpp/variant/aabb.hpp"
#include "godot_cpp/variant/vector3i.hpp"
#include "mesh_result.h"
#include "util/MultiThreadQueues.h"
#include "util/vector3iutil.h"
#include "voxel_chunk.h"
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>

using namespace godot;

class VoxelWorld : public Node3D {
	GDCLASS(VoxelWorld, Node3D)

private:
	GenerateQueue<VoxelChunk *> inactive_chunks;
	std::vector<VoxelChunk *> wait_tree_chunks;
	std::unordered_map<Vector3i, VoxelChunk *, Vector3Hasher> chunks;
	std::mutex mutex;

	std::vector<Ref<Thread>> threads;

protected:
	//CompleteQueue<meshResult *> Finished_chunks;
	void init();
	void _notification(int p_what);
	static void _bind_methods();
	void generate_chunk_thread_loop();
	void upper_generate_chunk_thread_loop();

	int32_t view_distance = 10;

public:
	VoxelWorld();
	~VoxelWorld();

	Ref<FastNoiseLite> noise;
	Ref<FastNoiseLite> biome_noise;
	Ref<Material> material;

	void generate();

	void add_chunk(VoxelChunk *chunk);
	void gen_chunk(const Vector3i &pos, World3D *world);
	bool has_chunk(const Vector3i &pos) const;
	VoxelChunk *get_chunk(const Vector3i &pos) const;
	void queue_chunk(VoxelChunk *chunk);
	void gen_new_chunk_threaded_queue(Vector3i pos);

	void set_view_distance(const int32_t p_view_distance);
	int32_t get_view_distance() const;

	void set_noise(const Ref<FastNoiseLite> &p_noise);
	Ref<FastNoiseLite> get_noise();

	void set_biome_noise(const Ref<FastNoiseLite> &p_biome_noise);
	Ref<FastNoiseLite> get_biome_noise();

	void set_material(const Ref<Material> &p_material);
	Ref<Material> get_material();

	void single_thread_generate(float delta = 0);

	void add_modification(AABB mod_aabb, Vector3i hit_chunk_pos, int block_type);
};
