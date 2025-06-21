#include "mesher.h"
#include "facedir.h"
#include "godot_cpp/classes/array_mesh.hpp"
#include "godot_cpp/classes/class_db_singleton.hpp"
#include "godot_cpp/classes/mesh.hpp"
#include "godot_cpp/classes/mesh_instance3d.hpp"
#include "godot_cpp/classes/mutex.hpp"
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/core/memory.hpp"
#include "godot_cpp/variant/array.hpp"
#include "godot_cpp/variant/packed_byte_array.hpp"
#include "godot_cpp/variant/packed_int32_array.hpp"
#include "godot_cpp/variant/packed_vector3_array.hpp"
#include "godot_cpp/variant/typed_array.hpp"
#include "voxel_chunk.h"
#include <float.h>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <mutex>
#include <thread>
#include <vector>

const Vector2 ADJACENT_AO_DIRS[9]{
	Vector2({ -1.f, -1.f }),
	Vector2({ -1.f, 0.f }),
	Vector2({ -1.f, 1.f }),
	Vector2({ 0.f, -1.f }),
	Vector2({ 0.f, 0.f }),
	Vector2({ 0.f, 1.f }),
	Vector2({ 1.f, -1.f }),
	Vector2({ 1.f, 0.f }),
	Vector2({ 1.f, 1.f }),
};

struct GreedyQuad {
	uint8_t x, y, w, h;

	GreedyQuad(uint8_t x, uint8_t y, uint8_t w, uint8_t h) :
			x(x), y(y), w(w), h(h) {};

	bool operator==(const GreedyQuad &other) const {
		return x == other.x && y == other.y && w == other.w && h == other.h;
	}
	INTSIZE hash(GreedyQuad const &g) const noexcept {
		return ((INTSIZE)g.x) << 24 | ((INTSIZE)g.y) << 16 | ((INTSIZE)g.w) << 8 | (INTSIZE)g.h;
	}

	static constexpr inline INTSIZE convertToMask(INTSIZE h) {
#if INTSIZE == uint32_t
		if (h > 31)
#else
		if (h > 63)
#endif
		{
			// If h is too large for a 32-bit shift, return all bits set to 1
			return std::numeric_limits<INTSIZE>::max();
		} else {
			return (static_cast<INTSIZE>(1) << h) - static_cast<INTSIZE>(1);
		}
	}

