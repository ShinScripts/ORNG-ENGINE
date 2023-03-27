#version 430 core

const int MAX_SPOT_LIGHTS = 5;

const unsigned int MATRICES_BINDING = 0;
//const unsigned int POINT_LIGHT_BINDING = 1;
//const unsigned int SPOT_LIGHT_BINDING = 2;

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 TexCoord;
in layout(location = 2) vec3 vertex_normal;
in layout(location = 3) mat4 transform;



layout(std140, binding = 0) uniform Matrices{
	mat4 projection; //base=16, aligned=0-64
	mat4 view; //base=16, aligned=64-128
} PVMatrices;


uniform int g_num_spot_lights;

uniform mat4 dir_light_matrix;

out vec4 spot;
out vec2 TexCoord0;
out vec4 vs_position;
out vec3 vs_normal;
out vec4 dir_light_frag_pos_light_space;


void main() {
	gl_Position = (PVMatrices.projection * PVMatrices.view * transform) * vec4(position, 1.0);
	vs_normal = transpose(inverse(mat3(transform))) * vertex_normal;
	vs_position = transform * vec4(position, 1.0f);
	dir_light_frag_pos_light_space = dir_light_matrix * vs_position;


	TexCoord0 = TexCoord;
}
