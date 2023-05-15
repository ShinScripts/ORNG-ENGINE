#include "pch/pch.h"

#include "util/TimeStep.h"
#include "scene/Scene.h"
#include "util/util.h"
#include "rendering/MeshInstanceGroup.h"
#include "rendering/Renderer.h"
#include "core/GLStateManager.h"
#include "scene/SceneEntity.h"
namespace ORNG {

	Scene::~Scene() {
		UnloadScene();
	}

	Scene::Scene() {
		m_spot_lights.reserve(Renderer::max_spot_lights);
		m_point_lights.reserve(Renderer::max_point_lights);
		m_mesh_instance_groups.reserve(20);
		m_mesh_components.reserve(100);

		/* Padding */
		m_entities.push_back(nullptr);
		m_mesh_components.push_back(nullptr);
		m_spot_lights.push_back(nullptr);
		m_point_lights.push_back(nullptr);
		m_script_components.push_back(nullptr);

		Material* default_material = new Material();
		default_material->diffuse_color = glm::vec3(1);
		default_material->ambient_color = glm::vec3(1);
		default_material->specular_color = glm::vec3(1);
		default_material->diffuse_texture = CreateTexture2DAsset("./res/textures/missing_texture.jpeg");
		default_material->normal_map_texture = CreateTexture2DAsset("./res/textures/terrain/coast_sand_rocks_02_nor_gl_2k.jpg");
		m_materials.push_back(default_material);
	}

	void Scene::Init() {
	}

	void Scene::UpdateScene() {
		for (int i = 0; i < m_mesh_instance_groups.size(); i++) {
			auto* group = m_mesh_instance_groups[i];

			group->ProcessUpdates();

			if (group->m_meshes.empty()) {
				delete group;
				m_mesh_instance_groups.erase(m_mesh_instance_groups.begin() + i);
			}
		}
		for (auto& script : m_script_components) {
			if (!script || !script->OnUpdate) continue;
			script->OnUpdate();
		}
	}

	PointLightComponent* Scene::AddPointLightComponent(unsigned long entity_id) {
		if (m_point_lights[entity_id]) {
			OAR_CORE_WARN("Pointlight component not added, entity '{0}' already has a pointlight component", m_entities[entity_id]->name);
			return nullptr;
		}

		PointLightComponent* comp = new PointLightComponent(entity_id);
		m_point_lights[comp->m_entity_handle] = comp;
		return m_point_lights[comp->m_entity_handle];
	}

	SpotLightComponent* Scene::AddSpotLightComponent(unsigned long entity_id) {
		if (m_spot_lights[entity_id]) {
			OAR_CORE_WARN("Spotlight component not added, entity '{0}' already has a spotlight component", m_entities[entity_id]->name);
			return nullptr;
		}
		SpotLightComponent* comp = new SpotLightComponent(entity_id);
		m_spot_lights[comp->m_entity_handle] = comp;
		return m_spot_lights[comp->m_entity_handle];
	}

	ScriptComponent* Scene::AddScriptComponent(unsigned long entity_id) {
		if (m_script_components[entity_id]) {
			OAR_CORE_WARN("Script component not added, entity '{0}' already has a script component", m_entities[entity_id]->name);
			return nullptr;
		}
		m_script_components[entity_id] = new ScriptComponent(entity_id);
		return m_script_components[entity_id];
	}

	MeshComponent* Scene::AddMeshComponent(unsigned long entity_id, const std::string& filename, unsigned int shader_id) {

		// Currently any mesh components inside scene are auto-instanced and grouped together in MeshInstanceGroups, depending on shader, material and asset type 

		if (m_mesh_components[entity_id]) {
			OAR_CORE_WARN("Mesh component not added, entity '{0}' already has a mesh component", m_entities[entity_id]->name);
			return nullptr;
		}

		if (m_mesh_components[entity_id] != nullptr) {
			OAR_CORE_CRITICAL("Component space already occupied, indexing misalignment");
			BREAKPOINT;
		}

		MeshComponent* comp = new MeshComponent(entity_id);

		//check if mesh data is already available for the instance group about to be created if new component does not have appropiate instance group
		int mesh_asset_index = -1;
		for (int i = 0; i < m_mesh_assets.size(); i++) {
			if (m_mesh_assets[i]->GetFilename() == filename) {
				mesh_asset_index = i;
				break;
			}
		}

		comp->m_shader_id = shader_id;

		if (mesh_asset_index == -1) {
			OAR_CORE_TRACE("Mesh data not found, creating for entity: {0}", filename);

			MeshAsset* mesh_data = CreateMeshAsset(filename);
			auto group = new MeshInstanceGroup(mesh_data, comp->m_shader_id, this, comp->m_material_id);

			m_mesh_instance_groups.push_back(group);
			group->AddMeshPtr(comp);
		}
		else {
			SortMeshIntoInstanceGroup(comp, m_mesh_assets[mesh_asset_index]);
		}

		m_mesh_components[comp->m_entity_handle] = comp;
		return m_mesh_components[comp->m_entity_handle];
	}