	static const inline void greedy_mesh_binary_plane(std::array<INTSIZE, VoxelChunk::ChunkSize> &data, std::vector<GreedyQuad> &result) {
		for (uint8_t row = 0; row < data.size(); row++) {
			INTSIZE y = 0;
			while (y <= VoxelChunk::ChunkSize) {
				y += std::_Countr_zero(data[row] >> y);
				if (y > VoxelChunk::ChunkSize)
					continue;
				INTSIZE const h = std::_Countr_zero(~(data[row] >> y));

				INTSIZE const h_as_mask = convertToMask(h);
				INTSIZE const mask = h_as_mask << y;
				// grow horizontally
				uint8_t w = 1;
				while (row + w < VoxelChunk::ChunkSize) {
					INTSIZE const next_row_h = (data[row + w] >> y) & h_as_mask;
					if (next_row_h != h_as_mask)
						break;

					data[row + w] = data[row + w] & ~mask;
					w += 1;
				}
				result.emplace_back(GreedyQuad(
						static_cast<uint8_t>(row),
						static_cast<uint8_t>(y),
						static_cast<uint8_t>(w),
						static_cast<uint8_t>(h)));
				y += h;
			}
		}
	}
	const inline void append_verticies(PackedVector3Array &verts, PackedVector3Array &normals, PackedFloat32Array &ao_type_array, const FaceDir::Dir &facedir, const INTSIZE &axis, const uint32_t &ao, const uint32_t &blocktype) {
		const auto normal = FaceDir::NormalIndex(facedir);
		const auto normalDir = FaceDir::Normal(facedir);
		const uint32_t v1ao = ((((ao >> 0) & 1) + ((ao >> 1) & 1) + ((ao >> 3) & 1))) | (blocktype << 3);
		const uint32_t v2ao = ((((ao >> 3) & 1) + ((ao >> 6) & 1) + ((ao >> 7) & 1))) | (blocktype << 3);
		const uint32_t v3ao = ((((ao >> 5) & 1) + ((ao >> 8) & 1) + ((ao >> 7) & 1))) | (blocktype << 3);
		const uint32_t v4ao = ((((ao >> 1) & 1) + ((ao >> 2) & 1) + ((ao >> 5) & 1))) | (blocktype << 3);
		//const auto v1 = voxel_vertex_t(FaceDir::WorldToSample(facedir, axis, x, y), v1ao, normal, blocktype);
		//const auto v2 = voxel_vertex_t(FaceDir::WorldToSample(facedir, axis, x + w, y), v2ao, normal, blocktype);
		//const auto v3 = voxel_vertex_t(FaceDir::WorldToSample(facedir, axis, x + w, y + h), v3ao, normal, blocktype);
		//const auto v4 = voxel_vertex_t(FaceDir::WorldToSample(facedir, axis, x, y + h), v4ao, normal, blocktype);

		//std::array<voxel_vertex_t, 4> new_verts{v1, v2, v3, v4};
		//PackedVector3Array new_verts;
		//new_verts.append(FaceDir::WorldToSample(facedir, axis, x, y));
		//new_verts.append(FaceDir::WorldToSample(facedir, axis, x + w, y));
		//new_verts.append(FaceDir::WorldToSample(facedir, axis, x + w, y + h));
		//new_verts.append(FaceDir::WorldToSample(facedir, axis, x, y + h));

		//PackedVector3Array new_normals;
		//new_normals.append(normalDir);
		//new_normals.append(normalDir);
		//new_normals.append(normalDir);
		//new_normals.append(normalDir);

		if (FaceDir::ReverseOrder(facedir)) {
			//std::reverse(new_verts.begin() + 1, new_verts.end());
			//new_verts.reverse();
			// verts->append_range(new_verts);
			verts.append_array(
					{ FaceDir::WorldToSample(facedir, axis, x, y + h),
							FaceDir::WorldToSample(facedir, axis, x + w, y + h),
							FaceDir::WorldToSample(facedir, axis, x + w, y),
							FaceDir::WorldToSample(facedir, axis, x, y) });
			ao_type_array.append_array({ static_cast<float>(v4ao), static_cast<float>(v3ao), static_cast<float>(v2ao), static_cast<float>(v1ao) });
		} else {
			verts.append_array(
					{ FaceDir::WorldToSample(facedir, axis, x, y),
							FaceDir::WorldToSample(facedir, axis, x + w, y),
							FaceDir::WorldToSample(facedir, axis, x + w, y + h),
							FaceDir::WorldToSample(facedir, axis, x, y + h) });
			ao_type_array.append_array({ static_cast<float>(v1ao), static_cast<float>(v2ao), static_cast<float>(v3ao), static_cast<float>(v4ao) });
		}
		normals.append_array({ normalDir, normalDir, normalDir, normalDir });

		//verts.append_array(new_verts);
		//normals.append_array(new_normals);
	};

	// Function to compute hash so unordered set works
};

PackedInt32Array generate_indices(const size_t &vertex_count) {
	const size_t indices_count = (vertex_count - 1) / 4;
	PackedInt32Array indices; // Reserve space for 6 indices per quad
	//indices.resize((indices_count * 6) - 12);
	// indices.reserve(indices_count * 6); // Reserve space for 6 indices per quad

	for (int32_t vert_index = 0; vert_index <= indices_count; ++vert_index) {
		const int32_t base_index = vert_index * 4;
		//std::array<uint32_t, 6> inds{ (base_index + 3), (base_index + 2), (base_index),
		//	(base_index + 2), (base_index + 1), (base_index) };
		indices.append_array({ (base_index + 3), (base_index + 2), (base_index),
				(base_index + 2), (base_index + 1), (base_index) });
		//indices.append(inds[0]);
		//indices.append(inds[1]);
		//indices.append(inds[2]);
		//indices.append(inds[3]);
		//indices.append(inds[4]);
		//indices.append(inds[5]);
	}

	return indices;
}
/* Voxel get_block(std::vector<Voxel> &voxels, Vector3 pos)
{
	size_t index = static_cast<size_t>(pos.X + pos.Y * Chunk::ChunkSize_P + pos.Z * Chunk::ChunkSize_P * Chunk::ChunkSize_P);
	if (index < Chunk::ChunkSize_P3)
	{
		return voxels[index];
	}
	return {0};
} */

