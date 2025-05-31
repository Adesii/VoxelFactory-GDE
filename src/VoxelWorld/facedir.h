#pragma once
#include "godot_cpp/variant/vector3.hpp"
#include <sys/stat.h>
#include <cstdint>
using namespace godot;

class FaceDir {
public:
	enum Dir : int {
		Up = 0,
		Down = 1,
		Left = 2,
		Right = 3,
		Front = 4,
		Back = 5,
		NUM_FACE_DIRS = 6
	};

	static const inline Vector3 WorldToSample(const FaceDir::Dir &dir, const uint32_t &axis, const int &x, const int &y) {
		switch (dir) {
			case FaceDir::Up:
				return Vector3({ static_cast<float>(x), static_cast<float>(axis + 1), static_cast<float>(y) });
			case FaceDir::Down:
				return Vector3({ static_cast<float>(x), static_cast<float>(axis), static_cast<float>(y) });
			case FaceDir::Left:
				return Vector3({ static_cast<float>(axis), static_cast<float>(y), static_cast<float>(x) });
			case FaceDir::Right:
				return Vector3({ static_cast<float>(axis + 1), static_cast<float>(y), static_cast<float>(x) });
			case FaceDir::Front:
				return Vector3({ static_cast<float>(x), static_cast<float>(y), static_cast<float>(axis) });
			case FaceDir::Back:
				return Vector3({ static_cast<float>(x), static_cast<float>(y), static_cast<float>(axis + 1) });
			default:
				return Vector3({ 0, 0, 0 }); // Invalid direction
		}
	};
	static const inline constexpr uint32_t NormalIndex(const FaceDir::Dir &dir) {
		switch (dir) {
			case FaceDir::Left:
				return 0;
			case FaceDir::Right:
				return 1;
			case FaceDir::Down:
				return 2;
			case FaceDir::Up:
				return 3;
			case FaceDir::Front:
				return 4;
			case FaceDir::Back:
				return 5;
			default:
				return 0; // Invalid direction
		}
	};
	static const inline Vector3 Normal(const FaceDir::Dir &dir) {
		switch (dir) {
			case FaceDir::Left:
				return Vector3({ -1, 0, 0 });
			case FaceDir::Right:
				return Vector3({ 1, 0, 0 });
			case FaceDir::Down:
				return Vector3({ 0, -1, 0 });
			case FaceDir::Up:
				return Vector3({ 0, 1, 0 });
			case FaceDir::Front:
				return Vector3({ 0, 0, 1 });
			case FaceDir::Back:
				return Vector3({ 0, 0, -1 });
			default:
				return Vector3({ 0, 0, 0 }); // Invalid direction
		};
	};
	static const inline constexpr bool ReverseOrder(FaceDir::Dir dir) {
		switch (dir) {
			case FaceDir::Left:
				return false;
			case FaceDir::Right:
				return true;
			case FaceDir::Down:
				return false;
			case FaceDir::Up:
				return true;
			case FaceDir::Front:
				return true;
			case FaceDir::Back:
				return false;
			default:
				return false; // Invalid direction
		};
	};
};
