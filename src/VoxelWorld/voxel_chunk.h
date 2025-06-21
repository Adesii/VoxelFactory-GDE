#pragma once

#include "godot_cpp/classes/array_mesh.hpp"
#include "godot_cpp/classes/mesh.hpp"
#include "godot_cpp/classes/mesh_instance3d.hpp"
#include "godot_cpp/classes/mutex.hpp"
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/classes/shape3d.hpp"
#include "godot_cpp/classes/world3d.hpp"
#include "godot_cpp/variant/packed_int32_array.hpp"
#include "godot_cpp/variant/transform3d.hpp"
#include "godot_cpp/variant/vector3.hpp"
#include "godot_cpp/variant/vector3i.hpp"
#include "util/direct_mesh_instance.h"
#include "util/direct_static_body.h"
#include <cstdint>
using namespace godot;

class VoxelWorld;
class VoxelChunk {
	//remove the copy constructor and assignment operator to prevent copying
	VoxelChunk(const VoxelChunk &) = delete;
	VoxelChunk &operator=(const VoxelChunk &) = delete;

	static void _bind_methods();
	PackedInt32Array voxels;
	Ref<Mutex> mutex;
	bool generated = false;

	Ref<World3D> _world;

	// Must match default value of `active`
	bool _visible = true;
	bool _collision_enabled = false;

	bool _parent_visible = true;

protected:
	void _set_visible(bool visible);

	inline void set_mesh_instance_visible(DirectMeshInstance &mi, bool visible) {
		if (visible) {
			mi.set_world(*_world);
		} else {
			mi.set_world(nullptr);
		}
	}

public:
	DirectMeshInstance _mesh_instance;
	DirectStaticBody _static_body;

	VoxelWorld *_voxel_world;

	Transform3D transform;
	Vector3 threaded_global_pos = Vector3(0, 0, 0);
	VoxelChunk();
	~VoxelChunk();

	static const size_t ChunkSize = 62; // Number of blocks in a direction inside of a chunk
	static const size_t ChunkSize_P = ChunkSize + 2;
	static const size_t ChunkSize_P2 = ChunkSize_P * ChunkSize_P;
	static const size_t ChunkSize_P3 = ChunkSize_P * ChunkSize_P * ChunkSize_P;

	Vector3i chunk_position; // Position of the chunk in world space
	void set_chunk_position(Vector3i pos) {
		//mutex->lock();
		chunk_position = pos;
		//call_deferred("set_global_position", pos * ChunkSize);
		threaded_global_pos = pos * ChunkSize;
		transform = Transform3D(Basis(), threaded_global_pos);
		//mutex->unlock();
	}
	Vector3i get_chunk_position() const {
		//mutex->lock();
		auto ret = chunk_position;
		//mutex->unlock();
		return ret;
	}

	void init(VoxelWorld *w);

	void main_thread_init();

	void _notification(int what);

	void update_mesh();

	bool set_voxel(Vector3i pos, uint32_t id);

	void update_collision_shape();

	///From Godot-Voxel-Engine.. can't be bothered wrapping it myself so im just stealing :)
	/// Do check it out. really good way to do Voxel Rendering in Godot. i just like the painful way.

	void set_world(Ref<World3D> p_world);

	// Visuals

	void set_mesh(
			Ref<Mesh> mesh,
			GeometryInstance3D::GIMode gi_mode,
			RenderingServer::ShadowCastingSetting shadow_setting,
			int render_layers_mask);

	Ref<Mesh> get_mesh() const;
	bool has_mesh() const;
	void drop_mesh();

	// Note, GIMode is not stored per block, it is a shared option so we provide it in several functions.
	// Call this function only if the mesh block already exists and has not changed mesh
	void set_gi_mode(GeometryInstance3D::GIMode mode);

	// Note, ShadowCastingSetting is not stored per block, it is a shared option so we provide it in several functions.
	// Call this function only if the mesh block already exists and has not changed mesh
	void set_shadow_casting(RenderingServer::ShadowCastingSetting setting);

	// Note, render layers is not stored per block, it is a shared option so we provide it in several functions.
	// Call this function only if the mesh block already exists and has not changed mesh
	void set_render_layers_mask(int mask);

	void set_visible(bool visible);
	bool is_visible() const;

	void set_parent_visible(bool parent_visible);
	void set_parent_transform(const Transform3D &parent_transform);
	// Collisions

	void set_collision_shape(Ref<Shape3D> shape, bool debug_collision, const Node3D *node, float margin);
	bool has_collision_shape() const;
	void set_collision_layer(int layer);
	void set_collision_mask(int mask);
	void set_collision_margin(float margin);
	void drop_collision();
	// TODO Collision layer and mask

	void set_collision_enabled(bool enable, float &time_since_last_collision_mesh_creation);
	bool is_collision_enabled() const;
};
