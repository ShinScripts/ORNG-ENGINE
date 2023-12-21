#include "pch/pch.h"
#include "components/ComponentSystems.h"
#include "scene/SceneEntity.h"
#include "assets/AssetManager.h"
#include "core/CodedAssets.h"
#include "scene/MeshInstanceGroup.h"

namespace ORNG {
	static void OnMeshComponentAdd(entt::registry& registry, entt::entity entity) {
		ComponentSystem::DispatchComponentEvent<MeshComponent>(registry, entity, Events::ECS_EventType::COMP_ADDED);
	}

	static void OnMeshComponentDestroy(entt::registry& registry, entt::entity entity) {
		ComponentSystem::DispatchComponentEvent<MeshComponent>(registry, entity, Events::ECS_EventType::COMP_DELETED);
	}

	static void OnBillboardComponentAdd(entt::registry& registry, entt::entity entity) {
		ComponentSystem::DispatchComponentEvent<BillboardComponent>(registry, entity, Events::ECS_EventType::COMP_ADDED);
	}

	static void OnBillboardComponentDestroy(entt::registry& registry, entt::entity entity) {
		ComponentSystem::DispatchComponentEvent<BillboardComponent>(registry, entity, Events::ECS_EventType::COMP_DELETED);
	}


	void MeshInstancingSystem::SortMeshIntoInstanceGroup(MeshComponent* comp) {

		if (comp->mp_instance_group) { // Remove first if it has an instance group
			comp->mp_instance_group->RemoveInstance(comp->GetEntity());
			comp->mp_instance_group = nullptr;
		}

		if (!comp->mp_mesh_asset)
			return;

		int group_index = -1;

		// check if new entity can merge into already existing instance group
		for (int i = 0; i < m_instance_groups.size(); i++) {
			//if same data and material, can be combined so instancing is possible
			if (m_instance_groups[i]->m_mesh_asset == comp->mp_mesh_asset
				&& m_instance_groups[i]->m_materials == comp->m_materials) {
				group_index = i;
				break;
			}
		}

		if (group_index != -1) { // if instance group exists, place into
			// add mesh component's world transform into instance group for instanced rendering
			MeshInstanceGroup* group = m_instance_groups[group_index];
			group->AddInstance(comp->GetEntity());
		}
		else { //else if instance group doesn't exist but mesh data exists, create group with existing data

			MeshInstanceGroup* group = new MeshInstanceGroup(comp->mp_mesh_asset, this, comp->m_materials);
			m_instance_groups.push_back(group);
			group->AddInstance(comp->GetEntity());

			group_index = m_instance_groups.size() - 1;
		}

		auto* p_group = m_instance_groups[group_index];
		comp->mp_instance_group = p_group;
	}

	void MeshInstancingSystem::OnBillboardAdd(BillboardComponent* p_comp) {
		if (!p_comp->p_material) {
			p_comp->p_material = AssetManager::GetAsset<Material>(ORNG_BASE_MATERIAL_ID);
		}

		SortBillboardIntoInstanceGroup(p_comp);
	}

	void MeshInstancingSystem::OnBillboardRemove(BillboardComponent* p_comp) {
		p_comp->mp_instance_group->RemoveInstance(p_comp->GetEntity());
	}

	void MeshInstancingSystem::SortBillboardIntoInstanceGroup(BillboardComponent* p_comp) {
		int group_index = -1;

		for (int i = 0; i < m_billboard_instance_groups.size(); i++) {
			if (m_billboard_instance_groups[i]->m_materials[0] == p_comp->p_material) {
				group_index = i;
			}
		}

		if (group_index != -1) {
			m_billboard_instance_groups[group_index]->AddInstance(p_comp->GetEntity());
		}
		else {
			auto* p_group = new MeshInstanceGroup(AssetManager::GetAsset<MeshAsset>(ORNG_BASE_QUAD_ID), this, p_comp->p_material);
			p_group->m_materials.push_back(p_comp->p_material);
			m_billboard_instance_groups.push_back(p_group);
			p_group->AddInstance(p_comp->GetEntity());
			group_index = m_billboard_instance_groups.size() - 1;
		}

		p_comp->mp_instance_group = m_billboard_instance_groups[group_index];
	}


