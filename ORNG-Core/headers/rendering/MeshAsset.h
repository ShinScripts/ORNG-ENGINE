#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "rendering/Material.h"
#include "components/BoundingVolume.h"
#include "VAO.h"
#include "util/UUID.h"

#define ORNG_MAX_MESH_INDICES 50'000'000


struct aiScene;
struct aiMesh;


namespace ORNG {

	class TransformComponent;

	class MeshAsset : public Asset {
	public:
		friend class Renderer;
		friend class EditorLayer;
		friend class MeshInstanceGroup;
		friend class SceneRenderer;
		friend class MeshInstancingSystem;
		friend class SceneSerializer;
		friend class CodedAssets;
		friend class AssetManager;


		MeshAsset() = delete;
		MeshAsset(const std::string& filename) : Asset(filename) {};
		MeshAsset(const std::string& filename, uint64_t t_uuid) : Asset(filename) { uuid = UUID(t_uuid); };
		MeshAsset(const MeshAsset& other) = default;
		~MeshAsset() =default;

		bool LoadMeshData();

		bool GetLoadStatus() const { return m_is_loaded; };

		unsigned int GetIndicesCount() const { return num_indices; }

		const AABB& GetAABB() const { return m_aabb; }

		const MeshVAO& GetVAO() const { return m_vao; }

		void ClearCPU_VertexData() {
			glFinish();
			m_vao.vertex_data.positions.clear();
			m_vao.vertex_data.normals.clear();
			m_vao.vertex_data.tangents.clear();
			m_vao.vertex_data.tex_coords.clear();
			m_vao.vertex_data.indices.clear();
		}

		unsigned GetNbMaterials() {
			return num_materials;
		}

		template<typename S>
		void serialize(S& s) {
			s.object(m_vao);
			s.object(m_aabb);
			s.value4b((uint32_t)m_submeshes.size());
			for (auto& entry : m_submeshes) {
				s.object(entry);
			}
			s.value1b((uint8_t)num_materials);
			s.object(uuid);
			s.text1b(filepath, ORNG_MAX_FILEPATH_SIZE);
		}

	private:

		// Callback for when this mesh has a VAO created for it and its materials set up by the AssetManager class
		// This is split from "LoadMeshData" so vertex data can be loaded asynchronously, cannot create the VAO asynchronously due to opengl contexts
		void OnLoadIntoGL();


		bool InitFromScene(const aiScene* p_scene);

		void ReserveSpace(unsigned int num_verts, unsigned int num_indices);

		void InitAllMeshes(const aiScene* p_scene);

		void InitSingleMesh(const aiMesh* p_ai_mesh);

		void CountVerticesAndIndices(const aiScene* p_scene, unsigned int& num_verts, unsigned int& num_indices);

		void PopulateBuffers();

		MeshVAO m_vao;

		AABB m_aabb;

		std::unique_ptr<Assimp::Importer> mp_importer = nullptr;

		unsigned int num_indices = 0;
		uint8_t num_materials = 0;

		bool m_is_loaded = false;

		const aiScene* p_scene = nullptr;


#define INVALID_MATERIAL 0xFFFFFFFF

		struct MeshEntry {
			MeshEntry() : num_indices(0), base_vertex(0), base_index(0), material_index(INVALID_MATERIAL) {};

			unsigned int num_indices;
			unsigned int base_vertex;
			unsigned int base_index;
			unsigned int material_index;
		};

		std::vector<MeshEntry> m_submeshes;

	};
}