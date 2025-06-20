#pragma once
#include "concave_polygon_shape_3d.h"
#include "godot_cpp/classes/mesh.hpp"

Ref<ConcavePolygonShape3D> concave_util::create_concave_polygon_shape(const Span<const Array> surfaces) {
	// Faster version of Mesh::create_trimesh_shape(), because `create_trimesh_shape` creates a Trimesh internally along
	// the way, which is super slow
	// See https://github.com/Zylann/godot_voxel/issues/54

	PackedVector3Array face_points;
	int face_points_size = 0;

	// find the correct size for face_points
	for (unsigned int i = 0; i < surfaces.size(); i++) {
		const Array &surface_arrays = surfaces[i];
		if (surface_arrays.size() == 0) {
			// That surface is empty
			continue;
		}
		// If the surface is not empty then it must have an expected amount of data arrays
		ERR_CONTINUE(surface_arrays.size() != Mesh::ARRAY_MAX);
		PackedInt32Array indices = surface_arrays[Mesh::ARRAY_INDEX];
		face_points_size += indices.size();
	}
	face_points.resize(face_points_size);

	if (face_points_size < 3) {
		return Ref<ConcavePolygonShape3D>();
	}

	// Deindex surfaces into a single one
	unsigned int face_points_offset = 0;
	for (unsigned int i = 0; i < surfaces.size(); i++) {
		const Array &surface_arrays = surfaces[i];
		if (surface_arrays.size() == 0) {
			continue;
		}
		PackedVector3Array positions = surface_arrays[Mesh::ARRAY_VERTEX];
		PackedInt32Array indices = surface_arrays[Mesh::ARRAY_INDEX];

		ERR_FAIL_COND_V(positions.size() < 3, Ref<ConcavePolygonShape3D>());
		ERR_FAIL_COND_V(indices.size() < 3, Ref<ConcavePolygonShape3D>());
		ERR_FAIL_COND_V(indices.size() % 3 != 0, Ref<ConcavePolygonShape3D>());

		unsigned int face_points_count = face_points_offset + indices.size();

		{
			Vector3 *w = face_points.ptrw();
			const int *index_r = indices.ptr();
			const Vector3 *position_r = positions.ptr();

			for (unsigned int p = face_points_offset; p < face_points_count; ++p) {
				const int ii = p - face_points_offset;
#ifdef DEBUG_ENABLED
				CRASH_COND(ii < 0 || ii >= indices.size());
#endif
				const int index = index_r[ii];
#ifdef DEBUG_ENABLED
				CRASH_COND(index < 0 || index >= positions.size());
#endif
				w[p] = position_r[index];
			}
		}

		face_points_offset += indices.size();
	}

	Ref<ConcavePolygonShape3D> shape;
	{
		shape.instantiate();
		shape->set_faces(face_points);
	}
	return shape;
}

PackedVector3Array deindex_mesh_pos_to_packed_vector3_array(
		const Span<const Vector3> positions,
		const Span<const int> indices) {
	PackedVector3Array face_points;
	face_points.resize(indices.size());
	{
		Vector3 *w = face_points.ptrw();
		for (unsigned int ii = 0; ii < indices.size(); ++ii) {
			const int index = indices[ii];
			w[ii] = positions[index];
		}
	}
	return face_points;
}

PackedVector3Array deindex_mesh_to_packed_vector3_array(
		const Span<const Vector3> vertices,
		const Span<const int32_t> indices) {
	PackedVector3Array face_points;
	face_points.resize(indices.size());

	{
		Span<Vector3> dst(face_points.ptrw(), face_points.size());
		for (unsigned int ii = 0; ii < indices.size(); ++ii) {
			const int index = indices[ii];
			dst[ii] = vertices[index];
		}
	}

	return face_points;
}

Ref<ConcavePolygonShape3D> concave_util::create_concave_polygon_shape(
		const Span<const Vector3> positions,
		const Span<const int> indices) {
	if (indices.size() < 3) {
		return Ref<ConcavePolygonShape3D>();
	}

	ERR_FAIL_COND_V(positions.size() < 3, Ref<ConcavePolygonShape3D>());
	ERR_FAIL_COND_V(indices.size() < 3, Ref<ConcavePolygonShape3D>());
	ERR_FAIL_COND_V(indices.size() % 3 != 0, Ref<ConcavePolygonShape3D>());

	const PackedVector3Array face_points = deindex_mesh_pos_to_packed_vector3_array(positions, indices);

	Ref<ConcavePolygonShape3D> shape;
	{
		shape.instantiate();
		shape->set_faces(face_points);
	}
	return shape;
}
