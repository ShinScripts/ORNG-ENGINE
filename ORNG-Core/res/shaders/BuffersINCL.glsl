ORNG_INCLUDE "CommonINCL.glsl"

layout(std140, binding = 2) buffer PointLights {
	PointLight lights[];
} ubo_point_lights_shadow;


layout(std140, binding = 1) buffer PointLightsShadowless {
	PointLight lights[];
} ubo_point_lights_shadowless;

layout(std140, binding = 4) buffer SpotLights {
	SpotLight lights[];
} ubo_spot_lights_shadow;

layout(std140, binding = 3) buffer SpotLightsShadowless {
	SpotLight lights[];
} ubo_spot_lights_shadowless;


layout(std140, binding = 1) uniform GlobalLighting{
	DirectionalLight directional_light;
} ubo_global_lighting;

layout(std140, binding = 2) uniform commons{
	vec4 camera_pos;
	vec4 camera_target;
	float time_elapsed;
	float render_resolution_x;
	float render_resolution_y;
	float cam_zfar;
	float cam_znear;
} ubo_common;

layout(std140, binding = 0) uniform Matrices{
	mat4 projection; //base=16, aligned=0-64
	mat4 view; //base=16, aligned=64-128
	mat4 proj_view;
	mat4 inv_projection;
	mat4 inv_view;
} PVMatrices;