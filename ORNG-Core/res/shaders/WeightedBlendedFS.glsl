#version 460 core
// your first render target which is used to accumulate pre-multiplied color values
layout(location = 0) out vec4 accum;

// your second render target which is used to store pixel revealage
layout(location = 1) out float reveal;

layout(binding = 1) uniform sampler2D diffuse_sampler;
layout(binding = 2) uniform sampler2D roughness_sampler;
layout(binding = 3) uniform sampler2DArray dir_depth_sampler;
layout(binding = 4) uniform sampler2DArray spot_depth_sampler;
layout(binding = 6) uniform sampler2D view_depth_sampler;
layout(binding = 7) uniform sampler2D normal_map_sampler;
layout(binding = 9) uniform sampler2D displacement_sampler;
layout(binding = 13) uniform samplerCube cube_color_sampler;
layout(binding = 17) uniform sampler2D metallic_sampler;
layout(binding = 18) uniform sampler2D ao_sampler;
layout(binding = 20) uniform samplerCube diffuse_prefilter_sampler;
layout(binding = 21) uniform samplerCube specular_prefilter_sampler;
layout(binding = 22) uniform sampler2D brdf_lut_sampler;
layout(binding = 25) uniform sampler2D emissive_sampler;
layout(binding = 26) uniform samplerCubeArray pointlight_depth_sampler;

in vec4 vs_position;
in vec3 vs_normal;
in vec3 vs_tex_coord;
in vec3 vs_tangent;
in mat4 vs_transform;
in vec3 vs_original_normal;
in vec3 vs_view_dir_tangent_space;

ORNG_INCLUDE "CommonINCL.glsl"

uniform uint u_shader_id;
uniform bool u_normal_sampler_active;
uniform bool u_roughness_sampler_active;
uniform bool u_metallic_sampler_active;
uniform bool u_displacement_sampler_active;
uniform bool u_emissive_sampler_active;
uniform bool u_ao_sampler_active;
uniform bool u_terrain_mode;
uniform bool u_skybox_mode;
uniform float u_parallax_height_scale;
uniform uint u_num_parallax_layers;
uniform float u_bloom_threshold;
uniform Material u_material;

#define DIR_DEPTH_SAMPLER dir_depth_sampler
#define POINTLIGHT_DEPTH_SAMPLER pointlight_depth_sampler
#define SPOTLIGHT_DEPTH_SAMPLER spot_depth_sampler
#define SPECULAR_PREFILTER_SAMPLER specular_prefilter_sampler
#define DIFFUSE_PREFILTER_SAMPLER diffuse_prefilter_sampler
#define BRDF_LUT_SAMPLER brdf_lut_sampler
ORNG_INCLUDE "LightingINCL.glsl"


vec4 CalculateAlbedoAndEmissive(vec2 tex_coord) {
vec4 sampled_albedo = texture(diffuse_sampler, tex_coord.xy);
	vec4 albedo_col = vec4(sampled_albedo.rgb * u_material.base_color.rgb, sampled_albedo.a);
	albedo_col *= u_material.emissive ? vec4(vec3(u_material.emissive_strength), 1.0) : vec4(1.0);

	if (u_emissive_sampler_active) {
		vec4 sampled_col = texture(emissive_sampler, tex_coord);
		albedo_col += vec4(sampled_col.rgb * u_material.emissive_strength * sampled_col.w, 0.0);
	}

	return albedo_col;
}



void main() {

    vec3 total_light = vec3(0.0, 0.0, 0.0);
    vec3 n = normalize(vs_normal);
	vec3 v = normalize(ubo_common.camera_pos.xyz - vs_position.xyz);
	vec3 r = reflect(-v, n);
	float n_dot_v = max(dot(n, v), 0.0);

	//reflection amount
	vec3 f0 = mix( vec3(0.03), u_material.base_color.rgb, u_material.metallic);

	float roughness = texture(roughness_sampler, vs_tex_coord.xy).r * float(u_roughness_sampler_active) + u_material.roughness * float(!u_roughness_sampler_active);
	float metallic = texture(metallic_sampler, vs_tex_coord.xy).r * float(u_metallic_sampler_active) + u_material.metallic * float(!u_metallic_sampler_active);
	float ao  = texture(ao_sampler, vs_tex_coord.xy).r * float(u_ao_sampler_active) + u_material.ao * float(!u_ao_sampler_active);
    vec4 albedo = CalculateAlbedoAndEmissive(vs_tex_coord.xy);

	total_light += CalculateDirectLightContribution(v, f0, vs_position.xyz, n, roughness, metallic, albedo.rgb);
	total_light += CalculateAmbientLightContribution(n_dot_v, f0, r, roughness, n, ao, metallic, albedo.rgb);
    vec4 color = vec4(total_light, u_material.base_color.a);

    float weight = clamp(pow(min(1.0, color.a * 10.0) + 0.01, 3.0) * 1e8 * 
                         pow(1.0 - gl_FragCoord.z * 0.9, 3.0), 1e-2, 3e3);

    accum = vec4(color.rgb * color.a, color.a) * weight;

    reveal = color.a;
}