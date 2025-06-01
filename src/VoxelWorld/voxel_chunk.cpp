#include "voxel_chunk.h"
#include "VoxelWorld.h"
#include "godot_cpp/classes/array_mesh.hpp"
#include "godot_cpp/classes/geometry_instance3d.hpp"
#include "godot_cpp/classes/mesh.hpp"
#include "godot_cpp/classes/mesh_instance3d.hpp"
#include "godot_cpp/classes/mutex.hpp"
#include "godot_cpp/classes/noise.hpp"
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/core/memory.hpp"
#include "godot_cpp/core/print_string.hpp"
#include "godot_cpp/variant/array.hpp"
#include "godot_cpp/variant/dictionary.hpp"
#include "godot_cpp/variant/packed_float32_array.hpp"
#include "godot_cpp/variant/packed_int32_array.hpp"
#include "godot_cpp/variant/vector3.hpp"
#include "godot_cpp/variant/vector3i.hpp"
#include "mesh_result.h"
#include "mesher.h"
#include "util/MultiThreadQueues.h"
#include <cstdint>
#include <mutex>
#include <vector>
void VoxelChunk::_bind_methods() {
	// Bind methods here if needed
}

VoxelChunk::VoxelChunk() {
	mutex.instantiate();
}
VoxelChunk::~VoxelChunk() {
	_mesh_instance.destroy();
}
void VoxelChunk::main_thread_init() {
	// Initialization logic that should be done on the main thread
	//mutex = memnew(Mutex);
	voxels.resize(ChunkSize_P3);
	//g_world->set_offset(Vector3i(chunk_position.x,chunk_position.z,0)*ChunkSize);
}