	MeshInstancingSystem::MeshInstancingSystem(entt::registry* p_registry, uint64_t scene_uuid) : ComponentSystem(p_registry, scene_uuid) {
		// Setup event listeners
		m_transform_listener.scene_id = scene_uuid;
		m_transform_listener.OnEvent = [this](const Events::ECS_Event<TransformComponent>& t_event) {
			if (mp_registry->valid(entt::entity(t_event.affected_components[0]->GetEntity()->GetEnttHandle())))
				OnTransformEvent(t_event);
			};

		// Instance group handling
		m_mesh_listener.scene_id = scene_uuid;
		m_mesh_listener.OnEvent = [this](const Events::ECS_Event<MeshComponent>& t_event) {
			OnMeshEvent(t_event);
			};

		m_asset_listener.OnEvent = [this](const Events::AssetEvent& t_event) {
			if (t_event.event_type == Events::AssetEventType::MATERIAL_DELETED) {
				OnMaterialDeletion(reinterpret_cast<Material*>(t_event.data_payload));
			}
			else if (t_event.event_type == Events::AssetEventType::MESH_DELETED) {
				OnMeshAssetDeletion(reinterpret_cast<MeshAsset*>(t_event.data_payload));
			}
			};

		m_billboard_listener.scene_id = scene_uuid;
		m_billboard_listener.OnEvent = [this](const Events::ECS_Event<BillboardComponent>& t_event) {
			switch (t_event.event_type) {
			case Events::ECS_EventType::COMP_ADDED:
				OnBillboardAdd(t_event.affected_components[0]);
				break;
			case Events::ECS_EventType::COMP_DELETED:
				OnBillboardRemove(t_event.affected_components[0]);
				break;
			}
			};


		Events::EventManager::RegisterListener(m_mesh_listener);
		Events::EventManager::RegisterListener(m_asset_listener);
		Events::EventManager::RegisterListener(m_transform_listener);
		Events::EventManager::RegisterListener(m_billboard_listener);
	};



	void MeshInstancingSystem::OnMeshEvent(const Events::ECS_Event<MeshComponent>& t_event) {
		switch (t_event.event_type) {
		case Events::ECS_EventType::COMP_ADDED:
			if (!t_event.affected_components[0]->mp_mesh_asset) {
				t_event.affected_components[0]->mp_mesh_asset = AssetManager::GetAsset<MeshAsset>(ORNG_BASE_MESH_ID);
			}

			if (t_event.affected_components[0]->m_materials.empty())
				t_event.affected_components[0]->m_materials = { t_event.affected_components[0]->mp_mesh_asset->num_materials, AssetManager::GetAsset<Material>(ORNG_BASE_MATERIAL_ID) };

			SortMeshIntoInstanceGroup(t_event.affected_components[0]);
			break;
		case Events::ECS_EventType::COMP_UPDATED:
			// Materials will be empty if the mesh asset has been changed
			if (t_event.affected_components[0]->m_materials.empty())
				t_event.affected_components[0]->m_materials = { t_event.affected_components[0]->mp_mesh_asset->num_materials, AssetManager::GetAsset<Material>(ORNG_BASE_MATERIAL_ID) };

			SortMeshIntoInstanceGroup(t_event.affected_components[0]);
			break;
		case Events::ECS_EventType::COMP_DELETED:
			t_event.affected_components[0]->mp_instance_group->RemoveInstance(t_event.affected_components[0]->GetEntity());
			break;
		}
	}