uint32_t get_block_by_n(const uint32_t &x, const uint32_t &y, const uint32_t &z, const PackedInt32Array &voxels) {
	uint32_t index = static_cast<uint32_t>(x + y * VoxelChunk::ChunkSize_P + z * VoxelChunk::ChunkSize_P * VoxelChunk::ChunkSize_P);
	if (index < VoxelChunk::ChunkSize_P3) {
		return voxels.get(index);
	}
	return 0;
}

void add_voxels_to_axis_cols(uint32_t voxel, size_t x, size_t y, size_t z, std::array<std::array<std::array<INTSIZE, VoxelChunk::ChunkSize_P>, VoxelChunk::ChunkSize_P>, 3> *axis_cols) {
	if (voxel == 0 || z >= VoxelChunk::ChunkSize_P || x >= VoxelChunk::ChunkSize_P || y >= VoxelChunk::ChunkSize_P || z < 0 || x < 0 || y < 0) {
		return;
	}

	(*axis_cols)[0][z][x] |= INTSIZE(1) << INTSIZE(y);
	(*axis_cols)[1][y][z] |= INTSIZE(1) << INTSIZE(x);
	(*axis_cols)[2][y][x] |= INTSIZE(1) << INTSIZE(z);
}

const std::array<uint32_t, 3> get_sample_pos(const size_t &axis, const Vector2 &v) {
	std::array<uint32_t, 3> sample_offset; // Back
	switch (axis) {
		case 0:
			sample_offset[0] = v.x; //{{v.X, -1, v.Y}}; // Back
			sample_offset[1] = -1;
			sample_offset[2] = v.y; //{{v.X, -1, v.Y}}; // Down
			break;
		case 1:
			// sample_offset = {{v.X, 1, v.Y}}; // Up
			sample_offset[0] = v.x; //{{v.X, -1, v.Y}}; // Back
			sample_offset[1] = 1;
			sample_offset[2] = v.y; //{{v.X, -1, v.Y}}; // UP
			break;
		case 2:
			// sample_offset = {{-1, v.Y, v.X}}; // Left
			sample_offset[0] = -1;
			sample_offset[1] = v.y; //{{v.X, -1, v.Y}}; // Left
			sample_offset[2] = v.x;
			break;
		case 3:
			// sample_offset = {{1, v.Y, v.X}}; // Right
			sample_offset[0] = 1;
			sample_offset[1] = v.y; //{{v.X, -1, v.Y}}; // Right
			sample_offset[2] = v.x;
			break;
		case 4:
			// sample_offset = {{v.X, v.Y, -1}}; // forward
			sample_offset[0] = v.x;
			sample_offset[1] = v.y; //{{v.X, -1, v.Y}}; // forward
			sample_offset[2] = -1;
			break;
		default:
			sample_offset[0] = v.x;
			sample_offset[1] = v.y;
			sample_offset[2] = 1;
			break;
	}
	return sample_offset;
}

