#pragma once

#include "godot_cpp/variant/packed_byte_array.hpp"
#include "godot_cpp/variant/packed_color_array.hpp"
#include "godot_cpp/variant/packed_float32_array.hpp"
#include "godot_cpp/variant/packed_int32_array.hpp"
#include "godot_cpp/variant/packed_string_array.hpp"
#include "godot_cpp/variant/packed_vector2_array.hpp"
#include "godot_cpp/variant/packed_vector3_array.hpp"
#include "godot_cpp/variant/vector2.hpp"
#include "godot_cpp/variant/vector3.hpp"
#include "span.h"
#include <cstdint>

using namespace godot;

// Copy functions for matching types.
// Can't have code using template Vector if we want to support compiling both as module and extension.
// So the following are defined for every case instead of a template.
void copy_to(PackedVector3Array &dst, Span<const Vector3> src);
void copy_to(PackedInt32Array &dst, Span<const int32_t> src);
void copy_to(PackedColorArray &dst, Span<const Color> src);
void copy_to(PackedFloat32Array &dst, Span<const float> src);
void copy_to(PackedByteArray &dst, Span<const uint8_t> src);

void copy_to(Span<uint8_t> dst, const PackedByteArray &src);
void copy_to(Span<float> dst, const PackedFloat32Array &src);

template <typename T>
inline void copy_bytes_to(PackedByteArray &dst, Span<const T> src) {
	const size_t bytes_count = src.size() * sizeof(T);
	dst.resize(bytes_count);
	uint8_t *dst_w = dst.ptrw();
	assert(dst_w != nullptr);
	memcpy(dst_w, src.data(), bytes_count);
}

template <typename T>
inline void copy_bytes_to(PackedByteArray &dst, T src) {
	dst.resize(sizeof(T));
	uint8_t *dst_w = dst.ptrw();
	assert(dst_w != nullptr);
	memcpy(dst_w, &src, sizeof(T));
}

// template <typename T>
// Span<const T> to_span_const(const Vector<T> &a) {
// 	return Span<const T>(a.ptr(), 0, a.size());
// }

inline Span<const Vector2> to_span(const PackedVector2Array &a) {
	return Span<const Vector2>(a.ptr(), a.size());
}

inline Span<const Vector3> to_span(const PackedVector3Array &a) {
	return Span<const Vector3>(a.ptr(), a.size());
}

inline Span<const float> to_span(const PackedFloat32Array &a) {
	return Span<const float>(a.ptr(), a.size());
}

inline Span<const int32_t> to_span(const PackedInt32Array &a) {
	return Span<const int32_t>(a.ptr(), a.size());
}

inline Span<const uint8_t> to_span(const PackedByteArray &a) {
	return Span<const uint8_t>(a.ptr(), a.size());
}

inline Span<String> to_span(PackedStringArray &a) {
	return Span<String>(a.ptrw(), a.size());
}