	void MeshInstancingSystem::OnTransformEvent(const Events::ECS_Event<TransformComponent>& t_event) {
		// Whenever a transform component changes, check if it has a mesh, if so then update the transform buffer of the instance group holding it.
		auto* p_entity = t_event.affected_components[0]->GetEntity();
		if (t_event.event_type == Events::ECS_EventType::COMP_UPDATED) {

			if (auto* meshc = p_entity->GetComponent<MeshComponent>()) {
				meshc->mp_instance_group->FlagInstanceTransformUpdate(meshc->GetEntity());
			}

			if (auto* p_billboard = p_entity->GetComponent<BillboardComponent>()) {
				p_billboard->mp_instance_group->FlagInstanceTransformUpdate(p_entity);
			}
		} 

	
	}




	void MeshInstancingSystem::OnLoad() {
		mp_registry->on_construct<MeshComponent>().connect<&OnMeshComponentAdd>();
		mp_registry->on_destroy<MeshComponent>().connect<&OnMeshComponentDestroy>();

		mp_registry->on_construct<BillboardComponent>().connect<&OnBillboardComponentAdd>();
		mp_registry->on_destroy<BillboardComponent>().connect<&OnBillboardComponentDestroy>();

	}




	void MeshInstancingSystem::OnUnload() {
		for (auto* group : m_instance_groups) {
			delete group;
		}

		m_instance_groups.clear();
	}



	void MeshInstancingSystem::OnMeshAssetDeletion(MeshAsset* p_asset) {
		// Remove asset from all components using it
		for (auto [entity, mesh] : mp_registry->view<MeshComponent>().each()) {
			if (mesh.GetMeshData() == p_asset)
				mesh.SetMeshAsset(AssetManager::GetAsset<MeshAsset>(ORNG_BASE_MESH_ID));
		}

		std::array<std::vector<MeshInstanceGroup*>*, 2> groups = { &m_instance_groups, &m_billboard_instance_groups };

		for (int y = 0; y < 2; y++) {
			for (int i = 0; i < groups[y]->size(); i++) {
				MeshInstanceGroup* group = (*groups[y])[i];

				if (group->m_mesh_asset == p_asset) {
					group->ClearMeshes();
					for (auto [p_ent, index] : group->m_instances) {
						p_ent->GetComponent<MeshComponent>()->mp_mesh_asset = nullptr;
					}

					// Delete all mesh instance groups using the asset as they cannot function without it
					m_instance_groups.erase(m_instance_groups.begin() + i);
					delete group;
				}
			}
		}
	}

	void MeshInstancingSystem::OnMaterialDeletion(Material* p_material) {
		for (int i = 0; i < m_instance_groups.size(); i++) {
			MeshInstanceGroup* group = m_instance_groups[i];

			std::vector<unsigned int> material_indices;

			// Replace material in group if it contains it
			for (int y = 0; y < group->m_materials.size(); y++) {
				const Material*& p_group_mat = group->m_materials[y];
				if (p_group_mat == p_material) {
					p_group_mat = AssetManager::GetAsset<Material>(ORNG_BASE_MATERIAL_ID);
					material_indices.push_back(y);
				}
			}

			if (material_indices.empty())
				continue;

			// Replace material in mesh if it contains it
			for (auto [p_ent, index] : group->m_instances) {
				for (auto valid_replacement_index : material_indices) {
					p_ent->GetComponent<MeshComponent>()->m_materials[valid_replacement_index] = AssetManager::GetAsset<Material>(ORNG_BASE_MATERIAL_ID);
				}
			}
		}
	}


	void MeshInstancingSystem::OnUpdate() {
		std::array<std::vector<MeshInstanceGroup*>*, 2> groups = { &m_instance_groups, &m_billboard_instance_groups };

		for (int y = 0; y < 2; y++) {
			std::vector<unsigned> group_deletion_indices;

			for (int i = 0; i < groups[y]->size(); i++) {
				auto* group = (*groups[y])[i];

				group->ProcessUpdates();

				// Check if group should be deleted
				if (group->m_instances.empty()) {
					group->ProcessUpdates();
					groups[y]->erase(groups[y]->begin() + i);
					delete group;
					i--;
				}
			}

		}
	}
}