void ChunkMesher::MeshChunk(Array &mesh_data, PackedInt32Array &voxels, bool &hasverts) {
	std::vector<GreedyQuad> greedy_quads;

	std::array<std::array<std::array<INTSIZE, VoxelChunk::ChunkSize_P>, VoxelChunk::ChunkSize_P>, 3> *axis_cols = new std::array<std::array<std::array<INTSIZE, VoxelChunk::ChunkSize_P>, VoxelChunk::ChunkSize_P>, 3>();
	std::array<std::array<std::array<INTSIZE, VoxelChunk::ChunkSize_P>, VoxelChunk::ChunkSize_P>, 6> *col_face_mask = new std::array<std::array<std::array<INTSIZE, VoxelChunk::ChunkSize_P>, VoxelChunk::ChunkSize_P>, 6>();
	// assert(voxels->size() == (Chunk::ChunkSize* Chunk::ChunkSize * Chunk::ChunkSize) || voxels->size() == 1);
	for (size_t z = 0; z < VoxelChunk::ChunkSize_P; ++z) {
		for (size_t y = 0; y < VoxelChunk::ChunkSize_P; ++y) {
			for (size_t x = 0; x < VoxelChunk::ChunkSize_P; ++x) {
				uint32_t voxel_index = x + y * VoxelChunk::ChunkSize_P + z * VoxelChunk::ChunkSize_P * VoxelChunk::ChunkSize_P;
				add_voxels_to_axis_cols(voxels.get(voxel_index), x, y, z, axis_cols);
			}
		}
	}

	// Face Culling
	for (size_t axis = 0; axis < 3; axis++) {
		for (size_t z = 0; z < VoxelChunk::ChunkSize; ++z) {
			for (size_t x = 0; x < VoxelChunk::ChunkSize; ++x) {
				INTSIZE col = (*axis_cols)[axis][z + 1][x + 1];
				(*col_face_mask)[2 * axis + 0][z + 1][x + 1] = col & ~(col << INTSIZE(1));
				(*col_face_mask)[2 * axis + 1][z + 1][x + 1] = col & ~(col >> INTSIZE(1));
			}
		}
	}

	std::array<std::unordered_map<INTSIZE, std::unordered_map<INTSIZE, std::array<INTSIZE, VoxelChunk::ChunkSize>>>, 6> data;
	// data = std::array<std::unordered_map<INTSIZE, std::unordered_map<INTSIZE, std::array<INTSIZE, Chunk::ChunkSize>>>, 6>{
	//     {{}, {}, {}, {}, {}, {}} // Initialize all maps to empty
	// };
	for (size_t axis = 0; axis < 6; ++axis) {
		for (INTSIZE z = 0; z < VoxelChunk::ChunkSize; ++z) {
			for (INTSIZE x = 0; x < VoxelChunk::ChunkSize; ++x) {
				INTSIZE col = (*col_face_mask)[axis][z + 1][x + 1];

				col >>= 1;
				col &= ~(INTSIZE(1) << INTSIZE(VoxelChunk::ChunkSize));
				while (col != 0) {
					INTSIZE y = std::_Countr_zero(col);
					col &= col - 1;

					// std::array<uint32_t, 3> voxelpos{static_cast<uint32_t>(x + 1), static_cast<uint32_t>(z + 1), static_cast<uint32_t>(y + 1)};
					size_t nx = x + 1;
					size_t ny = z + 1;
					size_t nz = y + 1;
					switch (axis) {
						case 0:
						case 1:
							// voxelpos = Vector3{{float(x), float(y), float(z)}};
							ny = y + 1;
							nz = z + 1;
							break;
						case 2:
						case 3:
							// voxelpos = Vector3{{float(y), float(z), float(x)}};
							nx = y + 1;
							ny = z + 1;
							nz = x + 1;
							break;
						default:
							break;
					}
					// voxelpos += Vector3{1, 1, 1};
					uint32_t ao_index = 0;
					uint32_t it_index = 0; // TODO: Fix AO
					for (const auto &v : ADJACENT_AO_DIRS) {
						auto sample_offset = get_sample_pos(axis, v);
						auto ao_block = get_block_by_n(nx + sample_offset[0], ny + sample_offset[1], nz + sample_offset[2], voxels);
						if (ao_block != 0) {
							ao_index |= uint32_t(1) << uint32_t(it_index);
						}
						it_index++;
					}

					uint32_t currentBlock = ao_index | ((get_block_by_n(nx, ny, nz, voxels)) << uint32_t(9));

					data[axis][currentBlock][y][x] |= INTSIZE(1) << INTSIZE(z);
				}
			}
		}
	}
	PackedVector3Array verts;
	//verts.resize(1000);
	PackedVector3Array normals;
	PackedFloat32Array ao_type_data;
	//normals.resize(1000);
	for (INTSIZE axis = 0; axis < 6; ++axis) {
		const auto block_data = &data[axis]; // get the data for this axis
		auto facedir = FaceDir::Back;
		switch (axis) {
			case 0:
				facedir = FaceDir::Down;
				break;
			case 1:
				facedir = FaceDir::Up;
				break;
			case 2:
				facedir = FaceDir::Left;
				break;
			case 3:
				facedir = FaceDir::Right;
				break;
			case 4:
				facedir = FaceDir::Front;
				break;
			default:
				break;
		}

		for (auto &[block, axis_plane] : *block_data) {
			auto ao = block & 0b111111111;
			auto block_type = block >> 9;
			for (auto &[axis_pos, plane] : axis_plane) {
				GreedyQuad::greedy_mesh_binary_plane(plane, greedy_quads);
				for (auto quad : greedy_quads) {
					quad.append_verticies(verts, normals, ao_type_data, facedir, axis_pos, ao, block_type);
				}
				greedy_quads.clear();
			}
		}
	}
	mesh_data[Mesh::ARRAY_VERTEX] = verts;
	mesh_data[Mesh::ARRAY_NORMAL] = normals;
	mesh_data[Mesh::ARRAY_CUSTOM0] = ao_type_data;
	if (!verts.is_empty()) {
		mesh_data[Mesh::ARRAY_INDEX] = generate_indices(verts.size());
		hasverts = true;
	}
	//mesh->call_deferred("add_surface_from_arrays", Mesh::PRIMITIVE_TRIANGLES, arrays);

	//mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);

	delete axis_cols;
	delete col_face_mask;
}
/*
// CULLED MESHING APPROACH FOR MY SANITY SAKE

std::vector<Vector3> get_corners(FaceDir::Dir dir, Vector3 pos)
{
	std::vector<Vector3> corners;
	pos.X = roundf(pos.X);
	pos.Y = roundf(pos.Y);
	pos.Z = roundf(pos.Z);

	switch (dir)
	{
	case FaceDir::Left:
		corners.push_back(Vector3{{pos.X, pos.Y, pos.Z}});
		corners.push_back(Vector3{{pos.X, pos.Y, pos.Z + 1}});
		corners.push_back(Vector3{{pos.X, pos.Y + 1, pos.Z + 1}});
		corners.push_back(Vector3{{pos.X, pos.Y + 1, pos.Z}});
		break;
	case FaceDir::Right:
		corners.push_back(Vector3{{pos.X, pos.Y + 1, pos.Z}});
		corners.push_back(Vector3{{pos.X, pos.Y + 1, pos.Z + 1}});
		corners.push_back(Vector3{{pos.X, pos.Y, pos.Z + 1}});
		corners.push_back(Vector3{{pos.X, pos.Y, pos.Z}});
		break;
	case FaceDir::Down:
		corners.push_back(Vector3{{pos.X, pos.Y, pos.Z}});
		corners.push_back(Vector3{{pos.X + 1, pos.Y, pos.Z}});
		corners.push_back(Vector3{{pos.X + 1, pos.Y, pos.Z + 1}});
		corners.push_back(Vector3{{pos.X, pos.Y, pos.Z + 1}});
		break;
	case FaceDir::Up:
		corners.push_back(Vector3{{pos.X, pos.Y, pos.Z + 1}});
		corners.push_back(Vector3{{pos.X + 1, pos.Y, pos.Z + 1}});
		corners.push_back(Vector3{{pos.X + 1, pos.Y, pos.Z}});
		corners.push_back(Vector3{{pos.X, pos.Y, pos.Z}});
		break;
	case FaceDir::Back:
		corners.push_back(Vector3{{pos.X, pos.Y, pos.Z}});
		corners.push_back(Vector3{{pos.X, pos.Y + 1, pos.Z}});
		corners.push_back(Vector3{{pos.X + 1, pos.Y + 1, pos.Z}});
		corners.push_back(Vector3{{pos.X + 1, pos.Y, pos.Z}});
		break;
	case FaceDir::Front:
		corners.push_back(Vector3{{pos.X + 1, pos.Y, pos.Z}});
		corners.push_back(Vector3{{pos.X + 1, pos.Y + 1, pos.Z}});
		corners.push_back(Vector3{{pos.X, pos.Y + 1, pos.Z}});
		corners.push_back(Vector3{{pos.X, pos.Y, pos.Z}});
		break;

	default:
		break;
	}
	return corners;
}

void push_face(Chunk::ChunkMesh *mesh, FaceDir::Dir dir, Vector3 pos, uint32_t block_type)
{
	auto quad = get_corners(dir, pos);
	for (auto &corner : quad)
	{
		mesh->vertices.push_back(voxel_vertex_t::MakeVertex(
			corner,
			FaceDir::NormalIndex(dir),
			block_type));
	}
}

Voxel get_block(Vector3 v, std::vector<Voxel> *voxels)
{
	if (v.X < 0 || v.Y < 0 || v.Z < 0 ||
		v.X >= Chunk::ChunkSize || v.Y >= Chunk::ChunkSize ||
		v.Z >= Chunk::ChunkSize)
	{
		return {0};
	}

	uint64_t x_i = static_cast<uint64_t>(v.X) % Chunk::ChunkSize;
	uint64_t y_i = static_cast<uint64_t>(v.Y) % Chunk::ChunkSize;
	uint64_t z_i = static_cast<uint64_t>(v.Z) % Chunk::ChunkSize;
	uint64_t index = x_i + y_i * Chunk::ChunkSize + z_i * Chunk::ChunkSize;

	if (index > voxels->size() || index < 0)
	{
		return {0};
	}

	return (*voxels)[index];
}

std::array<Voxel, 4> get_adjecent_block(Vector3 v, std::vector<Voxel> *voxels)
{
	std::array<Voxel, 4> adjecent_blocks;
	adjecent_blocks[0] = get_block(v, voxels);
	adjecent_blocks[1] = get_block(v + Vector3{{-1.0f, 0.0f, 0.0f}}, voxels);
	adjecent_blocks[2] = get_block(v + Vector3{{0.0f, 0.0f, -1.0f}}, voxels);
	adjecent_blocks[3] = get_block(v + Vector3{{0.0f, -1.0f, 0.0f}}, voxels);
	return adjecent_blocks;
}
Vector3 index_to_vec3(uint32_t index)
{
	return Vector3{
		{
			(float)(index / float(Chunk::ChunkSize * Chunk::ChunkSize)),
			(float)((index / Chunk::ChunkSize) % Chunk::ChunkSize),
			(float)(index % Chunk::ChunkSize),
		}};
}
Chunk::ChunkMesh *ChunkMesher::MeshChunkCulled(std::vector<Voxel> *voxels)
{
	auto chunk_mesh = new Chunk::ChunkMesh();
	for (size_t i = 0; i <= Chunk::ChunkSize * Chunk::ChunkSize * Chunk::ChunkSize; ++i)
	{
		auto local_voxel = index_to_vec3(i);
		auto [block, back, left, bottom] = get_adjecent_block(local_voxel, voxels);
		if (block.block_type != 0)
		{
			if (left.block_type == 0)
			{
				push_face(chunk_mesh, FaceDir::Back, local_voxel, block.block_type);
			}
			if (back.block_type == 0)
			{
				push_face(chunk_mesh, FaceDir::Left, local_voxel, block.block_type);
			}
			if (bottom.block_type == 0)
			{
				push_face(chunk_mesh, FaceDir::Down, local_voxel, block.block_type);
			}
		}
		else
		{
			if (left.block_type != 0)
			{
				push_face(chunk_mesh, FaceDir::Front, local_voxel, left.block_type);
			}
			if (back.block_type != 0)
			{
				push_face(chunk_mesh, FaceDir::Right, local_voxel, back.block_type);
			}
			if (bottom.block_type != 0)
			{
				push_face(chunk_mesh, FaceDir::Up, local_voxel, bottom.block_type);
			}
		}
	}
	chunk_mesh->indices = generate_indices(chunk_mesh->vertices.size());
	return chunk_mesh;
}
 */
