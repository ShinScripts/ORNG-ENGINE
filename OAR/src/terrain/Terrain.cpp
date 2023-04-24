#include "pch/pch.h"

#include "terrain/Terrain.h"


Terrain::Terrain(unsigned int seed, int width, glm::vec3 center_pos, int resolution, float height_scale, float sampling_density) : m_seed(seed),
m_width(width), m_center_pos(center_pos), m_resolution(resolution), m_height_scale(height_scale), m_sampling_density(sampling_density)
{
	m_quadtree = std::make_unique<TerrainQuadtree>(width, height_scale, sampling_density, seed, center_pos, resolution, &m_loader);
};



void Terrain::UpdateTerrainQuadtree() {
	m_quadtree->Update();
	m_loader.ProcessChunkLoads();
}


void Terrain::Init(Camera& camera) {
	m_quadtree->Init(camera);

	Texture2DArraySpec spec;
	spec.internal_format = GL_SRGB8_ALPHA8;
	spec.format = GL_RGBA;
	spec.tex_width = 2048;
	spec.tex_height = 2048;
	spec.min_filter = GL_LINEAR_MIPMAP_LINEAR;
	spec.mag_filter = GL_LINEAR;
	spec.generate_mipmaps = true;
	spec.filepaths = { "./res/textures/terrain/coast_sand_rocks_02_diff_2k.jpg", "./res/textures/terrain/coast_sand_rocks_02_diff_2k.jpg" };

	Texture2DArraySpec normal_spec;
	normal_spec.internal_format = GL_RGBA8;
	normal_spec.format = GL_RGBA;
	normal_spec.tex_width = 2048;
	normal_spec.tex_height = 2048;
	normal_spec.min_filter = GL_LINEAR_MIPMAP_LINEAR;
	normal_spec.mag_filter = GL_LINEAR;
	normal_spec.generate_mipmaps = true;
	normal_spec.filepaths = { "./res/textures/terrain/coast_sand_rocks_02_nor_gl_2k.jpg", "./res/textures/terrain/coast_sand_rocks_02_nor_gl_2k.jpg" };

	m_diffuse_texture_array.SetSpec(spec);
	m_diffuse_texture_array.Load();
	m_normal_texture_array.SetSpec(normal_spec);
	m_normal_texture_array.Load();
	m_terrain_top_mat.specular_color = glm::vec3(1.f);
	m_terrain_top_mat.diffuse_color = glm::vec3(1);

}
