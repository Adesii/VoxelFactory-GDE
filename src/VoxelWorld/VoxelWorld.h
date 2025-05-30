#pragma once

#include "godot_cpp/classes/fast_noise_lite.hpp"
#include "godot_cpp/classes/mutex.hpp"
#include "godot_cpp/classes/node3d.hpp"
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/classes/thread.hpp"
#include "godot_cpp/variant/array.hpp"
#include "godot_cpp/variant/node_path.hpp"
#include "godot_cpp/variant/typed_dictionary.hpp"
#include "godot_cpp/variant/variant.hpp"
#include "godot_cpp/variant/vector3i.hpp"
#include "voxel_chunk.h"
#include "util/MultiThreadQueues.h"
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>
#include "mesh_result.h"
#include "util/vector3iutil.h"


using namespace godot;

class VoxelWorld : public Node3D {
    GDCLASS(VoxelWorld, Node3D)

private:
	GenerateQueue<VoxelChunk*> inactive_chunks;
	std::vector<VoxelChunk*> wait_tree_chunks;
	std::unordered_map<Vector3i,NodePath,Vector3Hasher> chunks;
	std::mutex mutex;

	std::vector<Ref<Thread>> threads;
protected:
	CompleteQueue<meshResult*> Finished_chunks;
	void init();
	void _notification(int p_what);
    static void _bind_methods();
	void generate_chunk_thread_loop();

	int32_t view_distance = 10;
public:
	Ref<Mutex> chunk_mutex;
    VoxelWorld();
    ~VoxelWorld();

	Ref<FastNoiseLite> noise;




	void generate();

	void add_chunk( VoxelChunk* chunk);
	void gen_chunk(const Vector3i& pos);
	bool has_chunk(const Vector3i& pos) const;
	void queue_chunk(VoxelChunk* chunk);

	void set_view_distance(const int32_t p_view_distance);
	int32_t get_view_distance() const;

	void set_noise(const Ref<FastNoiseLite>& p_noise);
	Ref<FastNoiseLite> get_noise();

	void single_thread_generate();

};
