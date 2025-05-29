#pragma once

#include "godot_cpp/classes/node.hpp"


using namespace godot;
class VoxelWorld : public Node {
    GDCLASS(VoxelWorld, Node)
protected:
    static void _bind_methods();
public:
    VoxelWorld() = default;
    ~VoxelWorld() override = default;


};
