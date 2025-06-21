#include "packed_arrays.h"

template <typename PackedVector_T, typename T>
inline void copy_to_template(PackedVector_T &dst, Span<const T> src) {
	dst.resize(src.size());

	T *dst_data = dst.ptrw();
	// static_assert(sizeof(dst_data) == sizeof(T));
	memcpy(dst_data, src.data(), src.size() * sizeof(T));
}

void copy_to(PackedVector3Array &dst, Span<const Vector3> src) {
	copy_to_template(dst, src);
}

void copy_to(PackedInt32Array &dst, Span<const int32_t> src) {
	copy_to_template(dst, src);
}

void copy_to(PackedFloat32Array &dst, Span<const float> src) {
	copy_to_template(dst, src);
}

void copy_to(PackedColorArray &dst, Span<const Color> src) {
	copy_to_template(dst, src);
}

void copy_to(PackedByteArray &dst, Span<const uint8_t> src) {
	copy_to_template(dst, src);
}

void copy_to(Span<uint8_t> dst, const PackedByteArray &src) {
	const size_t src_size = src.size();
	const uint8_t *src_data = src.ptr();
	memcpy(dst.data(), src_data, src_size);
}

void copy_to(Span<float> dst, const PackedFloat32Array &src) {
	const size_t src_size = src.size();
	const float *src_data = src.ptr();
	memcpy(dst.data(), src_data, src_size * sizeof(float));
}