	Texture2D* Scene::LoadMeshAssetTexture(const std::string& dir, aiTextureType type, const aiMaterial* p_material) {
		Texture2D* p_tex = nullptr;

		if (p_material->GetTextureCount(type) > 0) {
			aiString path;

			if (p_material->GetTexture(type, 0, &path, NULL, NULL, NULL, NULL, NULL) == aiReturn_SUCCESS) {
				std::string p(path.data);
				std::string full_path;

				if (p.starts_with(".\\"))
					p = p.substr(2, p.size() - 2);

				full_path = dir + "/" + p;
				p_tex = CreateTexture2DAsset(full_path);

			}

		}

		return p_tex;
	}

	void Scene::LoadScene(Camera* cam) {
		TimeStep time = TimeStep(TimeStep::TimeUnits::MILLISECONDS);
		m_skybox.Init();
		m_terrain.Init(*cam);

		for (auto& mesh : m_mesh_assets) {
			if (mesh->GetLoadStatus() == false) {
				m_futures.push_back(std::async(std::launch::async, [&mesh] {mesh->LoadMeshData(); }));
			}
		}
		for (unsigned int i = 0; i < m_futures.size(); i++) {
			m_futures[i].get();
		}
		for (auto& mesh : m_mesh_assets) {
			if (mesh->GetLoadStatus() == false) {
				LoadMeshAssetIntoGPU(mesh);
			}
		}
		for (auto& group : m_mesh_instance_groups) {
			group->UpdateTransformSSBO();
		}

		OAR_CORE_INFO("Scene loaded in {0}ms", time.GetTimeInterval());
	}

	void Scene::UnloadScene() {
		OAR_CORE_INFO("Unloading scene...");
		for (MeshAsset* mesh_data : m_mesh_assets) {
			delete mesh_data;
		}
		for (auto* mesh : m_mesh_components) {
			delete mesh;
		}
		for (auto* group : m_mesh_instance_groups) {
			delete group;
		}
		for (auto* light : m_point_lights) {
			delete light;
		}
		for (auto* light : m_spot_lights) {
			delete light;
		}
		for (auto* entity : m_entities) {
			delete entity;
		}
		for (auto* script : m_script_components) {
			delete script;
		}
		for (auto* texture : m_texture_2d_assets) {
			if (texture)
				delete texture;
		}
		for (auto* material : m_materials) {
			delete material;
		}

		OAR_CORE_INFO("Scene unloaded");
	}

	void Scene::SortMeshIntoInstanceGroup(MeshComponent* comp, MeshAsset* asset) {

		// this tells the instance groups to update the transform buffers every frame before rendering to set the correct transforms, as now multiple sets of transforms are needed
		asset->m_is_shared_in_instance_groups = true;

		int group_index = -1;

		// check if new entity can merge into already existing instance group
		for (int i = 0; i < m_mesh_instance_groups.size(); i++) {
			//if same data, shader and material, can be combined so instancing is possible
			if (m_mesh_instance_groups[i]->m_mesh_asset == asset
				&& m_mesh_instance_groups[i]->m_group_shader_id == comp->m_shader_id
				&& m_mesh_instance_groups[i]->m_group_material_id == comp->m_material_id) {
				group_index = i;
				break;
			}
		}

		if (group_index != -1) { // if instance group exists, place into
			OAR_CORE_TRACE("Instance group found for entity: {0}, material id: {1}", asset->GetFilename(), comp->m_material_id);
			// add mesh component's world transform into instance group for instanced rendering
			MeshInstanceGroup* group = m_mesh_instance_groups[group_index];
			group->AddMeshPtr(comp);
		}
		else { //else if instance group doesn't exist but mesh data exists, create group with existing data
			OAR_CORE_TRACE("Mesh data found for entity: {0}", asset->GetFilename());
			MeshInstanceGroup* group = new MeshInstanceGroup(asset, comp->m_shader_id, this, comp->m_material_id);
			m_mesh_instance_groups.push_back(group);
			group->AddMeshPtr(comp);
		}

	}

	SceneEntity& Scene::CreateEntity(const char* name) {
		SceneEntity* ent = new SceneEntity(CreateEntityID(), this);
		ent->name = name;
		m_entities.push_back(ent);

		/* Allocate space for components */
		m_mesh_components.push_back(nullptr);
		m_spot_lights.push_back(nullptr);
		m_point_lights.push_back(nullptr);
		m_script_components.push_back(nullptr);

		return *ent;

	}

