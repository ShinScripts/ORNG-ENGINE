#pragma once
#include <assimp/config.h>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "WorldTransform.h"
#include "Texture.h"
#include "Material.h"

class BasicMesh {
public:
	BasicMesh(const std::string& filename) : m_filename(filename) {};
	BasicMesh(const BasicMesh& other) = default;
	~BasicMesh() {};

	friend class Renderer; // renderer needs access to basically everything for the draw call

	void LoadIntoGL();

	bool LoadMeshData();

	void UnloadMesh();

	void UpdateTransformBuffers(const std::vector<WorldTransform const*>* transforms);

	std::string GetFilename() const { return m_filename; };

	bool GetLoadStatus() const { return is_loaded; };

private:

	bool is_loaded = false;

	const aiScene* p_scene = nullptr;

	Assimp::Importer importer;

	void Clear();

	void LoadTextures(const std::string& t_dir, const aiMaterial* pMaterial, unsigned int index);

	void LoadDiffuseTexture(const std::string& t_dir, const aiMaterial* pMaterial, unsigned int index);

	void LoadSpecularTexture(const std::string& t_dir, const aiMaterial* pMaterial, unsigned int index);

	void LoadColors(const aiMaterial* pMaterial, unsigned int index);

	bool InitFromScene(const aiScene* pScene, const std::string& filename);

	void ReserveSpace(unsigned int NumVertices, unsigned int NumIndices);

	void InitAllMeshes(const aiScene* pScene);

	void InitSingleMesh(const aiMesh* paiMesh);

	bool InitMaterials(const aiScene* pScene, const std::string& filename);

	void CountVerticesAndIndices(const aiScene* pScene, unsigned int& NumVertices, unsigned int& NumIndices);


	void PopulateBuffers();

	std::string m_filename;

	enum BUFFER_TYPE {
		INDEX_BUFFER = 0,
		POS_VB = 1,
		TEXCOORD_VB = 2,
		NORMAL_VB = 3,
		WORLD_MAT_VB = 4,
		WVP_MAT_VB = 5,
		UNIFORM_BUFFER = 6,
		NUM_BUFFERS = 7
	};


	GLuint m_VAO = 0;

	unsigned int m_buffers[NUM_BUFFERS] = { 0 };

#define INVALID_MATERIAL 0xFFFFFFFF

	struct BasicMeshEntry {
		BasicMeshEntry() : numIndices(0), baseVertex(0), baseIndex(0), materialIndex(INVALID_MATERIAL) {};

		unsigned int numIndices;
		unsigned int baseVertex;
		unsigned int baseIndex;
		unsigned int materialIndex;
	};

	std::vector<BasicMeshEntry> m_meshes;
	std::vector<Texture> m_textures;
	std::vector<Material> m_materials;

	//temp space before loading into gpu
	std::vector<glm::fvec3> m_positions;
	std::vector<glm::fvec3> m_normals;
	std::vector<glm::fvec2> m_texCoords;
	std::vector<unsigned int> m_indices;
};