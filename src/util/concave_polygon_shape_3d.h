#pragma once

#include "godot_cpp/classes/concave_polygon_shape3d.hpp"
#include "span.h"

using namespace godot;

class concave_util {
public:
	// Combines all mesh surface arrays into one collider.
	static Ref<ConcavePolygonShape3D> create_concave_polygon_shape(const Span<const Array> surfaces);

	static Ref<ConcavePolygonShape3D> create_concave_polygon_shape(
			const Span<const Vector3> positions,
			const Span<const int> indices);
};
