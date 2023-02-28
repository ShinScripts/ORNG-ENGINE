#include <algorithm>
#include "Scene.h"

Scene::~Scene() {
	UnloadScene();
}

void Scene::LoadScene() {

}

void Scene::UnloadScene() {
	PrintUtils::PrintDebug("Unloading scene");
	for (PointLight* light : m_lights) {
		delete light;
	}
	for (auto group : m_group_mesh_instance_groups) {
		delete group;
	}
	PrintUtils::PrintSuccess("Scene unloaded");
}

MeshEntity* Scene::CreateMeshEntity(const std::string& filename) {
	int data_index = -1;
	for (int i = 0; i < m_group_mesh_instance_groups.size(); i++) {
		if (m_group_mesh_instance_groups[i]->m_mesh_data->GetFilename() == filename) data_index = i;
	}

	if (data_index != -1) {
		MeshEntity* mesh_entity = new MeshEntity(m_group_mesh_instance_groups[data_index]->m_mesh_data);
		m_group_mesh_instance_groups[data_index]->AddInstance(mesh_entity);

		PrintUtils::PrintDebug("Mesh data found, loading into entity: " + filename);
		return mesh_entity;
	}
	else {
		BasicMesh* mesh_data = CreateMeshData(filename);
		EntityInstanceGroup* instance_group = new EntityInstanceGroup(mesh_data);
		MeshEntity* mesh_entity = new MeshEntity(mesh_data);

		PrintUtils::PrintDebug("Mesh data not found, creating for entity: " + filename);
		instance_group->AddInstance(mesh_entity);
		m_group_mesh_instance_groups.push_back(instance_group);
		return mesh_entity;
	}
}

BasicMesh* Scene::CreateMeshData(const std::string& filename) {
	for (EntityInstanceGroup* group : m_group_mesh_instance_groups) {
		if (group->m_mesh_data->GetFilename() == filename) {
			PrintUtils::PrintError("MESH DATA ALREADY LOADED IN EXISTING INSTANCE GROUP: " + filename);
			return nullptr;
		}
	}

	BasicMesh* mesh_data = new BasicMesh(filename);
	EntityInstanceGroup* group = new EntityInstanceGroup(mesh_data);
	return mesh_data;
}