	unsigned int Scene::CreateMaterial() {
		Material* p_material = new Material();
		m_materials.push_back(p_material);
		Renderer::GetShaderLibrary().UpdateMaterialUBO(m_materials);
		return m_materials.size() - 1;
	}

	Texture2D* Scene::CreateTexture2DAsset(const std::string& filename) {

		for (auto& p_texture : m_texture_2d_assets) {
			if (p_texture->m_spec.filepath == filename) {
				OAR_CORE_WARN("Texture '{0}' already created", filename);
				return nullptr;
			}
		}

		Texture2D* p_tex = new Texture2D();
		p_tex->m_spec.filepath = filename;
		m_texture_2d_assets.push_back(p_tex);
		return p_tex;
	}

	MeshAsset* Scene::CreateMeshAsset(const std::string& filename) {
		for (auto mesh_data : m_mesh_assets) {
			if (mesh_data->GetFilename() == filename) {
				OAR_CORE_TRACE("Mesh asset already loaded in: {0}", filename);
				return mesh_data;
			}
		}
		MeshAsset* mesh_data = new MeshAsset(filename);
		m_mesh_assets.push_back(mesh_data);
		return mesh_data;
	}

	void Scene::DeleteMeshAsset(MeshAsset* data) {

		/* Remove asset from all components using it */
		for (int i = 0; i < m_mesh_instance_groups.size(); i++) {
			MeshInstanceGroup* group = m_mesh_instance_groups[i];

			if (group->m_mesh_asset == data) {
				for (auto& mesh : group->m_meshes) {
					group->ClearMeshes();
					mesh->mp_mesh_asset = nullptr;
				}

				m_mesh_instance_groups.erase(m_mesh_instance_groups.begin() + i);
				delete group;
			}
		}
		m_mesh_assets.erase(std::find(m_mesh_assets.begin(), m_mesh_assets.end(), data));
		delete data;
	};

	bool Scene::LoadMeshAssetIntoGPU(MeshAsset* asset) {

		if (asset->is_loaded) {
			OAR_CORE_TRACE("Mesh '{0}' is already loaded", asset->m_filename);
			return true;
		}

		GL_StateManager::BindVAO(asset->m_vao);

		// Get directory used for finding material textures
		std::string::size_type slash_index = asset->GetFilename().find_last_of("/");
		std::string dir;

		if (slash_index == std::string::npos) {
			dir = ".";
		}
		else if (slash_index == 0) {
			dir = "/";
		}
		else {
			dir = asset->GetFilename().substr(0, slash_index);
		}

		for (unsigned int i = 0; i < asset->p_scene->mNumMaterials; i++) {
			const aiMaterial* p_material = asset->p_scene->mMaterials[i];

			// Load material textures
			asset->m_materials[i].diffuse_texture = LoadMeshAssetTexture(dir, aiTextureType_DIFFUSE, p_material);
			asset->m_materials[i].normal_map_texture = LoadMeshAssetTexture(dir, aiTextureType_NORMALS, p_material);
			asset->m_materials[i].specular_texture = LoadMeshAssetTexture(dir, aiTextureType_SHININESS, p_material);

			// Load material colors 
			aiColor3D AmbientColor(0.0f, 0.0f, 0.0f);

			if (p_material->Get(AI_MATKEY_COLOR_AMBIENT, AmbientColor) == aiReturn_SUCCESS) {
				asset->m_materials[i].ambient_color.r = AmbientColor.r;
				asset->m_materials[i].ambient_color.g = AmbientColor.g;
				asset->m_materials[i].ambient_color.b = AmbientColor.b;
			}

			aiColor3D diffuse_color(0.0f, 0.0f, 0.0f);

			if (p_material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse_color) == aiReturn_SUCCESS) {
				asset->m_materials[i].diffuse_color.r = diffuse_color.r;
				asset->m_materials[i].diffuse_color.g = diffuse_color.g;
				asset->m_materials[i].diffuse_color.b = diffuse_color.b;
			}

			aiColor3D specular_color(0.0f, 0.0f, 0.0f);
			if (p_material->Get(AI_MATKEY_COLOR_SPECULAR, specular_color) == aiReturn_SUCCESS) {
				asset->m_materials[i].specular_color.r = specular_color.r;
				asset->m_materials[i].specular_color.g = specular_color.g;
				asset->m_materials[i].specular_color.b = specular_color.b;
			}

			// Create material in scene so it can be used globally, and modified
			Material* p_added_material = GetMaterial(CreateMaterial());
			*p_added_material = asset->m_materials[i]; //copy and preserve original (add original fetching later)
		}

		// Load into gpu
		asset->PopulateBuffers();
		asset->importer.FreeScene();
		asset->is_loaded = true;

	}

	unsigned long Scene::CreateEntityID() {
		return m_last_entity_id++;
	}
}