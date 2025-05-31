#ifndef DIRECT_MESH_INSTANCE_H
#define DIRECT_MESH_INSTANCE_H

#include "godot_cpp/classes/geometry_instance3d.hpp"
#include "godot_cpp/classes/mesh.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "non_copyable.h"

using namespace godot;

// Thin wrapper around VisualServer mesh instance API
class DirectMeshInstance : public NonCopyable {
public:
	DirectMeshInstance();
	DirectMeshInstance(DirectMeshInstance &&src);
	~DirectMeshInstance();

	bool is_valid() const;
	void create();
	void destroy();
	void set_world(World3D *world);
	void set_transform(Transform3D world_transform);
	void set_mesh(Ref<Mesh> mesh);
	void set_material_override(Ref<Material> material);
	void set_visible(bool visible);
	void set_cast_shadows_setting(RenderingServer::ShadowCastingSetting mode);
	void set_shader_instance_parameter(StringName key, Variant value);
	void set_gi_mode(GeometryInstance3D::GIMode mode);
	void set_render_layers_mask(int mask);
	void set_interpolated(const bool enabled);

	Ref<Mesh> get_mesh() const;
	const Mesh *get_mesh_ptr() const;

	// void move_to(DirectMeshInstance &dst);

	void operator=(DirectMeshInstance &&src);

private:
	RID _mesh_instance;
	Ref<Mesh> _mesh;
};

#endif // DIRECT_MESH_INSTANCE_H
