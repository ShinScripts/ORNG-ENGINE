#version 430 core

out layout(location = 0) vec4 g_position;
out layout(location = 1) vec4 normal;
out layout(location = 2) vec4 albedo;
out layout(location = 3) vec4 roughness_metallic_ao;
out layout(location = 4) uvec2 shader_material_id;


layout(binding = 1) uniform sampler2D diffuse_sampler;
layout(binding = 2) uniform sampler2D roughness_sampler;
layout(binding = 7) uniform sampler2D normal_map_sampler;
layout(binding = 8) uniform sampler2DArray diffuse_array_sampler;
layout(binding = 9) uniform sampler2D displacement_sampler;
layout(binding = 10) uniform sampler2DArray normal_array_sampler;
layout(binding = 13) uniform samplerCube cube_color_sampler;
layout(binding = 17) uniform sampler2D metallic_sampler;
layout(binding = 18) uniform sampler2D ao_sampler;


in vec4 vs_position;
in vec3 vs_normal;
in vec3 vs_tex_coord;
in vec3 vs_tangent;
in mat4 vs_transform;
in vec3 vs_original_normal;
in vec3 vs_view_dir_tangent_space;


struct Material {
	vec4 base_color_and_metallic;
	float roughness;
	float ao;
};


layout(std140, binding = 2) uniform commons{
	vec4 camera_pos;
	vec4 camera_target;
} ubo_common;


layout(std140, binding = 3) uniform Materials{ //change to ssbo
	Material materials[128];
} u_materials;


uniform uint u_material_id;
uniform uint u_shader_id;
uniform bool u_normal_sampler_active;
uniform bool u_roughness_sampler_active;
uniform bool u_metallic_sampler_active;
uniform bool u_displacement_sampler_active;
uniform bool u_ao_sampler_active;
uniform bool u_terrain_mode;
uniform bool u_skybox_mode;
uniform float u_parallax_height_scale;
uniform uint u_num_parallax_layers;


vec2 ParallaxMap()
{
	float layer_depth = 1.0 / float(u_num_parallax_layers);
	float current_layer_depth = 0.0f;
	vec2 current_tex_coords = vs_tex_coord.xy;
	float current_depth_map_value = 1.0 - texture(displacement_sampler, vs_tex_coord.xy).r;
	vec2 p = normalize(vs_view_dir_tangent_space).xy * u_parallax_height_scale;

	vec2 delta_tex_coords = p / u_num_parallax_layers;
	float delta_depth = layer_depth / u_num_parallax_layers;

	while (current_layer_depth < current_depth_map_value) {
		current_tex_coords -= delta_tex_coords;
		current_depth_map_value = 1.0 - texture(displacement_sampler, current_tex_coords).r;
		current_layer_depth += layer_depth;
	}

	vec2 prev_tex_coords = current_tex_coords + delta_tex_coords;

	float after_depth = current_depth_map_value - current_layer_depth;
	float before_depth = texture(displacement_sampler, prev_tex_coords).r - current_layer_depth + layer_depth;

	float weight = after_depth / (after_depth - before_depth);

	return prev_tex_coords * weight + current_tex_coords * (1.0 - weight);

}

mat3 CalculateTbnMatrix() {
	vec3 t = normalize(vs_tangent);
	vec3 n = normalize(vs_original_normal);

	t = normalize(t - dot(t, n) * n);
	vec3 b = cross(n, t);

	mat3 tbn = mat3(t, b, n);

	return tbn;
}


mat3 CalculateTbnMatrixTransform() {
	vec3 t = normalize(vec3(mat3(vs_transform) * vs_tangent));
	vec3 n = normalize(vec3(mat3(vs_transform) * vs_original_normal));

	t = normalize(t - dot(t, n) * n);
	vec3 b = cross(n, t);

	mat3 tbn = transpose(mat3(t, b, n));

	return tbn;
}



void main() {
	shader_material_id = uvec2(u_shader_id, u_material_id);
	vec2 adj_tex_coord = u_displacement_sampler_active ? ParallaxMap() : vs_tex_coord.xy;

	Material material = u_materials.materials[u_material_id];
	roughness_metallic_ao.r = u_roughness_sampler_active ? texture(roughness_sampler, adj_tex_coord.xy).r : material.roughness;
	roughness_metallic_ao.g = u_metallic_sampler_active ? texture(metallic_sampler, adj_tex_coord.xy).r : material.base_color_and_metallic.a;
	roughness_metallic_ao.b = u_ao_sampler_active ? texture(ao_sampler, adj_tex_coord.xy).r : material.ao;
	roughness_metallic_ao.a = 1.f;


	if (u_terrain_mode) {
		mat3 tbn = CalculateTbnMatrix();
		vec3 sampled_normal_1 = texture(normal_array_sampler, vec3(adj_tex_coord.xy, 0.0)).rgb * 2.0 - 1.0;
		vec3 sampled_normal_2 = texture(normal_array_sampler, vec3(adj_tex_coord.xy, 1.0)).rgb * 2.0 - 1.0;

		float terrain_factor = clamp(dot(vs_normal.xyz, vec3(0, 1.f, 0) * 0.5f), 0.f, 1.f); // slope
		vec3 mixed_terrain_normal = normalize(tbn * mix(sampled_normal_1, sampled_normal_2, terrain_factor).xyz);
		normal = vec4(normalize(mixed_terrain_normal), 1.f);
		g_position = vs_position;

		albedo = vec4(mix(texture(diffuse_array_sampler, vec3(adj_tex_coord.xy, 0.0)).rgb, texture(diffuse_array_sampler, vec3(adj_tex_coord.xy, 1.0)).rgb, terrain_factor), 1.f);

	}
	else if (u_skybox_mode) {
		g_position = vec4(ubo_common.camera_pos.xyz + normalize(vs_position.xyz) * 10000.f, 1.f); // give spherical appearance (used for fog)
		albedo = vec4(texture(cube_color_sampler, vs_tex_coord).rgb, 1.f);
	}
	else {
		g_position = vs_position;

		if (u_normal_sampler_active) {
			mat3 tbn = CalculateTbnMatrixTransform();
			normal = vec4(tbn * normalize(texture(normal_map_sampler, adj_tex_coord.xy).rgb * 2.0 - 1.0).rgb, 1);
		}
		else {
			normal = vec4(normalize(vs_normal), 1.f);
		}
		albedo = vec4(texture(diffuse_sampler, adj_tex_coord.xy).rgb * material.base_color_and_metallic.rgb, 1.f);
	}
}