#pragma once
#include "TerrainGenerator.h"

namespace ORNG {

	class Camera;
	class ChunkLoader;
	class TerrainChunk;

	class TerrainQuadtree {
	public:
		friend class Terrain;
		friend class EditorLayer;
		friend class Renderer;

		/*This constructor only to be used for master quadtree*/
		TerrainQuadtree(unsigned int width, int height_scale, unsigned int sampling_density, unsigned int seed, glm::vec3 center_pos, unsigned int resolution, ChunkLoader* loader);

		void Init(Camera& camera);
		void Update();
		TerrainQuadtree* FindParentWithChunk();
		TerrainQuadtree(glm::vec3 center_pos, TerrainQuadtree* parent);
		~TerrainQuadtree();

		void Subdivide();
		void Unsubdivide();

		/* Look for nodes within radius "boundary" from center_pos, leaf node matches stored in chunk_array, inclusive
		* Chunk pointers only guaranteed to be valid for one frame
		*/
		//void QueryChunks(std::vector<TerrainChunk*>& chunk_array, glm::vec3 center_pos, int boundary);
		void QueryTree(std::vector<TerrainQuadtree*>& node_array, glm::vec3 center_pos, int boundary);
		void QueryChunks(std::vector<TerrainQuadtree*>& node_array, glm::vec3 center_pos, int boundary);

		bool CheckIntersection(glm::vec3 position);

		/* Find leaf node inside quadtree that can contain position */
		TerrainQuadtree& DoBoundaryTest(glm::vec3 position);
		TerrainQuadtree& GetParent() {
			if (!m_is_root_node) return *m_parent;
			return *this;
		}


	private:

		TerrainChunk* m_chunk = nullptr;
		ChunkLoader* m_loader = nullptr;
		/*Point all subdivisions/lod will revolve around (currently the camera position) */
		Camera* m_lod_camera = nullptr;

		/* Nodes */
		std::vector<TerrainQuadtree> m_child_nodes;

		TerrainQuadtree* m_parent = nullptr;
		int m_master_width;
		bool m_is_root_node = false;
		bool m_is_subdivided = false;
		int m_subdivision_layer = 0;
		int m_max_subdivision_layer;
		int m_min_grid_width = 100;

		/*Data for building terrain*/
		glm::vec3 m_center_pos = glm::vec3(0, 0, 0);
		int m_width;
		int m_sampling_density;
		unsigned int m_seed;
		int m_height_scale;
		float m_resolution;

	};
}