void VoxelChunk::init(VoxelWorld *w) {
	std::vector<uint32_t> local_voxels;
	local_voxels.resize(ChunkSize_P3);
	Vector3i swiffled_pos = Vector3i((chunk_position.z * ChunkSize), (chunk_position.x * ChunkSize), 0);
	//g_world->set_offset(swiffled_pos);
	Vector3i chunk_above = Vector3i();
	bool should_gen_chunk_above = false;
	// Initialization logic for the voxel chunk
	//g_world->set_offset(Vector3i(chunk_position.x,chunk_position.z,0)*ChunkSize);
	for (size_t z = 0; z < ChunkSize_P; ++z) {
		for (size_t x = 0; x < ChunkSize_P; ++x) {
			float n = w->noise->get_noise_2d(((float)swiffled_pos.x) + z, ((float)swiffled_pos.y) + x);
			for (size_t y = 0; y < ChunkSize_P; ++y) {
				size_t indexs = (z * ChunkSize_P + y) * ChunkSize_P + x;
				//size_t xy_index = (z * ChunkSize_P + x); // y * ChunkSize + x

				// Vector3I pos = Vector3I{(int)y, (int)x, (int)z}; // Chunk::ChunkSize_P * pos;

				if (indexs >= ChunkSize_P3) {
					continue;
				}

				float noise_point = ((n + 1.) / 2.) * 100 - chunk_position.y * ChunkSize;
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
					float othern = w->biome_noise->get_noise_2d((((float)swiffled_pos.x) + z), (((float)swiffled_pos.y) + x));
					if (othern > 0.5)
						(local_voxels)[indexs] = { 1 };
					else
						(local_voxels)[indexs] = { 2 };
					continue;
				}
				(local_voxels)[indexs] = { 1 };
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
		w->gen_chunk(chunk_above, *_world);
		//result->gen_new_pos = chunk_above;
		result->should_gen_new_chunk = false;
	} else {
		result->should_gen_new_chunk = false;
		result->gen_new_pos = Vector3i(0, 0, 0);
	}
	bool has_verts = false;
	result->mesh_data.resize(Mesh::ARRAY_MAX);
	mesher.MeshChunk(result->mesh_data, local_voxels, has_verts);
	if (has_verts) {
		Ref<ArrayMesh> mesh;
		mesh.instantiate();
		Mesh::ArrayFormat format = (Mesh::ArrayFormat)(
				Mesh::ARRAY_FORMAT_VERTEX |
				Mesh::ARRAY_FORMAT_NORMAL |
				Mesh::ARRAY_FORMAT_INDEX |
				Mesh::ARRAY_FORMAT_CUSTOM0 |
				Mesh::ARRAY_CUSTOM_R_FLOAT << Mesh::ARRAY_FORMAT_CUSTOM0_SHIFT);

		mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, result->mesh_data, Array(), Dictionary(), format);
		mesh->surface_set_material(0, w->material);
		set_mesh(mesh, godot::GeometryInstance3D::GI_MODE_DISABLED, godot::RenderingServer::SHADOW_CASTING_SETTING_ON, 0);
		set_parent_transform(transform);
		//print_line("Generated Chunk");
	}
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

///Godot-Voxel-Engine code:

void VoxelChunk::set_world(Ref<World3D> p_world) {
	if (_world != p_world) {
		_world = p_world;

		// To update world. I replaced visibility by presence in world because Godot 3 culling performance is horrible
		_set_visible(_visible && _parent_visible);

		//if (_static_body.is_valid()) {
		//	_static_body.set_world(*p_world);
		//}
	}
}

void VoxelChunk::set_mesh(Ref<Mesh> mesh,
		GeometryInstance3D::GIMode gi_mode,
		RenderingServer::ShadowCastingSetting shadow_setting,
		int render_layers_mask) {
	// TODO Don't add mesh instance to the world if it's not visible.
	// I suspect Godot is trying to include invisible mesh instances into the culling process,
	// which is killing performance when LOD is used (i.e many meshes are in pool but hidden)
	// This needs investigation.

	if (mesh.is_valid()) {
		if (!_mesh_instance.is_valid()) {
			// Create instance if it doesn't exist
			_mesh_instance.create();
			_mesh_instance.set_interpolated(false);
			_mesh_instance.set_gi_mode(gi_mode);
			_mesh_instance.set_cast_shadows_setting(shadow_setting);
			_mesh_instance.set_render_layers_mask(1); //TODO: do this later
			set_mesh_instance_visible(_mesh_instance, _visible && _parent_visible);
		}

		_mesh_instance.set_mesh(mesh);

#ifdef VOXEL_DEBUG_LOD_MATERIALS
		_mesh_instance.set_material_override(_debug_material);
#endif

	} else {
		if (_mesh_instance.is_valid()) {
			// Delete instance if it exists
			_mesh_instance.destroy();
		}
	}
}
Ref<Mesh> VoxelChunk::get_mesh() const {
	if (_mesh_instance.is_valid()) {
		return _mesh_instance.get_mesh();
	}
	return Ref<Mesh>();
}

bool VoxelChunk::has_mesh() const {
	return _mesh_instance.get_mesh().is_valid();
}

void VoxelChunk::drop_mesh() {
	if (_mesh_instance.is_valid()) {
		_mesh_instance.destroy();
	}
}

void VoxelChunk::set_visible(bool visible) {
	if (_visible == visible) {
		return;
	}
	_visible = visible;
	_set_visible(_visible && _parent_visible);
}

bool VoxelChunk::is_visible() const {
	return _visible;
}

void VoxelChunk::_set_visible(bool visible) {
	if (_mesh_instance.is_valid()) {
		set_mesh_instance_visible(_mesh_instance, visible);
	}
}

void VoxelChunk::set_parent_visible(bool parent_visible) {
	if (_parent_visible && parent_visible) {
		return;
	}
	_parent_visible = parent_visible;
	_set_visible(_visible && _parent_visible);
}

void VoxelChunk::set_parent_transform(const Transform3D &parent_transform) {
	if (_mesh_instance.is_valid()) {
		const Transform3D local_transform(Basis(), Vector3(0, 0, 0));
		const Transform3D world_transform = parent_transform * local_transform;

		if (_mesh_instance.is_valid()) {
			_mesh_instance.set_transform(world_transform);
		}
	}
}
