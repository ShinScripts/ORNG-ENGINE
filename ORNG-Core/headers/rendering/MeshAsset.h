#pragma once

#include "rendering/Material.h"
#include "components/BoundingVolume.h"
#include "VAO.h"
#include "util/UUID.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define ORNG_MAX_MESH_INDICES 50'000'000


class aiScene;
class aiMesh;


namespace ORNG {

	class TransformComponent;

	class MeshAsset {
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
		MeshAsset(const std::string& filename) : m_filename(filename) {};
		MeshAsset(const std::string& filename, uint64_t t_uuid) : m_filename(filename), uuid(t_uuid) {};
		MeshAsset(const MeshAsset& other) = default;
		~MeshAsset() = default;

		bool LoadMeshData();

		const auto& GetMaterials() const { return m_material_assets; }
		std::string GetFilename() const { return m_filename; };

		bool GetLoadStatus() const { return m_is_loaded; };

		unsigned int GetIndicesCount() const { return num_indices; }

		const AABB& GetAABB() const { return m_aabb; }

		const VAO& GetVAO() const { return m_vao; }

		UUID uuid;

		Assimp::Importer importer;


		template<typename S>
		void serialize(S& s) {
			s.object(m_vao);
			s.object(m_aabb);
			s.value4b((uint32_t)m_submeshes.size());
			for (auto& entry : m_submeshes) {
				s.object(entry);
			}
		}

	private:
		AABB m_aabb;

		unsigned int num_indices = 0;

		bool m_is_loaded = false;

		const aiScene* p_scene = nullptr;


		bool InitFromScene(const aiScene* pScene);

		void ReserveSpace(unsigned int NumVertices, unsigned int NumIndices);

		void InitAllMeshes(const aiScene* pScene);

		void InitSingleMesh(const aiMesh* paiMesh);

		void CountVerticesAndIndices(const aiScene* pScene, unsigned int& NumVertices, unsigned int& NumIndices);

		void PopulateBuffers();

		std::string m_filename = "";

		VAO m_vao;



#define INVALID_MATERIAL 0xFFFFFFFF

		struct MeshEntry {
			MeshEntry() : num_indices(0), base_vertex(0), base_index(0), material_index(INVALID_MATERIAL) {};

			unsigned int num_indices;
			unsigned int base_vertex;
			unsigned int base_index;
			unsigned int material_index;
		};

		std::vector<MeshEntry> m_submeshes;
		// Pointers to material assets created in assetmanager
		std::vector<Material*> m_material_assets;


	};
}